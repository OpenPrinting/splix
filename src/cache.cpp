/*
 * 	    cache.cpp                 (C) 2008, Aurélien Croc (AP²C)
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
#include "cache.h"
#include <stddef.h>

/*
 * Initialisation du cache
 * Cache initialization
 */
bool initializeCache()
{
    return false;
}



/*
 * Extraction d'une page du cache
 * Cache page extraction
 */
Page* getPage(unsigned long nr)
{
    return NULL;
}



/*
 * Modification de la politique de gestion du cache
 * Update the cache policy
 */
void setCachePolicy(CachePolicy policy)
{
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

