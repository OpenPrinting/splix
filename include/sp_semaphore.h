/*
 * 	    sp_semaphore.h            (C) 2026, SpliX Modernization Project
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 */
#ifndef _SP_SEMAPHORE_H_
#define _SP_SEMAPHORE_H_

#include <semaphore>
#include <cstddef>

/**
 * @namespace SP
 * @brief SpliX Modernization Namespace
 */
namespace SP {

/**
 * @class Semaphore
 * @brief A modern C++20 wrapper for synchronization.
 *
 * This class provides a consistent interface for semaphores across the SpliX
 * driver, wrapping std::counting_semaphore for safety and modernization.
 */
class Semaphore {
    public:
        /**
         * @brief Construct a new Semaphore object
         * @param initial_count Initial value of the semaphore
         */
        explicit Semaphore(std::ptrdiff_t initial_count = 0);
        
        ~Semaphore() = default;

        // Prevent copying and assignment for thread safety
        Semaphore(const Semaphore&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;

        /**
         * @brief Decrements the internal counter or blocks until it is greater than zero.
         */
        void acquire();

        /**
         * @brief Increments the internal counter and unblocks any waiting threads.
         * @param update The value to increment by (default 1)
         */
        void release(std::ptrdiff_t update = 1);

        /**
         * @brief Tries to decrement the counter without blocking.
         * @return true if successfully acquired, false otherwise.
         */
        bool try_acquire();

    private:
        std::counting_semaphore<1024> _sem;
};

} // namespace SP

#endif // _SP_SEMAPHORE_H_
