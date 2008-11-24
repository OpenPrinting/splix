/*
 *         algo0x0e.cpp SpliX is Copyright 2006-2008 by Aurélien Croc
 *                      This file is a SpliX derivative work
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the
 *  Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Id$
 *
 * --
 * This code is written by Leonardo Hamada
 */
#include "algo0x0e.h"
#include <new>
#include <string.h>
#include "errlog.h"
#include "request.h"
#include "printer.h"
#include "bandplane.h"

#define getData(x) data[(x)]

#define setOutputData(y,z) output[(y)]=(z)

#define codecR(R, V, n)                         \
    addReplicativeRun(output, outputSize, R, V)

#define codecL(X, L, B, n)                                      \
    addLiteralSequence(output, outputSize, data, X, L, B)



/*
 * Constructor
 */
Algo0x0E::Algo0x0E()
{
}

Algo0x0E::~Algo0x0E()
{
}

inline void Algo0x0E::addLiteralSequence(
                                        unsigned char       * output,
                                        unsigned long       & outputSize,
                                        unsigned char       * data,
                                        unsigned long       position,
                                        unsigned long       length,
                                        unsigned long       blanks )
{
    /* Set control value for the literal chunk length. */
    unsigned long tmp = length + blanks - 1;

    unsigned long w;

    output[ outputSize++ ] = 0x80 | (unsigned char)(tmp >> 8);

    output[ outputSize++ ] = (unsigned char)tmp;

    /* Copy literal data chunk. */
    for(w=0;w<length;w++){
        output[outputSize++] = data[position + w];
    }

    /* Pad with required blanks. */
    for(w=0;w<blanks;w++){
        data[outputSize++] = 0xff;
    }
}

inline void Algo0x0E::addReplicativeRun(
                                       unsigned char       * output,
                                       unsigned long       & outputSize,
                                       unsigned long       runs,
                                       unsigned char       value )
{
    /* Verify run numbers to choose appropriate control header. */
    if ( 65 >= runs ) {
        output[ outputSize++ ] = 0x7f & (unsigned char)(1L - runs);
    } else {
        unsigned long t = 0xffff - runs + 2;

        output[ outputSize++ ] = (unsigned char)(t >> 8);

        output[ outputSize++ ] = (unsigned char)t;
    }

    /* Set value to be replicated. */
    output[ outputSize++ ] = value;
}



/* Check if segment at 'e' position and forward can be encoded as 
   consecutive runs. 'L' limits the width of the seek. */
unsigned long Algo0x0E::verifyGain(unsigned long e, 
                                  unsigned long L,
                                  unsigned char * data)
{
    unsigned long g=0, u=1;
    while(e+u<L){
    gain_seek:
	if(getData(e+u-1)==getData(e+u)){
            if(++u==4){
                break;
            }
        }else{
            break;
        };
    }
    if(u>=2){
        g+=(u<=65)?(u-2):(u-3);
        if(g<2){
            e+=u;
            if(e+1<L){
                u=1;
                goto gain_seek;
            }
        }
    }
    return g;
}

unsigned long Algo0x0E::encodeReplications(unsigned long q,
                                          unsigned long L,
                                          unsigned char * data,
                                          unsigned char * output,
                                          unsigned long & outputSize)
{
    unsigned long r;
 runs_enc: r=1;
    while(q+r<L){
        if(getData(q+r-1)==getData(q+r)){
            r++;
        }else{
            break;
        }
    }
    if(r>=2){
        codecR(r,getData(q),0);
        q+=r;
        goto runs_enc;
    }
    return q;
}

unsigned long Algo0x0E::locateBackwardReplications(unsigned long L,
                                                  unsigned char * data)
{
    /* This must be signed.*/
    long int i=L-1, r;     
 seek_literal2: r=1;
    while(i-r>=0){
        if(data[i-r+1]==data[i-r]){
            r++;
        }else{
            break;
        }
    }
    if(r>1){    
        i=i-r;
        goto seek_literal2;
    }
    return i+1;
}

BandPlane * Algo0x0E::compress(const Request & request, unsigned char *data,
                              unsigned long width, unsigned long height)
{
    /* Basic parameters validation. */
    if ( !data || !height || !width ) {
        ERRORMSG(_("Invalid given data for compression: 0xe"));
        return NULL;
    }

    /* We will interpret the band heigth of 128 pixels as 600 DPI printing
       request. Likewise, height of 64 pixels as 300 DPI printing. */
    if ( ! ( 128 == height || 64 == height ) ) {
        ERRORMSG(_("Invalid band height for compression: 0xe"));
        return NULL;
    }

    /* The row-bytes of the bitmap. */
    const unsigned long rowBytes = ( width + 7 ) / 8;

    /* This is the allowed raw data size per scan-line. */
    const unsigned long maxWorkRb = ( 0x40 == height ) ?
        0x09A0 / 8 : 0x1360 / 8;

    /* If rowBytes is larger than allowed size, print will be cropped. */
    const unsigned long workRb = ( rowBytes > maxWorkRb ) ?
        maxWorkRb : rowBytes;

    unsigned char * output = NULL;

    try {
        /* Estimate a buffer size equal to 2-byte control header overhead +
           maxWorkRb, times the bitmap height + up to 3-byte padding at end. */
        output = new unsigned char[ ( 2 + maxWorkRb ) * height + 3 ];
    } catch( std::bad_alloc & ){
        /* Catch error if buffer creation fails. */
        ERRORMSG(_("Could not allocate work buffer for encoding: 0xe"));
        return NULL;
    }

    /* Keep track of encoded data size. */
    unsigned long outputSize = 0;

    /* Main encoding loop for each scan-line.
       Top to bottom scan-line processing. */
    while(true){
        /*
          i: index into data.
          F: last replication encodable marker.
          E: resized WorkRb per scan-line.
          B: blank paddings.
        */
        unsigned long i, F, E, B;

        /* Adjust this working scan-line size
           up to where there is no blank bytes on the right end. */
	for(E=workRb;(E>0)&&(getData(E-1)==0xff);E--)
	/* Empty statement. */
        ;

        /* Determine the number of padding blank bytes to the right
           end of the scan-line relative to constant max. width. */ 
        B=maxWorkRb-E;
        
        /* If scan-line is not blank and the number of blank padding is 
           not 1, seek the beginning position of the last scan-line segment,
           when it can be encoded as contiguous replication runs. */
        F=((E>0)&&(B!=1))?locateBackwardReplications(E,data):E;
  
        /* Try to encode the first segment as replication runs. */
        i=(F>0)?encodeReplications(0,F,data,output,outputSize):0;

        /* Continue to encode the rest of data as replication or literal
           segment chunks as appropriate. */
        /* l: length of cumulative literal segment.*/
        unsigned long l=0;
        while(i+l+1<F){
            if(getData(i+l+1)!=getData(i+l)){
                l++;
            }else{
                if(verifyGain(i+l,F,data)>=2){
                    codecL(i,l,0,1);
                    i=encodeReplications(i+l,F,data,output,outputSize);
                    l=0;                                
                }else{
                    l++;
                }
            }
        }
        
        /* Encode last remaining segments and white paddings. */  
        if(F<E){
            /* Encode previous literal segment that remains unencoded. */
            if(i<F){
                codecL(i,F-i,0,2);
            }
            /* Encode the last non blank replication runs. */
            i=encodeReplications(F,E,data,output,outputSize);
            /* Ends a scan-line with required white paddings */
            if(B>=2){
                codecR(B,0xff,3);
            }
        }else{
            /* When the end of non blank data of a scan-line does not 
               end with replication runs, encode with literal sequence. */  
            if(B>=2){
                /* Encode previous literal segment that remains unencoded. */
                if(i<E){
                    codecL(i,E-i,0,4);
                }
                /* Ends a scan-line with required white paddings. */
                codecR(B,0xff,5);
            }else if((B>0)||(i<E)){
                /* Ends a scan-line encoding with a 
                   literal sequence with blanks if any. */
                codecL(i,E-i,B,6);
            }
        }

        if( --height>0 ){
            /* Proceed to the next scan-line. */
            data = & data[ rowBytes ];
        }else{
            break;
        }
    }

    /* Zero value byte padding for data size alignment to 4-bytes bounds. */
    unsigned long zerosPad = 4 - ( outputSize % 4 );

    /* Check if it is already aligned. */
    if ( 4 > zerosPad ){
        while ( zerosPad-- ){
            output[ outputSize++ ] = 0;
        }
    }

    /* Prepare to return data encoded by algorithm 0xe. */
    BandPlane * plane = new BandPlane();

    /* Regsiter data and its size. */
    plane->setData( output, outputSize );
    plane->setEndian( BandPlane::Dependant );

    /* Set this band encoding type. */
    plane->setCompression( 0xe );
    DEBUGMSG(_("Finished band encoding: type=0xe, size=%lu"), outputSize);
    /* Bye-bye. */
    return plane;
}
