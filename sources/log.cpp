/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// identity
#include "log.hpp"

// stdc++
#include <chrono>
#include <iomanip>

// boost
#include <boost/filesystem.hpp>

// application
#include "shell.hpp"
#include "require.hpp"

/******************************************************************************/

log_helper_t::log_helper_t(log_t* log) : log_m(log) {
}

/******************************************************************************/

log_helper_t::~log_helper_t() {
    log_m->commit(log_m->flush_line());
}

/******************************************************************************/

constexpr auto bin_k = std::ios::binary;
constexpr auto appbin_k = std::ios::app | std::ios::binary;

log_t::log_t(boost::filesystem::path path, bool append, bool timestamped) :
    path_m{std::move(path)},
    file_m{path_m, append ? appbin_k : bin_k},
    timestamped_m{timestamped} {

    instance_identifier() = "MAIN";

    require(file_m.good());

    commit ("LOGG : Opened");
}

/******************************************************************************/

log_t::~log_t() {
    instance_identifier() = "MAIN";

    commit ("LOGG : Closed");
}

/******************************************************************************/

std::string log_t::tail(std::size_t lines) const {
    return shell::cmd("tail -n " + std::to_string(lines) +
                      " " + path_m.string());
}

/******************************************************************************/

std::string& log_t::instance_identifier() {
    thread_local std::string id_s{"????"};
    return id_s;
}

/******************************************************************************/

std::stringstream& log_t::line() {
    thread_local std::stringstream line_s;
    return line_s;
}

/******************************************************************************/

log_helper_t log_t::operator()() {
    return log_helper_t{this};
}

/******************************************************************************/

log_helper_t log_t::operator()(const std::string& identifier) {
    instance_identifier() = identifier;

    return (*this)();
}

/******************************************************************************/

std::string log_t::flush_line() {
    auto&       stream = line();
    std::string result = stream.str();

    stream.str(std::string());
    stream.clear();

    return result;
}

/******************************************************************************/

void log_t::commit(const std::string& line) {
    typedef std::chrono::system_clock clock_t;

    constexpr auto numerator = clock_t::period::num;
    constexpr auto denominator = clock_t::period::den;

    static_assert(numerator == 1, "unexpected numerator");

    auto now = clock_t::now();
    auto count = now.time_since_epoch().count();
    auto seconds = count / denominator;
    auto subseconds = count % denominator;

    std::lock_guard<std::mutex> lock{mutex_m};

    if (timestamped_m) {
        file_m << seconds << "." << subseconds << ",";
    }

    if (!instance_identifier().empty()) {
        file_m << instance_identifier() << " : ";
    }

    file_m << line << '\n';

    file_m.flush();
}

/******************************************************************************/

log_timer_t::~log_timer_t() {
    clock_t::time_point end = clock_t::now();
    auto                delta = end - start_m;
    auto                seconds = std::chrono::duration_cast<std::chrono::milliseconds>(delta);

    log_m() << "TIMR : " << tag_m
            << (details_m.empty() ? std::string() : " : " + details_m)
            << " : "<< (seconds.count()/1000.) << "s";
}

/******************************************************************************/
