/*
 *         algo0x0e.h SpliX is Copyright 2006-2008 by Aurélien Croc
 *                    This file is a SpliX derivative work
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
 * --
 * This code has been written by Leonardo Hamada
 */
#ifndef _ALGO0x0E_H_
#define _ALGO0x0E_H_

#include "algorithm.h"
#include <inttypes.h>

/**
  * @brief This class implements the type 0xe encoding.
  */
class Algo0x0E : public Algorithm
{
    protected:    
        inline void addLiteralSequence(
                                        unsigned char       * output,
                                        unsigned long       & outputSize, 
                                        unsigned char       * data,
                                        unsigned long       position,
                                        unsigned long       length,
                                        unsigned long       blanks );
        inline void addReplicativeRun(
                                        unsigned char       * output,
                                        unsigned long       & outputSize,
                                        unsigned long       runs,
                                        unsigned char       value );
	unsigned long verifyGain(unsigned long e,
                                 unsigned long L,
                                 unsigned char * data);
	unsigned long encodeReplications(unsigned long q,
                                         unsigned long L,
                                         unsigned char * data,
                                         unsigned char * output,
                                         unsigned long & outputSize);
	unsigned long locateBackwardReplications(unsigned long L,
                                                 unsigned char * data);

    public:
        Algo0x0E();
        virtual ~Algo0x0E();

    public:
        virtual BandPlane*      compress(const Request& request, 
                                    unsigned char *data, unsigned long width,
                                    unsigned long height);
        virtual bool            reverseLineColumn() {return false;}
        virtual bool            inverseByte() {return true;}
        virtual bool            splitIntoBands() {return true;}
};

#endif /* _ALGO0x0E_H_ */
