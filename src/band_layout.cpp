/*
 * Band layout helpers for SpliX fixed-width printer protocols.
 *
 * Copyright (C) 2026 Michael Maertzdorf
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "band_layout.h"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace splix {

unsigned long fixedBandWidthBytes(unsigned long xResolution)
{
    const unsigned long fixedWidthAt600Dpi = 4960;
    const unsigned long bitsPerByte = 8;
    return xResolution * fixedWidthAt600Dpi / 600 / bitsPerByte;
}

void copyBandRows(unsigned char* band,
                  unsigned long bandWidthInBytes,
                  unsigned long bandHeight,
                  const unsigned char* source,
                  unsigned long sourceStride,
                  unsigned long sourceOffset,
                  unsigned long rows,
                  BandLayout layout)
{
    std::memset(band, 0, bandWidthInBytes * bandHeight);
    if (sourceOffset >= sourceStride)
        return;

    const unsigned long copyWidth = std::min(
        sourceStride - sourceOffset, bandWidthInBytes);
    const unsigned long copyRows = std::min(rows, bandHeight);

    for (unsigned long y = 0; y < copyRows; ++y) {
        for (unsigned long x = 0; x < copyWidth; ++x) {
            const unsigned long destination =
                layout == ColumnMajor
                    ? x * bandHeight + y
                    : y * bandWidthInBytes + x;
            band[destination] = source[y * sourceStride + sourceOffset + x];
        }
    }
}

} // namespace splix
