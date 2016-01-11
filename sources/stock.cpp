/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// identity
#include "stock.hpp"

//stdc++
#include <iostream>
#include <fstream>

// application
#include "configuration.hpp"
#include "curl.hpp"
#include "reentrant.hpp"
#include "require.hpp"

/******************************************************************************/

namespace {

/******************************************************************************/

static const std::string api_url_k("https://api.stockfighter.io/ob/api/");

/******************************************************************************/

json_t api_perform(curl_t& curl, bool validate) {
    curl.set_header("X-Starfighter-Authorization:" + config::settings().api_key_m);

    const std::string& result = curl.perform();

    // HTTP code 204 is no content.
    if (curl.response_code() == 204 && result.empty()) {
        return json_t();
    }

    json_t json;

    if (result[0] == '<') {
        // We didn't get json - it's likely html. Fake an error.
        json = json_t::object {
                   { "ok", false },
                   { "error", result }
               };
    } else {
        json = parse_json(result);
    }

    if (validate) {
        stock::error_check(json);
    }

    return json;
}

/******************************************************************************/

json_t api_get(const std::string& api,
               bool               validate = true) {
    curl_t curl;

    curl.set_url(api);

    return api_perform(curl, validate);
}

/******************************************************************************/

json_t api_post(const std::string& api,
                const json_t&      parameters = json_t(),
                bool               validate = true) {
    curl_t curl;

    curl.set_url(api);
    curl.set_post();
    curl.set_post_data(parameters.dump());

    return api_perform(curl, validate);
}

/******************************************************************************/

stock::order_type_t order_type_cast(const std::string& type) {
    stock::order_type_t result{stock::order_type_t::limit};

    if (type == "market") {
        result = stock::order_type_t::market;
    } else if (type == "fill-or-kill") {
        result = stock::order_type_t::fok;
    } else if (type == "immediate-or-cancel") {
        result = stock::order_type_t::ioc;
    }

    return result;
}

/******************************************************************************/

const char* order_type_cast(stock::order_type_t type) {
    switch (type) {
        case stock::order_type_t::limit: return "limit";
        case stock::order_type_t::market: return "market";
        case stock::order_type_t::fok: return "fill-or-kill";
        case stock::order_type_t::ioc: return "immediate-or-cancel";
        default: throw_error("unknown order type");
    }
}

/******************************************************************************/

stock::direction_t direction_cast(const std::string& direction) {
    stock::direction_t result{stock::direction_t::buy};

    if (direction == "sell") {
        result = stock::direction_t::sell;
    }

    return result;
}

/******************************************************************************/

const char* direction_cast(stock::direction_t type) {
    switch (type) {
        case stock::direction_t::buy: return "buy";
        case stock::direction_t::sell: return "sell";
        default: throw_error("unknown direction");
    }
}

/******************************************************************************/

void update_holding(stock::holdings_t& holdings, const stock::order_t& order) {
    if (order.direction_m == stock::direction_t::buy) {
        holdings.position_m += order.total_filled_m;
        holdings.cash_m -= order.cash_value();
    } else {
        holdings.position_m -= order.total_filled_m;
        holdings.cash_m += order.cash_value();
    }
}

/******************************************************************************/

void update_holding(stock::holdings_t& holdings, const stock::ticker_t& quote) {
    holdings.nav_m = holdings.cash_m + (holdings.position_m * quote.last_m);
}

/******************************************************************************/

} // namespace

/******************************************************************************/
#if 0
#pragma mark -
#endif
/******************************************************************************/

namespace stock {

/******************************************************************************/

bool heartbeat() {
    api_get(api_url_k + "heartbeat");

    return true;
}

/******************************************************************************/

void error_check(const json_t& json) {
    const std::string& error = json["error"].string_value();

    if (!json["ok"].bool_value() || !error.empty()) {
        throw_error(!error.empty() ? error : "unknown error");
    }
}

/******************************************************************************/

fill_t make_fill(const json_t& json) {
    fill_t fill;

    fill.price_m = json["price"].int_value();
    fill.quantity_m = json["qty"].int_value();
    fill.ts_m = json["ts"].string_value();

    return fill;
}

/******************************************************************************/

order_book_t::value_type make_order(const json_t& json) {
    order_key_t key;
    order_t     order;

    key.first = json["venue"].string_value();
    key.second = json["id"].int_value();

    order.account_m = json["account"].string_value();
    order.direction_m = direction_cast(json["direction"].string_value());
    order.open_m = json["open"].bool_value();
    order.type_m = order_type_cast(json["orderType"].string_value());
    order.original_quantity_m = json["originalQty"].int_value();
    order.price_m = json["price"].int_value();
    order.quantity_m = json["qty"].int_value();
    order.symbol_m = json["symbol"].string_value();
    order.total_filled_m = json["totalFilled"].int_value();
    order.timestamp_m = json["ts"].string_value();

    for (const auto& fill : json["fills"].array_items()) {
        order.fills_m.push_back(make_fill(fill));
    }

    return order_book_t::value_type{std::move(key), std::move(order)};
}

/******************************************************************************/

execution_t make_execution(const json_t& json) {
    execution_t result;

    result.order_m = make_order(json["order"]).second;
    result.account_m = json["account"].string_value();
    result.venue_m = json["venue"].string_value();
    result.symbol_m = json["symbol"].string_value();
    result.standing_id_m = json["standingId"].int_value();
    result.incoming_id_m = json["incomingId"].int_value();
    result.price_m = json["price"].int_value();
    result.filled_m = json["filled"].int_value();
    result.filled_at_m = json["filledAt"].string_value();
    result.standing_complete_m = json["standingComplete"].bool_value();
    result.incoming_complete_m = json["incomingComplete"].bool_value();

    return result;
}

/******************************************************************************/

void dump_instructions(const std::string& level_name,
                       const json_t&      instructions) {
    for (const auto& instruction : instructions.object_items()) {
        std::string file_name = instruction.first;

        std::transform(begin(file_name), end(file_name), begin(file_name),
                       [](char c) {
                           return std::isspace(c) ? '_' : std::tolower(c);
                       });

        auto path = config::derivative_file("_" + level_name +
                                            "_" + file_name +
                                            ".md");

        std::ofstream output(path.string());

        output << instruction.second.string_value();
    }
}

/******************************************************************************/
#if 0
#pragma mark -
#endif
/******************************************************************************/

std::size_t order_t::cash_value() const {
    std::size_t result{0};

    for (const auto& fill : fills_m) {
        result += fill.quantity_m * fill.price_m;
    }

    return result;
}

/******************************************************************************/
#if 0
#pragma mark -
#endif
/******************************************************************************/

void engine_t::start(const std::string& level_name) {
    json_t json = api_post("https://www.stockfighter.io/gm/levels/" + level_name);

    account_m = json["account"].string_value();
    seconds_per_day_m = json["secondsPerTradingDay"].int_value();
    id_m = json["instanceId"].int_value();

    for (const auto& symbol : json["tickers"].array_items()) {
        stock_symbols_m.push_back(symbol.string_value());
    }

    for (const auto& symbol : json["venues"].array_items()) {
        venue_symbols_m.push_back(symbol.string_value());
    }

    dump_instructions(level_name, json["instructions"]);

    require(!venue_symbols_m.empty());

    require(!stock_symbols_m.empty());
}

/******************************************************************************/

void engine_t::refresh() {
    static std::atomic<bool> sentry_flag_s{false};
    sentry_t                 sentry{sentry_flag_s};

    if (!sentry) {
        return;
    }

    // REVISIT : Maybe add a timeout to this specific API call?
    json_t json = api_get(world_api(id_m));

    done_m = json["done"].bool_value();
    state_m = json["state"].string_value();
    flash_m = json["flash"];
    last_day_m = json["details"]["endOfTheWorldDay"].int_value();

    if (today_m.exchange(json["details"]["tradingDay"].int_value()) >= 0 &&
        !ready_m.exchange(true)) {
        world_ready_m.notify_all();
    }
}

/******************************************************************************/

json_t engine_t::restart(std::size_t id) {
    return api_post(world_api(id) + "/restart");
}

/******************************************************************************/

json_t engine_t::stop(std::size_t id) {
    return api_post(world_api(id) + "/stop");
}

/******************************************************************************/

json_t engine_t::resume(std::size_t id) {
    return api_post(world_api(id) + "/resume");
}

/******************************************************************************/

std::string engine_t::world_api(std::size_t id) {
    return "https://www.stockfighter.io/gm/instances/" + std::to_string(id);
}

/******************************************************************************/

void engine_t::world_wide_wait() {
    if (ready_m) {
        return;
    }

    lock_t lock(world_mutex_s);

    world_ready_m.wait(lock, [=]() -> bool { return !done_m && ready_m; });
}

/******************************************************************************/

const std::string& engine_t::venue() const {
    return venue_symbols_m.front();
}

/******************************************************************************/

const std::string& engine_t::symbol() const {
    return stock_symbols_m.front();
}

/******************************************************************************/

bool engine_t::update_ticker(const ticker_t& new_ticker_data,
                             ticker_t&       old_ticker,
                             ticker_t&       cur_ticker) {
    lock_t lock{quote_mutex_m};

    if (quote_m.quote_time_m > new_ticker_data.quote_time_m) {
        return false;
    }

    old_ticker = quote_m;

    if (new_ticker_data.bid_m) {
        quote_m.bid_m = new_ticker_data.bid_m;
        quote_m.bid_size_m = new_ticker_data.bid_size_m;
        quote_m.bid_depth_m = new_ticker_data.bid_depth_m;
    }

    if (new_ticker_data.ask_m) {
        quote_m.ask_m = new_ticker_data.ask_m;
        quote_m.ask_size_m = new_ticker_data.ask_size_m;
        quote_m.ask_depth_m = new_ticker_data.ask_depth_m;
    }

    if (new_ticker_data.last_m) {
        quote_m.last_m = new_ticker_data.last_m;
        quote_m.last_size_m = new_ticker_data.last_size_m;
        quote_m.last_trade_m = new_ticker_data.last_trade_m;
    }

    quote_m.quote_time_m = new_ticker_data.quote_time_m;

    cur_ticker = quote_m;

    return true;
}

/******************************************************************************/

ticker_t engine_t::quote() const {
    lock_t lock{quote_mutex_m};

    return quote_m;
}

/******************************************************************************/

void engine_t::update_position(const order_key_t& key,
                               const execution_t& execution) {
    // There should be a lot of state validation that happens here.

    lock_t lock{book_mutex_m};

    book_m[key] = execution.order_m;
}

/******************************************************************************/

bool engine_t::own_order(const order_key_t& key) const {
    lock_t lock{book_mutex_m};

    return book_m.find(key) != book_m.end();
}

/******************************************************************************/

holdings_t engine_t::holdings() {
    holdings_t result;

    /* book lock scope */ {
        lock_t lock{book_mutex_m};

        for (const auto& order : book_m) {
            update_holding(result, order.second);
        }
    }

    /* quote lock scope */ {
        lock_t lock{quote_mutex_m};

        update_holding(result, quote_m);
    }

    return result;
}

/******************************************************************************/

order_book_t::value_type engine_t::order(std::size_t  price,
                                         std::size_t  quantity,
                                         order_type_t type,
                                         direction_t  direction) {
    const std::string& venue = this->venue();
    const std::string& symbol = this->symbol();

    json_t parameters = json_t::object {
        { "account", account_m },
        { "venue", venue },
        { "stock", symbol },
        { "price", static_cast<int>(price) },
        { "qty", static_cast<int>(quantity) },
        { "direction", direction_cast(direction) },
        { "orderType", order_type_cast(type) }
    };

    json_t                   json{api_post("https://api.stockfighter.io/ob/api/venues/" +
                                               venue +
                                               "/stocks/" +
                                               symbol +
                                               "/orders",
                                           parameters)};
    order_book_t::value_type order = make_order(json);

    require(order.first.first == venue);
    require(order.second.symbol_m == symbol);
    require(order.second.account_m == account_m);

    if (type == order_type_t::limit || type == order_type_t::market) {
        require(order.second.quantity_m + order.second.total_filled_m ==
                    order.second.original_quantity_m);
    }

    require(order.second.original_quantity_m == quantity);
    require(order.second.type_m == type);
    require(order.second.direction_m == direction);

    lock_t lock{book_mutex_m};
    book_m.emplace(order);

    return order;
}

/******************************************************************************/

order_book_t::value_type engine_t::buy(std::size_t  price,
                                       std::size_t  quantity,
                                       order_type_t type) {
    return order(price, quantity, type, direction_t::buy);
}

/******************************************************************************/

order_book_t::value_type engine_t::sell(std::size_t  price,
                                        std::size_t  quantity,
                                        order_type_t type) {
    return order(price, quantity, type, direction_t::sell);
}

/******************************************************************************/

json_t engine_t::cancel_nothrow(std::size_t order_id) {
    return api_post("https://api.stockfighter.io/ob/api/venues/" +
                        venue() +
                        "/stocks/" +
                        symbol() +
                        "/orders/" +
                        std::to_string(order_id) +
                        "/cancel",
                    json_t(),
                    false);
}

/******************************************************************************/

void engine_t::cancel(std::size_t order_id) {
    error_check(cancel_nothrow(order_id));
}

/******************************************************************************/

} // namespace stock

/******************************************************************************/
