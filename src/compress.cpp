// Split by compression responsibility; compiled as one translation unit.
#include "compress_shared.inc"
#include "compress_banded.inc"
#ifndef DISABLE_JBIG
#include "compress_jbig_banded.inc"
#include "compress_whole_page.inc"
#endif /* DISABLE_JBIG */
#include "compress_entry.inc"
