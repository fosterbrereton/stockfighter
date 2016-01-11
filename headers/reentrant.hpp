/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef reentrant_hpp__
#define reentrant_hpp__

/******************************************************************************/

// stdc++
#include <atomic>

// application
#include "require.hpp"

/******************************************************************************/

struct sentry_t {
    typedef std::atomic<bool> flag_t;

    explicit sentry_t(std::atomic<bool>& flag) : flag_m(flag) {
        require(!owns_m.exchange(!flag_m.exchange(true)));
    }

    ~sentry_t() {
        if (owns_m)
            require(flag_m.exchange(false));
    }

    // You own the sentry iff this returns true. Proceeding when it is false is
    // undefined.
    explicit operator bool () const {
        return owns_m;
    }

private:
    flag_t&           flag_m;
    std::atomic<bool> owns_m{false};
};

/******************************************************************************/

struct reentrant_t {
    reentrant_t() : sentry_m(flag_m) {
    }

    bool operator()() {
        return static_cast<bool>(sentry_m);
    }

private:
    std::atomic<bool> flag_m{false};
    sentry_t          sentry_m;
};

/******************************************************************************/

#endif // reentrant_hpp__

/******************************************************************************/
