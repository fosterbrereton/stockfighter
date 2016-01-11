/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// identity
#include "str.hpp"

// stdc++
#include <vector>

/******************************************************************************/

namespace str {

/******************************************************************************/

std::string tolower(const std::string& src) {
    std::string result(src);

    std::transform(begin(result), end(result), begin(result),
                   [](char c) {
                       return std::tolower(c);
                   });

    return result;
}

/******************************************************************************/

std::string toupper(const std::string& src) {
    std::string result(src);

    std::transform(begin(result), end(result), begin(result),
                   [](char c) {
                       return std::toupper(c);
                   });

    return result;
}

/******************************************************************************/

std::string pop_front(std::string& src) {
    std::string result(src);
    std::size_t pop_count(0);

    for (; pop_count < src.size(); ++pop_count)
        if (std::isspace(src[pop_count]))
            break;

    result.erase(pop_count, std::string::npos);

    if (pop_count == src.size()) {
        src.clear();
    } else {
        src.erase(0, pop_count + 1);
    }

    return result;
}

/******************************************************************************/

std::string to_money(std::int64_t value) {
    std::int64_t dollars{value / 100};
    std::int64_t cents{value % 100};

    if (value < 0) {
        return "($" + std::to_string(-dollars) + "." + std::to_string(-cents) + ")";
    }

    return "$" + std::to_string(dollars) + "." + std::to_string(cents);
}


/******************************************************************************/

} // namespace str

/******************************************************************************/
