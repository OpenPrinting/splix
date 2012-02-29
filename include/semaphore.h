/*
 * 	    semaphore.h               (C) 2007-2008, Aurélien Croc (AP²C)
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
#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#ifndef DISABLE_THREADS

#include <pthread.h>

/**
 * @brief This class provides the semaphore mechanism.
 */
class Semaphore {
    protected:
        unsigned long           _counter;
        pthread_mutex_t         _lock;
        pthread_cond_t          _cond;

        bool                    _mutex;

    public:
        /**
         * Initialize the Semaphore instance.
         * The value of the internal counter will be initialized to 1.
         */
        Semaphore();
        /**
         * Initialize the Semaphore instance by specifying the value of the 
         * internal counter.
         * @param counter the initial value of the internal counter.
         */
        Semaphore(unsigned long counter);
        /**
         * Destroy the instance.
         */
        virtual ~Semaphore();

    public:
        /**
         * Use the semaphore as a mutex and lock it.
         */
        void                    lock();
        /**
         * Use th semaphore as a mutex and unlock it.
         */
        void                    unlock();
        
        /**
         * Decrement the semaphore.
         */
        Semaphore&              operator --(int);
        /**
         * Increment the semaphore.
         */
        Semaphore&              operator ++(int);
};

#endif /* DISABLE_THREADS */

#endif /* _SEMAPHORE_H_ */

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

