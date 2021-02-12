cmake_minimum_required(VERSION 3.10)

# set the project name
project("Itho WPU")

SET(CMAKE_CC_FLAGS "-Werror -Wno-psabi -Wall -Wextra -O3")
SET(CMAKE_CXX_FLAGS "-Werror -Wno-psabi -Wall -Wextra -O3 -fPIC")

SET(CMAKE_BUILD_TYPE Release)

add_subdirectory (pi_slave)
add_subdirectory (pi_master)

install(EXPORT itho-targets DESTINATION lib/itho)
