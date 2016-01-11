/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// identity
#include "console.hpp"

// application
#include "error.hpp"
#include "stock.hpp"
#include "str.hpp"

/******************************************************************************/

namespace {

/******************************************************************************/

void handle_line(std::string      line,
                 log_t&           log,
                 recur::engine_t& recur,
                 task_queue_t&    queue,
                 game_t&          game) {
    std::string command = str::pop_front(line);

    if (command == "q") {
        std::cout << game.quote() << '\n';
    } else if (command == "h") {
        stock::holdings_t holdings = game.holdings();

        std::cout << " : CASH : " << str::to_money(holdings.cash_m)
                  << " : POS : " << holdings.position_m
                  << " : NAV : " << str::to_money(holdings.nav_m)
                  << '\n';
    } else if (command == "b") {
        std::size_t qty = std::stoul(str::pop_front(line));
        std::size_t price = std::stoul(str::pop_front(line));

        game.buy(qty, price);
    } else if (command == "s") {
        std::size_t qty = std::stoul(str::pop_front(line));
        std::size_t price = std::stoul(str::pop_front(line));

        game.sell(qty, price);
    } else if (command == "quit") {
        std::cout << "Bye!\n";

        recur.terminate();
    } else if (command == "stop") {
        std::size_t id = game.instance_id();

        std::cout << "Stopping " << id << "...\n";

        if (stock::engine_t::stop(id)["ok"].bool_value()) {
            recur.terminate();
        }
    } else {
        std::cout << "Huh?\n";
    }
}

/******************************************************************************/

} // namespace

/******************************************************************************/

void console(log_t&           log,
             recur::engine_t& recur,
             task_queue_t&    queue,
             game_t&          game) {
#if qMac
    pthread_setname_np("console");
#endif

    log("COUT") << "Initiated";

    while (!recur.done()) {
        try {
            std::cout << "?> ";

            std::string line;

            std::getline(std::cin, line);

            queue.push(std::bind(handle_line,
                                 std::move(line), // moving into a block fixed in c++14
                                 std::ref(log),
                                 std::ref(recur),
                                 std::ref(queue),
                                 std::ref(game)));
        } catch (const std::exception& error) {
            std::cout << "Error : " << error.what() << '\n';
        } catch (...) {
            std::cout << "Error : unknown\n";
        }
    }

    log("COUT") << "Terminated";
}

/******************************************************************************/
