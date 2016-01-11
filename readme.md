# Stockfighter C++ Client

This is a framework for a Stockfighter client in C++.

## Dependencies

These must be downloaded separately and placed in their own directories in the top-level folder:

 - [Boost](http://www.boost.org/) (into `./boost`)
 - [Websocketpp](https://github.com/zaphoyd/websocketpp) (into `./websocketpp`)

[json11](https://github.com/dropbox/json11) must also be downloaded and placed into `./sources`, and `./headers` as appropriate (only `json11.hpp` and `json11.cpp` are necessary.)

There are other dependencies (libcurl, OpenSSL, etc.) that are included by MacOS and need no further setup there.

## Building

Right now the only environment set up to build is MacOS X. The `./generate.sh` file should produce the Xcode project file, which is then used to build, run, debug, etc.

The project uses CMake so getting other environments set up should be doable. (There are some thing that will need a Windows analogue, like shell command support.)

## Usage

The level is instantiated from within `game_t::impl_t::start`:

        engine_m.start("first_steps");

Once the instance is running, the bulk of the client boils down to two handlers (both in `./sources/game.cpp`):

 - `game_t::impl_t::world_reaction`. Code here responds to changes in the state of the world.
 - `game_t::impl_t::ticker_reaction`. Code here responds to changes in the ticker.

It helps to have one or more terminals tailing the logs and other output the client produces.

## Future Work

 - Windows build support
 - TravisCI support

 - libcurl: move to the nonblocking (`multi_`) APIs. There's even [an example](http://curl.haxx.se/libcurl/c/asiohiper.html) that uses `boost::asio` for socket handling.
 - task_queue: Add per-thread queues and task stealing
 - Improve exception handling, both in the task queue and the recurrent engine.
