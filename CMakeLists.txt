cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(jsoneditor)

# Enable debug symbols by default
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()
# (you can also set it on the command line: -D CMAKE_BUILD_TYPE=Release)

set(C_SOURCES json.c)

set(CXX_SOURCES
  main.cpp
  str.cpp
  MurmurHash3.cpp
  nametable.cpp
  hashtable_test.cpp
  nametable_test.cpp
  test.cpp
)

set(SOURCES ${C_SOURCES} ${CXX_SOURCES})

add_executable(${PROJECT_NAME} ${SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 98)


if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set_property(SOURCE ${CXX_SOURCES} PROPERTY COMPILE_FLAGS --std=c++03)

  target_compile_options(${PROJECT_NAME}
    PRIVATE
    -g
    -pedantic
    -Weverything
    -Werror
    -Wno-error=unused-parameter
    -Wno-error=unused-variable
    -Wno-switch-enum
    -Wno-error=unreachable-code
    -Wno-unused-function
    -Wno-missing-prototypes
    -Wno-long-long
    -Wno-unused-macros
    -Wno-logical-op-parentheses
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-padded
    -Wno-weak-vtables
    -Wno-variadic-macros
    -Wno-old-style-cast
    )

  set_property(SOURCE MurmurHash3.cpp PROPERTY COMPILE_FLAGS "-Wno-sign-conversion -Wno-cast-align")
endif()

