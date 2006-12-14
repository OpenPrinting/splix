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

		const char*	_username;
		const char*	_jobname;
		
		unsigned long	_xresolution;
		unsigned long	_yresolution;
		unsigned char	_paperType;
		unsigned char	_paperSource;
		unsigned short	_duplex;

		unsigned char	_compVersion;

		unsigned short	_bandHeight;
		long double	_printableX;
		long double	_printableY;
		long double	_pageSizeX;
		long double	_pageSizeY;
		long double	_marginX;
		long double	_marginY;
		long double	_areaX;
		long double	_areaY;

		char*		_docHeaderValues;
		bool		_color;
		unsigned char	_qpdlVersion;
		
	protected:
		long double	_convertX(long double d) const;
		long double	_convertY(long double d) const;
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
		void		setResolution(unsigned long xres, unsigned long
					yres) 
				{_xresolution = xres; _yresolution = yres;}
		void		setPrintableX(long double f) {_printableX = f;}
		void		setPrintableY(long double f) {_printableY = f;}
		void		setCompVersion(unsigned char v) 
				{_compVersion = v;}
		void		setUsername(const char *user) 
				{_username = user;}
		void		setJobName(const char *job) {_jobname = job;}

		long double	pageSizeX() const 
				{return _convertX(_pageSizeX);}
		long double	pageSizeY() const 
				{return _convertY(_pageSizeY);}
		long double	marginX() const 
				{return _convertX(_marginX);};
		long double	marginY() const 
				{return _convertY(_marginY);};
		long double	areaX() const 
				{return _convertX(_areaX);}
		long double	areaY() const 
				{return _convertY(_areaY);}
		unsigned long	printableX() const 
				{return (unsigned long)_convertX(_printableX);}
		unsigned long 	printableY() const 
				{return (unsigned long)_convertY(_printableY);}
		unsigned long	resolutionX() const {return _xresolution;}
		unsigned long	resolutionY() const {return _yresolution;}
		unsigned long 	bandHeight() const {return _bandHeight;}

		unsigned char	paperType() const {return _paperType;}
		unsigned char	paperSource() const {return _paperSource;}
		unsigned short	duplex() const {return _duplex;}
		unsigned char	compVersion() const {return _compVersion;}

		char		docHeaderValues(unsigned long val) const
				{return _docHeaderValues[val];}

		bool		isColorPrinter() const {return _color;}
		unsigned char	qpdlVersion() const {return _qpdlVersion;}
};

#endif /* PRINTER_H_ */

