/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef error_hpp__
#define error_hpp__

/******************************************************************************/

#include <string>

/******************************************************************************/

struct stockfighter_error : public std::runtime_error {
    template <typename T>
    stockfighter_error(T&& expression, const char* file, std::size_t line) :
        std::runtime_error(expression), // forward here?
        file_m(file),
        line_m(line) {       
    }

    std::string what_more() const {
        return what() + std::string(" (") + where() + ")";
    }

    std::string where() const {
        return std::string(file_m) + ':' + std::to_string(line_m);
    }

private:
    const char* file_m; // don't risk the allocation of a std::string
    std::size_t line_m;
};

/******************************************************************************/

#define throw_error(expression) throw_error_((expression), __FILE__, __LINE__);

template <typename T>
inline void throw_error_(T&&         expression,
                         const char* file,
                         std::size_t line) {
    throw stockfighter_error(std::forward<T>(expression), file, line);
}

/******************************************************************************/

#endif // error_hpp__

/******************************************************************************/
