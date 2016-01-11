/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// identity
#include "configuration.hpp"

// stdc++
#include <regex>
#include <iostream>

// boost
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

// application
#include "error.hpp"
#include "json.hpp"
#include "reentrant.hpp"
#include "require.hpp"
#include "shell.hpp"
#include "str.hpp"

/******************************************************************************/

namespace {

/******************************************************************************/

config::settings_t& app() {
    static config::settings_t state_s;
    return state_s;
}

/******************************************************************************/

json_t slurp_json(const boost::filesystem::path& path) {
    boost::filesystem::ifstream input{path, std::ios::in | std::ios::binary};

    if (!input)
        return json_t();

    std::string json_raw;

    input.seekg(0, std::ios::end);

    json_raw.resize(input.tellg());

    input.seekg(0, std::ios::beg);

    input.read(&json_raw[0], json_raw.size());

    return json_raw.empty() ? json_t() : parse_json(json_raw);
}

/******************************************************************************/

typedef std::mutex                   mutex_t;
typedef std::unique_lock<std::mutex> lock_t;

/******************************************************************************/

} // namespace

/******************************************************************************/

namespace config {

/******************************************************************************/

boost::filesystem::path derivative_file(std::string extension_etc) {
    const settings_t& settings = app();

    return settings.dir_m / (settings.stem_m + std::move(extension_etc));
}

/******************************************************************************/

void prefs_t::init() {
    if (app().inited_m)
        return;

    // filepath_m = derivative_file("_prefs.json");

    // json_t json = slurp_json(filepath_m);
}

/******************************************************************************/

prefs_t::~prefs_t() {
    save();
}

/******************************************************************************/

void prefs_t::save() {
    static reentrant_t reentrancy_check_s;

    if (!reentrancy_check_s())
        return;

    boost::filesystem::ofstream output{filepath_m, std::ios::out | std::ios::binary};

    if (!output)
        return;
}

/******************************************************************************/

const settings_t& settings() {
    return app();
}

/******************************************************************************/

bool init(const std::string& binary_path_str,
          const std::string& settings_path_str) {
    settings_t& settings = app();

    if (settings.inited_m)
        return settings.inited_m;

    settings.bin_path_m = binary_path_str;

    boost::filesystem::path settings_path(settings_path_str);

    settings.stem_m = settings_path.stem().string();
    settings.dir_m = settings_path.branch_path();

    json_t json = slurp_json(settings_path);

    settings.api_key_m = json["api_key"].string_value();

    prefs().init();

    settings.inited_m = true;

    return settings.inited_m;
}

/******************************************************************************/

prefs_t& prefs() {
    static prefs_t state_s;
    return state_s;
}

/******************************************************************************/

} // namespace config

/******************************************************************************/
