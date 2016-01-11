/******************************************************************************/
//
// Copyright 2015 Foster T. Brereton.
// See license.md in this repository for license details.
//
/******************************************************************************/

// identity
#include "shell.hpp"

// stdc++
#include <vector>

// boost

// application
#include "require.hpp"

/******************************************************************************/

namespace shell {

/******************************************************************************/

std::string cmd(const std::string& command) {
    FILE* fp = popen(command.c_str(), "r");

    require (fp);

    std::vector<char> buffer(2048, 0);
    std::string       result;

    while (fgets(&buffer[0], buffer.size(), fp))
        result += &buffer[0];

    require (pclose(fp) != -1);

    return result;
}

/******************************************************************************/

} // namespace shell

/******************************************************************************/
