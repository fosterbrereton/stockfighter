/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef configuration_hpp__
#define configuration_hpp__

/******************************************************************************/

// stdc++
#include <map>
#include <string>

// boost
#include <boost/filesystem/path.hpp>

/******************************************************************************/

namespace config {

/******************************************************************************/
// The primary difference between settings and preferences is that the latter
// are modified at runtime; the former are not. Some of the preferences, too,
// are written to disk when necessary to preserve state across app launches.
/******************************************************************************/

struct settings_t {
    bool                    inited_m{false}; // true after config::init is done
    boost::filesystem::path dir_m;           // path to the settings file's directory
    std::string             stem_m;          // name of the settings file sans extension
    boost::filesystem::path bin_path_m;      // path to self
    std::string             api_key_m;       // stockfighter api key
};

/******************************************************************************/

struct prefs_t {
    ~prefs_t();

    void init();

    void save();

private:
    boost::filesystem::path filepath_m;
};

/******************************************************************************/
// Uses the settings file that launched the app as the basis directory and name
// for derivative files (logs, etc.)
boost::filesystem::path derivative_file(std::string extension_etc);

/******************************************************************************/

// Returns true iff initialization was successful.
bool init(const std::string& binary_file_path,
          const std::string& settings_file_path);

/******************************************************************************/

const settings_t& settings();

/******************************************************************************/

prefs_t& prefs();

/******************************************************************************/

} // namespace config

/******************************************************************************/

#endif // configuration_hpp__

/******************************************************************************/
