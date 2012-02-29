/*
 * 	    options.h                 (C) 2006-2008, Aurélien Croc (AP²C)
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
#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#ifdef DISABLE_JBIG
    bool opt_jbig = false;
#else
    bool opt_jbig = true;
#endif /* DISABLE_JBIG */

#ifdef DISABLE_THREADS
    bool opt_threads = false;
#define THREADS 0
#define CACHESIZE 0
#else
    bool opt_threads = true;
#endif /* DISABLE_THREADS */

#ifdef DISABLE_BLACKOPTIM
    bool opt_blackoptim = false;
#else
    bool opt_blackoptim = true;
#endif /* DISABLE_BLACKOPTIM */

#endif /* _OPTIONS_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

