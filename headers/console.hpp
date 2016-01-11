/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef console_hpp__
#define console_hpp__

/******************************************************************************/

// application
#include "game.hpp"
#include "log.hpp"
#include "recurrent.hpp"
#include "task_queue.hpp"

/******************************************************************************/

void console(log_t&           log,
             recur::engine_t& recur,
             task_queue_t&    queue,
             game_t&          game);

/******************************************************************************/

#endif // console_hpp__

/******************************************************************************/
