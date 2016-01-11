/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef game_hpp__
#define game_hpp__

/******************************************************************************/

// stdc++
#include <memory>

// application
#include "log.hpp"
#include "recurrent.hpp"
#include "stock_fwd.hpp"
#include "task_queue.hpp"

/******************************************************************************/

struct game_t {
    game_t(log_t& log, recur::engine_t& recur, task_queue_t& queue);

    void start();

    std::string       quote();
    stock::holdings_t holdings();
    void              buy(std::size_t qty, std::size_t price);
    void              sell(std::size_t qty, std::size_t price);
    std::size_t       instance_id() const;

private:
    game_t(const game_t&) = delete;
    game_t(game_t&&) = delete;
    game_t& operator=(const game_t&) = delete;
    game_t& operator=(game_t&&) = delete;

    struct impl_t;

    std::shared_ptr<impl_t> impl_m;
};

/******************************************************************************/

#endif // game_hpp__

/******************************************************************************/
