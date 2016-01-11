/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

#ifndef require_hpp__
#define require_hpp__

// application
#include "error.hpp"

/******************************************************************************/

#define require(expression) require_((expression), #expression, __FILE__, __LINE__);

inline void require_(bool        expression_value,
                     const char* expression,
                     const char* file,
                     std::size_t line) {
    if (expression_value)
        return;

    throw_error_(std::string("RQRE : ") + expression, file, line);
}

/******************************************************************************/

#endif // require_hpp__

/******************************************************************************/
