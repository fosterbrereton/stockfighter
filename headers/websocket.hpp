/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef websocket_hpp__
#define websocket_hpp__

/******************************************************************************/

#include <string>
#include <functional>
#include <memory>

/******************************************************************************/

struct websocket_t {
    typedef std::function<void ()>                   open_handler_t;
    typedef std::function<void ()>                   close_handler_t;
    typedef std::function<void ()>                   fail_handler_t;
    typedef std::function<void ()>                   interrupt_handler_t;
    typedef std::function<bool (const std::string&)> ping_handler_t;
    typedef std::function<void (const std::string&)> pong_handler_t;
    typedef std::function<void (const std::string&)> pong_timeout_handler_t;
    typedef std::function<bool ()>                   validate_handler_t;
    typedef std::function<void ()>                   http_handler_t;
    typedef std::function<void (const std::string&)> message_handler_t;

    websocket_t();

    void handle_message(message_handler_t handler);
    void handle_open(open_handler_t handler);
    void handle_close(close_handler_t handler);
    void handle_fail(fail_handler_t handler);
    void handle_interrupt(interrupt_handler_t handler);
    void handle_ping(ping_handler_t handler);
    void handle_pong(pong_handler_t handler);
    void handle_pong_timeout(pong_timeout_handler_t handler);
    void handle_validate(validate_handler_t handler);
    void handle_http(http_handler_t handler);

    void send_message(const std::string& message);

    void connect(const std::string& uri);

    bool connected() const;

    void poll();

    void disconnect();

private:
    websocket_t(const websocket_t&) = delete;
    websocket_t(websocket_t&&) = delete;
    websocket_t& operator=(const websocket_t&) = delete;
    websocket_t& operator=(websocket_t&&) = delete;

    struct impl_t;

    std::shared_ptr<impl_t> impl_m;
};

typedef std::shared_ptr<websocket_t> shared_websocket_t;
typedef std::weak_ptr<websocket_t>   weak_websocket_t;

/******************************************************************************/

#endif // websocket_hpp__

/******************************************************************************/
