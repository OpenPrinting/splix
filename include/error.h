/*
 * 	error.h			(C) 2006, Aurélien Croc (AP²C)
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
 *  $Id$
 * 
 */
#ifndef ERROR_H_
#define ERROR_H_

#include <stdio.h>

#define _(X)			X

#define ERROR(X, args ...)	fprintf(stderr, "[33m" X "[0m\n", ##args);
//#define DEBUG(X, args ...)	fprintf(stderr, "[32m" X "[0m\n", ##args);
#define DEBUG(X, args ...)


#endif /* ERROR_H_ */

