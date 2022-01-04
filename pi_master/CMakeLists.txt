cmake_minimum_required(VERSION 3.10)

# add the executable
add_executable(pimaster pimaster2.c)

target_link_libraries(pimaster)

install(
  TARGETS pimaster
  DESTINATION lib/itho
  EXPORT itho-targets)
