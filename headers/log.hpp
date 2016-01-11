/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef log_hpp__
#define log_hpp__

/******************************************************************************/

// stdc++
#include <chrono>
#include <sstream>
#include <string>
#include <iostream>

// boost
#include <boost/filesystem/fstream.hpp>

/******************************************************************************/

struct log_t;

/******************************************************************************/

struct log_helper_t {
    explicit log_helper_t(log_t* log);

    ~log_helper_t();

    template <typename T>
    const log_helper_t& append(T&& x) const;

  private:
    log_t* log_m;
};

/******************************************************************************/

struct log_t {
    log_t(boost::filesystem::path path,
          bool                    append,
          bool                    timestamped);

    ~log_t();

    log_helper_t operator()();

    log_helper_t operator()(const std::string& identifier);

    std::string tail(std::size_t lines) const;

    std::string& instance_identifier();

  private:
    log_t(const log_t&) = delete;
    log_t(log_t&&) = delete;
    log_t& operator=(const log_t&) = delete;
    log_t& operator=(log_t&&) = delete;

    friend struct log_helper_t;

    std::stringstream& line();

    std::string flush_line();

    void commit(const std::string& line);

    boost::filesystem::path     path_m;
    boost::filesystem::ofstream file_m;
    bool                        timestamped_m{false};
    std::mutex                  mutex_m;
};

/******************************************************************************/

template <typename T>
const log_helper_t& log_helper_t::append(T&& x) const {
    log_m->line() << std::forward<T>(x);

    return *this;
}

/******************************************************************************/

template <typename T>
const log_helper_t& operator<<(const log_helper_t& helper, T&& x) {
    return helper.append(std::forward<T>(x));
}

/******************************************************************************/

struct log_timer_t {
    using clock_t = std::chrono::steady_clock;

    log_timer_t(log_t& log, const std::string& tag) :
        log_m(log),
        tag_m(tag),
        start_m(clock_t::now()) {
    }

    void set_details(const std::string& details) {
        details_m = details;
    }

    ~log_timer_t();

  private:
    log_t&              log_m;
    std::string         tag_m;
    std::string         details_m;
    clock_t::time_point start_m;
};

/******************************************************************************/

#endif // log_hpp__

/******************************************************************************/
