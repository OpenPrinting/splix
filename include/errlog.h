/*
 * 	    errlog.h                  (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _ERRLOG_H_
#define _ERRLOG_H_

#include <stdio.h>

#define _(X)            X

#ifdef DEBUG
#   define ERRORMSG(X, args ...) fprintf(stderr, "[33mERROR: " X "[0m\n", ##args);
#   define WARNMSG(X, args ...)  fprintf(stderr, "[34mWARNING: " X "[0m\n", ##args);
#   define DEBUGMSG(X, args ...) fprintf(stderr, "[32mDEBUG: " X "[0m\n", ##args);
#else
#   define ERRORMSG(X, args ...) fprintf(stderr, "ERROR: SpliX " X "\n", ##args);
#   define WARNMSG(X, args ...)  fprintf(stderr, "WARNING: SpliX " X "\n", ##args);
#   define DEBUGMSG(X, args ...) fprintf(stderr, "DEBUG: SpliX " X "\n", ##args);
#endif /* DEBUG */

#endif /* _ERRLOG_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

