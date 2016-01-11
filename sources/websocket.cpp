/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// identity
#include "websocket.hpp"

// stdc++

// boost
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

// websocketpp
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>

// application
#include "error.hpp"
#include "reentrant.hpp"
#include "switches.hpp"

/******************************************************************************/

namespace {

/******************************************************************************/

namespace ws = websocketpp;

typedef ws::client<ws::config::asio_tls_client> client_t;

namespace wsstd = ws::lib;

using wsstd::placeholders::_1;
using wsstd::placeholders::_2;
using wsstd::bind;
using wsstd::error_code;

typedef ws::config::asio_tls_client::message_type::ptr message_ptr;
typedef wsstd::shared_ptr<boost::asio::ssl::context>   context_ptr;
typedef client_t::connection_ptr                       connection_ptr;

/******************************************************************************/

boost::asio::io_service& service() {
    static boost::asio::io_service service_s;
    return service_s;
}

/******************************************************************************/

} // namespace

/******************************************************************************/

struct websocket_t::impl_t {
    impl_t () {
#if qDebugOff
        client_m.set_access_channels(ws::log::alevel::all);
        client_m.set_error_channels(ws::log::elevel::all);
#else
        client_m.set_access_channels(ws::log::alevel::none);
        client_m.set_error_channels(ws::log::elevel::none);
#endif

        // Initialize ASIO
        client_m.init_asio(&service());

        // Register our handlers
        client_m.set_tls_init_handler(bind(&impl_t::on_tls_init, this, ::_1));

        client_m.set_open_handler(bind(&impl_t::on_open, this, ::_1));
        client_m.set_close_handler(bind(&impl_t::on_close, this, ::_1));
        client_m.set_fail_handler(bind(&impl_t::on_fail, this, ::_1));
        client_m.set_ping_handler(bind(&impl_t::on_ping, this, ::_1, ::_2));
        client_m.set_pong_handler(bind(&impl_t::on_pong, this, ::_1, ::_2));
        client_m.set_pong_timeout_handler(bind(&impl_t::on_pong_timeout, this, ::_1, ::_2));
        client_m.set_interrupt_handler(bind(&impl_t::on_interrupt, this, ::_1));
        client_m.set_http_handler(bind(&impl_t::on_http, this, ::_1));
        client_m.set_validate_handler(bind(&impl_t::on_validate, this, ::_1));
        client_m.set_message_handler(bind(&impl_t::on_message, this, ::_1, ::_2));
    }

    void connect(const std::string& uri) {
        error_code ec;

        connection_m = client_m.get_connection(uri, ec);

        if (ec)
            throw_error(ec.message());

        client_m.connect(connection_m);
    }

    bool connected() const {
        ws::session::state::value state = connection_m->get_state();

        return state == ws::session::state::connecting ||
               state == ws::session::state::open;
    }

    void poll() {
        static std::atomic<bool> sentry_flag_s{false};
        sentry_t                 sentry{sentry_flag_s};

        if (!sentry) {
            return;
        }

        service().poll();
    }

    void disconnect() {
        client_m.close(connection_m->get_handle(),
                       ws::close::status::going_away,
                       "");

        connection_m->terminate(error_code());
    }

    void send_message(const std::string& message) {
        client_m.send(connection_m->get_handle(),
                      message,
                      ws::frame::opcode::text);
    }

    open_handler_t         open_handler_m;
    close_handler_t        close_handler_m;
    fail_handler_t         fail_handler_m;
    interrupt_handler_t    interrupt_handler_m;
    ping_handler_t         ping_handler_m;
    pong_handler_t         pong_handler_m;
    pong_timeout_handler_t pong_timeout_handler_m;
    validate_handler_t     validate_handler_m;
    http_handler_t         http_handler_m;
    message_handler_t      message_handler_m;

  private:
    impl_t(const impl_t&) = delete;
    impl_t(impl_t&&) = delete;
    impl_t& operator=(const impl_t&) = delete;
    impl_t& operator=(impl_t&&) = delete;

    context_ptr on_tls_init(ws::connection_hdl hdl) {
        context_ptr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));

        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);

        return ctx;
    }

    template <typename F, typename ... Args>
    auto do_handler(F& handler, Args&& ... args) -> decltype(handler(std::forward<Args>(args)...)) {
        typedef decltype(handler(std::forward<Args>(args)...)) result_type;

        if (handler) {
            try {
                return handler(std::forward<Args>(args)...);
            }
            catch (...) {
                // No better handling here?
            }
        }

        return result_type();
    }

    void on_open(ws::connection_hdl) {
        do_handler(open_handler_m);
    }

    void on_close(ws::connection_hdl) {
        do_handler(close_handler_m);
    }

    void on_fail(ws::connection_hdl) {
        do_handler(fail_handler_m);
    }

    void on_interrupt(ws::connection_hdl) {
        do_handler(interrupt_handler_m);
    }

    bool on_ping(ws::connection_hdl, std::string str) {
        return ping_handler_m ? do_handler(ping_handler_m, str) : true;
    }

    void on_pong(ws::connection_hdl, std::string str) {
        do_handler(pong_handler_m, str);
    }

    void on_pong_timeout(ws::connection_hdl, std::string str) {
        do_handler(pong_timeout_handler_m, str);
    }

    bool on_validate(ws::connection_hdl) {
        return validate_handler_m ? do_handler(validate_handler_m) : true;
    }

    void on_http(ws::connection_hdl) {
        do_handler(http_handler_m);
    }

    void on_message(ws::connection_hdl hdl, message_ptr msg) {
        do_handler(message_handler_m, msg->get_payload());
    }

    client_t       client_m;
    connection_ptr connection_m;
};

/******************************************************************************/

websocket_t::websocket_t() : impl_m(new websocket_t::impl_t) {
}

/******************************************************************************/

void websocket_t::handle_message(message_handler_t handler) {
    impl_m->message_handler_m = std::move(handler);
}

/******************************************************************************/

void websocket_t::handle_open(open_handler_t handler) {
    impl_m->open_handler_m = std::move(handler);
}

/******************************************************************************/

void websocket_t::handle_close(close_handler_t handler) {
    impl_m->close_handler_m = std::move(handler);
}

/******************************************************************************/

void websocket_t::handle_fail(fail_handler_t handler) {
    impl_m->fail_handler_m = std::move(handler);
}

/******************************************************************************/

void websocket_t::handle_interrupt(interrupt_handler_t handler) {
    impl_m->interrupt_handler_m = std::move(handler);
}

/******************************************************************************/

void websocket_t::handle_ping(ping_handler_t handler) {
    impl_m->ping_handler_m = std::move(handler);
}

/******************************************************************************/

void websocket_t::handle_pong(pong_handler_t handler) {
    impl_m->pong_handler_m = std::move(handler);
}

/******************************************************************************/

void websocket_t::handle_pong_timeout(pong_timeout_handler_t handler) {
    impl_m->pong_timeout_handler_m = std::move(handler);
}

/******************************************************************************/

void websocket_t::handle_validate(validate_handler_t handler) {
    impl_m->validate_handler_m = std::move(handler);
}

/******************************************************************************/

void websocket_t::handle_http(http_handler_t handler) {
    impl_m->http_handler_m = std::move(handler);
}

/******************************************************************************/

void websocket_t::send_message(const std::string& message) {
    impl_m->send_message(message);
}

/******************************************************************************/

void websocket_t::connect(const std::string& uri) {
    impl_m->connect(uri);
}

/******************************************************************************/

bool websocket_t::connected() const {
    return impl_m->connected();
}

/******************************************************************************/

void websocket_t::poll() {
    impl_m->poll();
}

/******************************************************************************/

void websocket_t::disconnect() {
    impl_m->disconnect();
}

/******************************************************************************/
