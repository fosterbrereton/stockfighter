cmake_minimum_required (VERSION 2.6)

project (stockfighter)

set(GCC_COVERAGE_COMPILE_FLAGS "-std=c++11")
set(GCC_COVERAGE_LINK_FLAGS    "-lcurl -lcrypto -lssl")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")

add_definitions(-D_WEBSOCKETPP_CPP11_STL_)

file(GLOB BOOST_SRC ./boost/libs/*/src/*.cpp)

file(GLOB APP_SRC ./sources/*.cpp)

add_library(boost_sources STATIC ${BOOST_SRC})

get_filename_component(BOOST_PATH ./boost ABSOLUTE)

target_include_directories(boost_sources PUBLIC ${BOOST_PATH})

get_filename_component(WEBSOCKETPP_PATH ./websocketpp ABSOLUTE)

get_filename_component(APP_HEADERS_PATH ./headers ABSOLUTE)

include_directories(stockfighter AFTER ${WEBSOCKETPP_PATH} ${APP_HEADERS_PATH})

add_executable(stockfighter ${APP_SRC})

add_dependencies(stockfighter boost_sources)

target_link_libraries(stockfighter PUBLIC boost_sources)
