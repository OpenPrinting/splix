/*
 * 	raster.h		(C) 2006, Aurélien Croc (AP²C)
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
#ifndef RASTER_H_
#define RASTER_H_

#include "document.h"
#include <cups/raster.h>

class Raster : public Document
{
	protected:
		const char*	_jobId;
		const char*	_user;
		const char*	_title;
		const char*	_copies;
		const char*	_options;
		const char*	_file;
		int		_fd;

		cups_raster_t*	_ras;
		cups_page_header_t	_header;

		unsigned long	_width;
		unsigned long	_height;
		unsigned long	_lineSize;
		unsigned long	_line;
		unsigned long	_page;

		bool		_color;


		unsigned char*	_lineBuffer;

	public:
		Raster(const char *job, const char *user, const char *title, 
			const char *copies, const char *options, 
			const char *file);
		virtual ~Raster();

	public:
		virtual void	unload();
		virtual int	load();
		virtual int	loadPage(Printer *printer);
		
		virtual int	readLine();

	public:
		virtual unsigned long	width() const {return _width;}
		virtual unsigned long	height() const  {return _height;}
		virtual unsigned long	lineSize() const {return _lineSize;}
		virtual unsigned char*	lineBuffer() const {return _lineBuffer;}

	public:
		virtual bool		isColor() const {return _color;}
};

#endif /* RASTER_H_ */

