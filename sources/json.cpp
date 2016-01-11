/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// identity
#include "json.hpp"

// application
#include "error.hpp"

/******************************************************************************/

json_t parse_json(const std::string& json_raw) {
    std::string json_parse_error;
    json_t      result = json_t::parse(json_raw, json_parse_error);

    if (!json_parse_error.empty())
        throw_error(json_parse_error);

    return result;
}

/******************************************************************************/
