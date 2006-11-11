/*
 * 	document.h		(C) 2006, Aurélien Croc (AP²C)
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
#ifndef DOCUMENT_H_
#define DOCUMENT_H_

#include <stdio.h>
#include "printer.h"

class Document
{
	public:
		Document() {};
		virtual ~Document() {};

	public:
		virtual void	unload() = 0;
		virtual int	load() = 0;
		virtual int	loadPage(Printer *printer) = 0;
		virtual int	readLine() = 0;

	public:
		virtual unsigned long	width() const = 0;
		virtual unsigned long	height() const = 0;
		virtual unsigned long	lineSize() const = 0;
		virtual unsigned char*	lineBuffer() const = 0;

	public:
		virtual bool		isColor() const = 0;
};

#endif /* DOCUMENT_H_ */

