cmake_minimum_required(VERSION 3.10)

# set the project name
project("Itho WPU")

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(CURL REQUIRED)
# find_package(SQLite3 REQUIRED) find_package(PIGPIO REQUIRED)
find_package(Threads REQUIRED)

include_directories(${libcurl_INCLUDE_DIR})
include_directories(/usr/include)
link_directories(/usr/lib)

# add the executable
add_executable(pislave pislave.cpp datalogparser.cpp config.cpp conversion.cpp
                       counterparser.cpp)

target_link_libraries(pislave CURL::libcurl sqlite3 pigpio Threads::Threads)

install(
  TARGETS pislave
  DESTINATION lib/itho
  EXPORT itho-targets)
