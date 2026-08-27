// Split worker, threaded, and serial rendering paths.
#include "rendering_prelude.inc"
#ifndef DISABLE_THREADS
#include "rendering_worker.inc"
#include "rendering_threaded.inc"
#else /* DISABLE_THREADS */
#include "rendering_serial.inc"
#endif /* DISABLE_THREADS */
#include "rendering_trailer.inc"
