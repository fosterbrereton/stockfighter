/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// stc++
#include <iostream>
#include <regex>
#include <deque>

// application
//#include "bot.hpp"
#include "configuration.hpp"
#include "console.hpp"
#include "curl.hpp"
#include "error.hpp"
#include "game.hpp"
#include "json.hpp"
#include "log.hpp"
#include "recurrent.hpp"
#include "require.hpp"
#include "shell.hpp"
#include "str.hpp"
#include "stock.hpp"
#include "switches.hpp"
#include "task_queue.hpp"
#include "websocket.hpp"

/******************************************************************************/

namespace {

/******************************************************************************/

void keepalive(log_t& log, recur::engine_t& recur) {
    // If the API dies, so should we.
    if (!stock::heartbeat()) {
        log("MAIN") << "Heartbeat failure";

        recur.terminate();
    }
}

/******************************************************************************/

} // namespace

/******************************************************************************/

int main(int argc, char** argv) try {
#if qMac
    pthread_setname_np("main");
#endif

    std::string binary_path(argv[0]);
    std::string settings_path(argc > 1 ? argv[1] : "");

    if (!config::init(binary_path, settings_path)) {
        std::cout << "Usage : stockfighter /path/to/settings/file.json\n";

        return 1;
    }

    log_t           log(config::derivative_file(".log"), true, false);
    task_queue_t    queue{6};
    recur::engine_t recur{queue};
    game_t          game(log, recur, queue);

    std::thread([&](){ console(log, recur, queue, game); }).detach();

    recur.insert(std::chrono::minutes(1), [&](){ keepalive(log, recur); });

    log("MAIN") << "Startup";

    queue.push([&](){
        game.start();
    });

    recur.run();

    return 0;
} catch (const std::exception& error) {
    std::cerr << "Fatal error : " << error.what() << '\n';

    return 1;
} catch (...) {
    std::cerr << "Fatal error : Unknown" << '\n';

    return 1;
}
