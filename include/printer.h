/*
 * 	printer.h		(C) 2006, Aurélien Croc (AP²C)
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
#ifndef PRINTER_H_
#define PRINTER_H_

#include <cups/ppd.h>
#include <stdlib.h>

class Printer
{
	protected:
		ppd_file_t*	_ppd;
		
		unsigned long	_resolution;
		unsigned char	_paperType;
		unsigned char	_paperSource;
		unsigned short	_duplex;

		unsigned char	_compVersion;

		long double	_pageSizeX;
		long double	_pageSizeY;
		long double	_marginX;
		long double	_marginY;
		long double	_areaX;
		long double	_areaY;

		char*		_docHeaderValues;
		
	protected:
		long double	_convert(long double d) const;
		char*		_convertStr(const char *str) const;

	public:
		Printer();
		Printer(ppd_file_t* ppd);
		~Printer();

		void		newJob(FILE *output);
		void		endJob(FILE *output);

	public:
		void		setPageSizeX(long double f) {_pageSizeX = f;}
		void		setPageSizeY(long double f) {_pageSizeY = f;}
		void		setMarginX(long double f) {_marginX = f;}
		void		setMarginY(long double f) {_marginY = f;}
		void		setAreaX(long double f) {_areaX = f;}
		void		setAreaY(long double f) {_areaY = f;}
		void		setResolution(unsigned long res) 
				{_resolution = res;}

		long double	pageSizeX() const {return _convert(_pageSizeX);}
		long double	pageSizeY() const {return _convert(_pageSizeY);}
		long double	marginX() const {return _convert(_marginX);};
		long double	marginY() const {return _convert(_marginY);};
		long double	areaX() const {return _convert(_areaX);}
		long double	areaY() const {return _convert(_areaY);}
		unsigned long	resolution() const {return _resolution;}
		unsigned long 	bandHeight() const {return 0x80;}

		unsigned char	paperType() const {return _paperType;}
		unsigned char	paperSource() const {return _paperSource;}
		unsigned short	duplex() const {return _duplex;}
		unsigned char	compVersion() const {return _compVersion;}
};

#endif /* PRINTER_H_ */

