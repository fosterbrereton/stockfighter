/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef stock_fwd_hpp__
#define stock_fwd_hpp__

/******************************************************************************/

// stdc++
#include <cstdint>

/******************************************************************************/

namespace stock {

/******************************************************************************/

struct holdings_t {
    std::int64_t cash_m{0};
    std::int64_t position_m{0};
    std::int64_t nav_m{0}; // net account value. (cash_m + price * position_m)
                           // where price is the last market-executed price
};

inline bool operator== (const holdings_t& x, const holdings_t& y) {
    return x.cash_m == y.cash_m && x.position_m == y.position_m;
}

inline bool operator!= (const holdings_t& x, const holdings_t& y) {
    return !(x == y);
}

/******************************************************************************/

} // namespace stock

/******************************************************************************/

#endif // stock_fwd_hpp__

/******************************************************************************/
