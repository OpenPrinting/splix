/*
 * 	    compress.cpp              (C) 2006-2008, Aurélien Croc (AP²C)
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
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#include "compress.h"
#include <math.h>
#include <string.h>
#include "page.h"
#include "band.h"
#include "errlog.h"
#include "request.h"
#include "bandplane.h"

#include "algo0x0d.h"
#include "algo0x0e.h"
#include "algo0x11.h"
#include "algo0x13.h"
#include "algo0x15.h"
#include "band_layout.h"

#include "compress_widths.inc"
#include "compress_banded.inc"
#ifndef DISABLE_JBIG
#include "compress_jbig.inc"
#endif
#include "compress_dispatch.inc"

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
