/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// identity
#include "curl.hpp"

// application
#include "error.hpp"

/******************************************************************************/

std::string construct_full_url(const curl_t& curl,
                               std::string   url,
                               const json_t& parameters) {
    bool first = true;

    for (const auto& param : parameters.object_items()) {
        const auto& key = param.first;
        std::string value;

        switch (param.second.type()) {
            case json_t::NUL: {
                value = "null";
                break;
            } case json_t::NUMBER: {
                value = std::to_string(param.second.number_value());
                break;
            } case json_t::BOOL: {
                value = param.second.bool_value() ? "true" : "false";
                break;
            } case json_t::STRING: {
                value = param.second.string_value();
                break;
            } default: {
                throw_error("url parameter error");
            }
        }

        if (key.empty() || value.empty())
            continue;

        url += (first ? "?" : "&") + key + "=" + curl.url_escape(value);

        first = false;
    }

    return url;
}

/******************************************************************************/
