/*
 * Band layout helpers for SpliX fixed-width printer protocols.
 *
 * Copyright (C) 2026 Michael Maertzdorf
 * SPDX-License-Identifier: GPL-2.0-only
 */
#ifndef SPLIX_BAND_LAYOUT_H
#define SPLIX_BAND_LAYOUT_H

namespace splix {

enum BandLayout {
    RowMajor,
    ColumnMajor,
};

unsigned long fixedBandWidthBytes(unsigned long xResolution);

void copyBandRows(unsigned char* band,
                  unsigned long bandWidthInBytes,
                  unsigned long bandHeight,
                  const unsigned char* source,
                  unsigned long sourceStride,
                  unsigned long sourceOffset,
                  unsigned long rows,
                  BandLayout layout);

} // namespace splix

#endif /* SPLIX_BAND_LAYOUT_H */
