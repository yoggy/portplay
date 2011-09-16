cmake_minimum_required(VERSION 2.8)

PROJECT(portplay)

if(WIN32)
  # nothing to do?
elseif(APPLE)
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
  set(CMAKE_CXX_FLAGS "-g -Wall")
  set(CMAKE_C_FLAGS "-g -Wall")
endif()

add_executable(portplay portplay.c)
target_link_libraries(portplay portaudio)

