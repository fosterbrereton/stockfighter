/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef stockfighter_hpp__
#define stockfighter_hpp__

/******************************************************************************/

// stdc++
#include <string>
#include <map>
#include <vector>
#include <mutex>

// application
#include "json.hpp"
#include "stock_fwd.hpp"

/******************************************************************************/

namespace stock {

/******************************************************************************/

typedef std::mutex                mutex_t;
typedef std::unique_lock<mutex_t> lock_t;

/******************************************************************************/

bool heartbeat();

/******************************************************************************/

void error_check(const json_t& json);

/******************************************************************************/

typedef std::string                 symbol_t;
typedef symbol_t                    stock_symbol_t;
typedef symbol_t                    venue_symbol_t;
typedef std::vector<stock_symbol_t> stock_symbols_t;
typedef std::vector<venue_symbol_t> venue_symbols_t;

/******************************************************************************/

struct ticker_t {
    // These are the folks looking to BUY the stock
    std::size_t bid_m;        // best price currently bid for the stock
    std::size_t bid_size_m;   // aggregate size of all orders at the best bid
    std::size_t bid_depth_m;  // aggregate size of *all bids*

    // These are the folks looking to SELL the stock
    std::size_t ask_m;        // best price currently offered for the stock
    std::size_t ask_size_m;   // aggregate size of all orders at the best ask
    std::size_t ask_depth_m;  // aggregate size of *all asks*

    std::size_t last_m;       // price of last trade
    std::size_t last_size_m;  // quantity of last trade

    std::string last_trade_m; // timestamp of last trade
    std::string quote_time_m; // server ts of quote generation
};

enum class order_type_t {
    // Limit
    //
    // The most common order: designed to a) immediately match any orders on
    // the book which offer prices "as good or better as the ones listed in the
    // order" and b) have any unmatched portion of the order then rest on the
    // book. It will stay there, patiently waiting for another trader to send
    // in an order which matches with it.
    //
    // Stockfighter limit orders are "good until canceled" -- we do not
    // automatically expire them for you.
    limit,

    // Market
    //
    // The preferred order type for suckers! Seriously, never, ever put in a
    // market order.
    //
    // A market order executes immediately against any orders in the book
    // regardless of their price. This can very easily have pathological
    // behavior if you judge the state of the book poorly: for example, if you
    // see an ask on the book offering 100 shares for $50.00 apiece and you put
    // in a market order for 100 shares expecting to pay $5,000, you can very
    // easily find that ask withdrawn in the time it takes your order to get to
    // the exchange and the actual price of the order to be $10,000 or more!
    //
    // The primary use of market orders is making note of who sends them to the
    // market. Are they dumb money which is easy to predate... or just trying
    // very hard to look like dumb money?
    market,

    // Fill-or-Kill
    //
    // A limit order which has a special rule: it is for immediate execution on
    // an all-or-nothing basis. i.e. If you request to buy 100 shares at $50
    // each and only 80 shares are available at or below that price, your order
    // will be accepted ("ok": true) but it will not receive any fills and it
    // will be immediately canceled ("open": false).
    fok,

    // Immediate-Or-Cancel
    //
    // Like a fill-or-kill order, except while it executes immediately, it
    // accepts partial execution. In the above example, it would fill against
    // the 80 shares and have the remaining 20 shares of the order immediately
    // canceled.
    ioc
};

enum class direction_t {
    buy,
    sell
};

struct fill_t {
    std::size_t price_m;
    std::size_t quantity_m;
    std::string ts_m;
};

typedef std::vector<fill_t> fills_t;

fill_t make_fill(const json_t& json);

struct order_t {
    bool         open_m{false};
    bool         complete_m{false};
    std::string  account_m;
    std::string  symbol_m;
    direction_t  direction_m;
    order_type_t type_m;
    fills_t      fills_m; // may have zero or multiple fills
    std::size_t  original_quantity_m{0}; // requested quantity
    std::size_t  price_m{0}; // the price on the order (which may not match the fills)
    std::size_t  quantity_m{0}; // unfulfilled quantity
    std::size_t  total_filled_m{0}; // fulfilled quantity
    std::string  timestamp_m; // time when order was received

    std::size_t cash_value() const; // always positive
};

// order_key_t is the venue symbol and the originating order id, which is
// guaranteed to be unique on this venue.
typedef std::pair<std::string, std::size_t> order_key_t;
typedef std::map<order_key_t, order_t>      order_book_t;

order_book_t::value_type make_order(const json_t& json);

struct execution_t {
    order_t     order_m;
    std::string account_m;
    std::string venue_m;
    std::string symbol_m;
    std::size_t standing_id_m{0};
    std::size_t incoming_id_m{0};
    std::size_t price_m{0};
    std::size_t filled_m{0};
    std::string filled_at_m{0};
    bool        standing_complete_m{false};
    bool        incoming_complete_m{false};
};

execution_t make_execution(const json_t& json);

struct engine_t {
    // instance related
    void start(const std::string& level_name); // initialize a new world instance on the service
    void refresh(); // re-grab the state of the world from the service
    static json_t restart(std::size_t id);
    static json_t stop(std::size_t id);
    static json_t resume(std::size_t id);
    void world_wide_wait(); // blocks until refresh() (called asynchronously) reports nonzero state

    const std::string& venue() const;
    const std::string& symbol() const;

    // ticker apis. Returns true iff the ticker was updated.
    bool update_ticker(const ticker_t& new_ticker_data,
                       ticker_t&       old_ticker,
                       ticker_t&       cur_ticker);

    ticker_t quote() const; // copy because threadsafe

    // orderbook apis. All block while accessing the book.
    void                     update_position(const order_key_t& key,
                                             const execution_t& execution);
    bool                     own_order(const order_key_t& key) const; // O(log n)
    holdings_t               holdings();
    order_book_t::value_type buy(std::size_t  price,
                                 std::size_t  quantity,
                                 order_type_t type);
    order_book_t::value_type sell(std::size_t  price,
                                  std::size_t  quantity,
                                  order_type_t type);

    // nonblocking.
    json_t cancel_nothrow(std::size_t order_id);
    void   cancel(std::size_t order_id);

    // instance related
    std::string               state_m;
    std::int32_t              last_day_m{0};
    std::atomic<std::int32_t> today_m{0};
    json_t                    flash_m;
    std::size_t               seconds_per_day_m;
    std::size_t               id_m{0};
    std::string               account_m;

private:
    static std::string world_api(std::size_t id);

    order_book_t::value_type order(std::size_t  price,
                                   std::size_t  quantity,
                                   order_type_t type,
                                   direction_t  direction);

    stock_symbols_t          stock_symbols_m;
    venue_symbols_t          venue_symbols_m;
    mutex_t                  world_mutex_s;
    std::condition_variable  world_ready_m;
    bool                     done_m{false};
    std::atomic<bool>        ready_m{false};
    ticker_t                 quote_m;
    mutable mutex_t          quote_mutex_m;
    order_book_t             book_m;
    mutable mutex_t          book_mutex_m;
};

/******************************************************************************/

} // namespace stock

/******************************************************************************/

#endif // stockfighter_hpp__

/******************************************************************************/
