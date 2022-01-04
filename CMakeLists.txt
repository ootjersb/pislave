cmake_minimum_required(VERSION 3.10)

# set the project name
project("Itho WPU")

set(CMAKE_CC_FLAGS "-Werror -Wno-psabi -Wall -Wextra -O3")
set(CMAKE_CXX_FLAGS "-Werror -Wno-psabi -Wall -Wextra -O3 -fPIC")

set(CMAKE_BUILD_TYPE Release)

add_subdirectory(pi_slave)
add_subdirectory(pi_master)

install(EXPORT itho-targets DESTINATION lib/itho)
