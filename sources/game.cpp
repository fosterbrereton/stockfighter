/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// identity
#include "game.hpp"

//stdc++
#include <cmath>
#include <iostream>
#include <set>
#include <regex>

// application
#include "configuration.hpp"
#include "json.hpp"
#include "reentrant.hpp"
#include "require.hpp"
#include "str.hpp"
#include "stock.hpp"
#include "switches.hpp"
#include "websocket.hpp"

/******************************************************************************/

namespace {

/******************************************************************************/

template <typename T>
struct debounce {
    explicit debounce(T value = T()) : last_m{value} {
    }

    bool operator()(T value) {
        if (last_m == value) {
            return false;
        }

        last_m = std::move(value);

        return true;
    }

    operator const T& () const {
        return last_m;
    }

    const T& operator*() const {
        return last_m;
    }

    const T* operator->() const {
        return &last_m;
    }

private:
    T last_m;
};

template <typename T>
struct debounce<std::atomic<T>> {
    explicit debounce(T value = T()) : last_m{value} {
    }

    bool operator()(T value) {
        return last_m.exchange(value) != value;
    }

    operator const T& () const {
        return last_m;
    }

    const T& operator*() const {
        return last_m;
    }

    const T* operator->() const {
        return &last_m;
    }

private:
    std::atomic<T> last_m;
};

typedef debounce<std::string>              debounce_string_t;
typedef debounce<std::int64_t>             debounce_sint_t;
typedef debounce<std::uint64_t>            debounce_uint_t;
typedef debounce<json_t>                   debounce_json_t;
typedef debounce<stock::holdings_t>        debounce_holdings_t;
typedef debounce<std::atomic<std::size_t>> debounce_atomic_size_t;

/******************************************************************************/

struct gamesocket_t {
    gamesocket_t(std::string      name,
                 log_t&           log,
                 recur::engine_t& recur) :
        name_m(std::move(name)),
        log_m(log),
        recur_m(recur) {
    }

    ~gamesocket_t() {
        recur_m.erase(polling_token_m);
    }

    void handle_message(websocket_t::message_handler_t handler) {
        socket_m.handle_message(std::move(handler));
    }

    void connect(std::string uri) {
        uri_m = std::move(uri);

        socket_m.connect(uri_m);

        polling_token_m = recur_m.insert(std::chrono::milliseconds(100),
                                         [=](){ poll(); });

        socket_m.handle_open([=]() {
            log_m(name_m) << "SOCK : OPEN";
        });

        socket_m.handle_close([=]() {
            log_m(name_m) << "SOCK : CLOS";

            connect(uri_m);
        });

        socket_m.handle_fail([=]() {
            log_m(name_m) << "SOCK : FAIL";
        });

        socket_m.handle_interrupt([=]() {
            log_m(name_m) << "SOCK : INTP";
        });

        socket_m.handle_ping([=](const std::string& string) {
#if qDebugOff
            log_m(name_m) << "SOCK : PING";
#endif
            return true;
        });

        socket_m.handle_pong([=](const std::string& string) {
            log_m(name_m) << "SOCK : PONG";
        });

        socket_m.handle_pong_timeout([=](const std::string& string) {
            log_m(name_m) << "SOCK : PONG : TOUT";
        });

        socket_m.handle_validate([=]() {
            log_m(name_m) << "SOCK : VALD";

            return true;
        });

        socket_m.handle_http([=]() {
            log_m(name_m) << "SOCK : HTTP";
        });
    }

private:
    gamesocket_t(const gamesocket_t&) = delete;
    gamesocket_t(gamesocket_t&&) = delete;
    gamesocket_t& operator=(const gamesocket_t&) = delete;
    gamesocket_t& operator=(gamesocket_t&&) = delete;

    void poll() {
        if (socket_m.connected()) {
            socket_m.poll();
        }
    }

    std::string      name_m;
    log_t&           log_m;
    recur::engine_t& recur_m;
    websocket_t      socket_m;
    std::string      uri_m;
    recur::token_t   polling_token_m;
};

typedef std::shared_ptr<gamesocket_t> shared_gamesocket_t;

/******************************************************************************/

} // namespace

/******************************************************************************/

struct game_t::impl_t {
    impl_t(log_t& log, recur::engine_t& recur, task_queue_t& queue) :
        log_m(log),
        recur_m(recur),
        queue_m(queue),
        ticker_m("TCKR", log_m, recur_m),
        executions_m("EXEC", log_m, recur_m),
        exec_map_m(log_m, recur_m, engine_m) {
    }

    // external apis
    void                            start();
    std::string                     quote();
    stock::holdings_t               holdings();
    stock::order_book_t::value_type buy(std::size_t         qty,
                                        std::size_t         price,
                                        stock::order_type_t type = stock::order_type_t::ioc);
    stock::order_book_t::value_type sell(std::size_t         qty,
                                         std::size_t         price,
                                         stock::order_type_t type = stock::order_type_t::ioc);

    // internal apis - called when something in their context changes.
    void world_reaction();
    void ticker_reaction();

    // recurrent routine(s)
    void world_ping();

    // websocket handlers
    void handle_tick(const json_t& message);
    void handle_execution(const json_t& message);

    log_t&              log_m;
    recur::engine_t&    recur_m;
    task_queue_t&       queue_m;
    stock::engine_t     engine_m;
    gamesocket_t        ticker_m;
    gamesocket_t        executions_m;
    std::size_t         pingerr_m{0};
    debounce_string_t   last_state_m;
    debounce_uint_t     last_end_m;
    debounce_sint_t     last_today_m{-1};
    debounce_holdings_t last_holdings_m;
    debounce_json_t     last_flash_m;
    stock::ticker_t     last_quote_m;
    stock::ticker_t     cur_quote_m;
    task_queue_t        order_queue_m{4};
};

/******************************************************************************/

void game_t::impl_t::world_ping() try {
    log_m.instance_identifier() = engine_m.venue();

    engine_m.refresh();

    pingerr_m = 0;

    world_reaction();
} catch (const std::exception& error) {
    log_m() << "EROR " << ++pingerr_m << " : WORLD : " << error.what();

    if (pingerr_m >= 5) {
        log_m() << "EROR : WORLD : ABRT";

        recur_m.terminate();
    }
} catch (...) {
    log_m() << "EROR " << ++pingerr_m << " : WORLD : unknown";

    if (pingerr_m >= 5) {
        log_m() << "EROR : WORLD : ABRT";

        recur_m.terminate();
    }
}

/******************************************************************************/

void game_t::impl_t::handle_tick(const json_t& json) {
    log_m.instance_identifier() = engine_m.venue();

    stock::ticker_t ticker;

    const json_t& quote = json["quote"];

    ticker.bid_m = quote["bid"].int_value();
    ticker.bid_size_m = quote["bidSize"].int_value();
    ticker.bid_depth_m = quote["bidDepth"].int_value();

    ticker.ask_m = quote["ask"].int_value();
    ticker.ask_size_m = quote["askSize"].int_value();
    ticker.ask_depth_m = quote["askDepth"].int_value();

    ticker.last_m = quote["last"].int_value();
    ticker.last_size_m = quote["lastSize"].int_value();

    ticker.last_trade_m = quote["lastTrade"].string_value();
    ticker.quote_time_m = quote["quoteTime"].string_value();

    static log_t ticker_s(config::derivative_file("_ticker_raw.csv"), false, false);

    ticker_s("") << ticker.quote_time_m
                 << ',' << ticker.bid_m
                 << ',' << ticker.last_m
                 << ',' << ticker.ask_m
                 ;

    if (engine_m.update_ticker(ticker, last_quote_m, cur_quote_m)) {
        ticker_reaction();
    }
}

/******************************************************************************/

void game_t::impl_t::world_reaction() {
    if (last_state_m(engine_m.state_m)) {
        log_m() << "WORLD : STATE : " << str::toupper(last_state_m);

        if (engine_m.state_m == "won" ||
            engine_m.state_m == "lost" ||
            engine_m.state_m == "end") {
           recur_m.terminate();
        }
    }

    if (last_end_m(engine_m.last_day_m)) {
        log_m() << "WORLD : END : " << last_end_m;
    }

    std::int64_t cur_today = engine_m.today_m; // read once for thread consistency
    std::int64_t last_today = last_today_m;

    if (last_today_m(cur_today) >= 0) {
        if (cur_today != last_today) {
            log_m() << "WORLD : DAY : " << last_today_m;

            if (last_holdings_m(holdings())) {
                log_m() << "HOLD"
                        << " : CASH : " << str::to_money(last_holdings_m->cash_m)
                        << " : POS : " << last_holdings_m->position_m
                        << " : NAV : " << str::to_money(last_holdings_m->nav_m)
                        ;
            }
        }
    }

    if (last_flash_m(engine_m.flash_m)) {
        // log_m() << quote();

        for (const auto& flash : last_flash_m->object_items()) {
            log_m() << "WORLD : FLASH"
                    << " : " << str::toupper(flash.first)
                    << " : " << flash.second.string_value();
        }
    }

    // Handle further events predicated on the world state here.
}

/******************************************************************************/

void game_t::impl_t::ticker_reaction() {
    static log_t                  bla_s(config::derivative_file("_ticker_bla.csv"), false, false);
    static debounce_atomic_size_t last_bid_s;
    static debounce_atomic_size_t last_last_s;
    static debounce_atomic_size_t last_ask_s;

    bool new_bid = last_bid_s(cur_quote_m.bid_m);
    bool new_last = last_last_s(cur_quote_m.last_m);
    bool new_ask = last_ask_s(cur_quote_m.ask_m);

    if (new_bid || new_last || new_ask) {
        bla_s("") << cur_quote_m.quote_time_m << cur_quote_m.bid_m << ',' << cur_quote_m.last_m << ',' << cur_quote_m.ask_m;
    }

    // Handle further events predicated on the ticker here.
}

/******************************************************************************/

void game_t::impl_t::handle_execution(const json_t& json) {
    log_m.instance_identifier() = engine_m.venue();

    stock::execution_t              execution;
    stock::order_book_t::value_type order{stock::make_order(json["order"])};

    execution.order_m = std::move(order.second);

    engine_m.update_position(order.first, execution);

    log_m() << "FILL"
            << " : " << (execution.order_m.direction_m == stock::direction_t::buy ? "BUYY" : "SELL")
            << " : " << order.first.second
            << " : " << execution.order_m.fills_m.back().quantity_m
                     << " " << execution.order_m.symbol_m
                     << " @ " << str::to_money(execution.order_m.fills_m.back().price_m)
            << " : " << execution.order_m.total_filled_m << "/" << execution.order_m.original_quantity_m
            << " : " << execution.order_m.fills_m.back().ts_m
            ;
}

/******************************************************************************/

void game_t::impl_t::start() try {
    log_m("GAME") << "Attempting connection...";

    engine_m.start("first_steps");

    log_m() << engine_m.id_m
            << " : " << engine_m.venue()
            << " : " << engine_m.symbol()
            << " : " << engine_m.account_m;

    log_m.instance_identifier() = engine_m.venue();

    exec_map_m.venue_m = engine_m.venue();

    std::string websocket_url("https://api.stockfighter.io/ob/api/ws/" +
                              engine_m.account_m +
                              "/venues/" +
                              engine_m.venue() +
                              "/");

    ticker_m.handle_message([=](const std::string& message) {
        queue_m.push([=](){
            json_t json = parse_json(message);

            stock::error_check(json);

            handle_tick(json);
        });
    });

    ticker_m.connect(websocket_url + "tickertape");

    executions_m.handle_message([=](const std::string& message) {
        queue_m.push([=](){
            json_t json = parse_json(message);

            stock::error_check(json);

            handle_execution(json);
        });
    });

    executions_m.connect(websocket_url + "executions");

    // ping the world three times a "day", so we're relatively caught up with
    // the state of things.
    std::size_t world_ping_frequency = engine_m.seconds_per_day_m / 3. * 1000;
    recur_m.insert(std::chrono::milliseconds(world_ping_frequency),
                   [=](){ world_ping(); });

    // Wait for the world to come online.
    engine_m.world_wide_wait();
}
catch (const std::exception& error) {
    log_m() << "Error : " << error.what();

    recur_m.terminate();
}
catch (...) {
    log_m() << "Error : unknown";

    recur_m.terminate();
}

/******************************************************************************/

std::string game_t::impl_t::quote() {
    std::stringstream stream;

    stream << "QUOT"
           << " : " << cur_quote_m.bid_m << " (" << cur_quote_m.bid_size_m << ")"
           << " : " << cur_quote_m.last_m << " (" << cur_quote_m.last_size_m << ")"
           << " : " << cur_quote_m.ask_m << " (" << cur_quote_m.ask_size_m << ")";

    return stream.str();
}

/******************************************************************************/

stock::holdings_t game_t::impl_t::holdings() {
    return engine_m.holdings();
}

/******************************************************************************/

stock::order_book_t::value_type game_t::impl_t::buy(std::size_t         qty,
                                                    std::size_t         price,
                                                    stock::order_type_t type) {
    log_m.instance_identifier() = engine_m.venue();

    stock::order_book_t::value_type order = engine_m.buy(price, qty, type);

    log_m() << "ORDR : BUYY"
            << " : " << qty << " @ " << str::to_money(price)
            << " : " << order.first.second
            << " : " << order.second.total_filled_m << "/" << order.second.original_quantity_m;

    return order;
}

/******************************************************************************/

stock::order_book_t::value_type game_t::impl_t::sell(std::size_t         qty,
                                                     std::size_t         price,
                                                     stock::order_type_t type) {
    log_m.instance_identifier() = engine_m.venue();

    stock::order_book_t::value_type order = engine_m.sell(price, qty, type);

    log_m() << "ORDR : SELL"
            << " : " << qty << " @ " << str::to_money(price)
            << " : " << order.first.second
            << " : " << order.second.total_filled_m << "/" << order.second.original_quantity_m;

    return order;
}

/******************************************************************************/
#if 0
#pragma mark -
#endif
/******************************************************************************/

game_t::game_t(log_t&           log, 
               recur::engine_t& recur, 
               task_queue_t&    queue) :
    impl_m(new impl_t(log, recur, queue)) {
}

/******************************************************************************/

void game_t::start() {
    impl_m->start();
}

/******************************************************************************/

std::string game_t::quote() {
    return impl_m->quote();
}

/******************************************************************************/

stock::holdings_t game_t::holdings() {
    return impl_m->holdings();
}

/******************************************************************************/

void game_t::buy(std::size_t qty, std::size_t price) {
    impl_m->buy(qty, price);
}

/******************************************************************************/

void game_t::sell(std::size_t qty, std::size_t price) {
    impl_m->sell(qty, price);
}

/******************************************************************************/

std::size_t game_t::instance_id() const {
    return impl_m->engine_m.id_m;
}

/******************************************************************************/
