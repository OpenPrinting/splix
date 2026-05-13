/*
 * 	    sp_semaphore.cpp          (C) 2026, SpliX Modernization Project
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */
#include "sp_semaphore.h"

namespace SP {

Semaphore::Semaphore(std::ptrdiff_t initial_count) 
    : _sem(initial_count) 
{
}

void Semaphore::acquire() 
{
    _sem.acquire();
}

void Semaphore::release(std::ptrdiff_t update) 
{
    _sem.release(update);
}

bool Semaphore::try_acquire() 
{
    return _sem.try_acquire();
}

} // namespace SP
