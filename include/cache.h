/*
 * 	    cache.h                   (C) 2008, Aurélien Croc (AP²C)
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
#ifndef _CACHE_H_
#define _CACHE_H_

class Page;

/**
  * List all the different cache policy available.
  */
enum CachePolicy {
    /** All pages are needed with page number increasing. */
    EveryPagesIncreasing,
    /** Only even pages are needed with page number decreasing. */
    EvenDecreasing, 
    /** Only odd pages are needed with page number increasing. */
    OddIncreasing,
};


/**
  * Initialize the cache mechanism and load the cache controller thread.
  * @return TRUE if the initialization succeed. Otherwise it returns FALSE.
  */
extern bool initializeCache();

/**
  * Extract a specific page from the cache.
  * @param nr the page number to extract.
  * @return the instance of the page. Otherwise it returns NULL if no page are
  *         found.
  */
extern Page* getPage(unsigned long nr);

/**
  * Set the new cache policy.
  * @param policy the new cache policy.
  */
extern void setCachePolicy(CachePolicy policy);

#endif /* _CACHE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

