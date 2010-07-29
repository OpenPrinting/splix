/*
 *         algo0x0d.h SpliX is Copyright 2006-2008 by Aurélien Croc
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
 *  This code is written by Leonardo Hamada
 */
#ifndef _ALGO0x0D_H_
#define _ALGO0x0D_H_

#include "algorithm.h"
#include <inttypes.h>


/**
  * @brief This class implements the type 0xd encoding.
  */
class Algo0x0D : public Algorithm
{
    protected:
        inline void writeTwoBytesPacket(
                    unsigned char       * output,
                    unsigned long       & outputSize, 
                    long int            accumulatedHorizontalOffsetValue,
                    long int            accumulatedVerticalOffsetValue,
                    unsigned long       accumulatedRunCount );
    
        inline void writeFourBytesPacket(
                    unsigned char       * output,
                    unsigned long       & outputSize, 
                    long int            accumulatedHorizontalOffsetValue,
                    long int            accumulatedVerticalOffsetValue,
                    unsigned long       accumulatedRunCount );
    
        inline void writeSixBytesPacket(
                    unsigned char       * output,
                    unsigned long       & outputSize,
                    long int            accumulatedCombinedOffsetValue,
                    unsigned long       accumulatedRunCount );
    
        inline void selectPacketSize(
                    unsigned char       * output,
                    unsigned long       & outputSize,
                    unsigned long       preAccumulatedHorizontalOffsetValue,
                    unsigned long       accumulatedHorizontalOffsetValue,
                    unsigned long       currentHorizontalPenPosition,
                    unsigned long       accumulatedRunCount,
                    unsigned long       consecutiveBlankScanLines,
                    unsigned long       currentVerticalPenPosition,
                    const unsigned long wrapWidth );

    public:
        Algo0x0D();
        virtual ~Algo0x0D();

    public:
        virtual BandPlane*      compress(const Request& request, 
                                    unsigned char *data, unsigned long width,
                                    unsigned long height);
        virtual bool            reverseLineColumn() {return false;}
        virtual bool            inverseByte() {return false;}
        virtual bool            splitIntoBands() {return true;}
};

#endif /* _ALGO0x0D_H_ */
