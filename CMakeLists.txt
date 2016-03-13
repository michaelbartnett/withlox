cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
project(jsoneditor)

# Enable debug symbols by default
if(CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()
# (you can also set it on the command line: -D CMAKE_BUILD_TYPE=Release)


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})


find_package(SDL2 REQUIRED)

if(NOT SDL2_FOUND)
  message(FATAL_ERROR "Failed to find SDL2")
endif()


find_package(OpenGL REQUIRED)


if(WIN32)
  set(CXX_PLATFORM_SOURCES platform_win32.cpp)
elseif(APPLE)
  set(CXX_PLATFORM_SOURCES platform_macosx.cpp)
else()
  message(FATAL_ERROR "Provide platform implementation and add to CMakeLists.txt")
endif()

set(C_SOURCES json.c linenoise.c)

set(IMGUI_SOURCES
  dearimgui/imconfig.h
  dearimgui/imgui.cpp
  dearimgui/imgui.h
  dearimgui/imgui_demo.cpp
  dearimgui/imgui_draw.cpp
  dearimgui/imgui_impl_sdl.h
  dearimgui/imgui_impl_sdl.cpp
  )

set(CXX_SOURCES
  main.cpp
  str.cpp
  MurmurHash3.cpp
  nametable.cpp
  hashtable_test.cpp
  nametable_test.cpp
  tokenizer.cpp
  test.cpp
  pretty.cpp
  dynarray.h
  hashtable.h
  common.h
  json.h
  platform.h
  imgui_helpers.h
  logging.h
  logging.cpp
  ${IMGUI_SOURCES}  
  ${CXX_PLATFORM_SOURCES}
  )

set(SOURCES ${C_SOURCES} ${CXX_SOURCES})


add_executable(${PROJECT_NAME} ${SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 98)

target_include_directories(${PROJECT_NAME}
  PRIVATE
  ${SDL2_INCLUDE_DIR}
  ${OPENGL_INCLUDE_DIR}
  )

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  ${SDL2_LIBRARY}
  ${OPENGL_LIBRARIES}
  )


if("${CMAKE_CXX_COMPILER_ID}" MATCHES "MSVC")
  
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
  set_property(SOURCE ${CXX_SOURCES} PROPERTY COMPILE_FLAGS "--std=c++03")

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
    -Wno-float-equal
    -Wno-documentation
    )

  set_property(SOURCE MurmurHash3.cpp APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-sign-conversion -Wno-cast-align")
  set_property(SOURCE linenoise.c APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-unreachable-code-return -Wno-conversion -Wno-sign-conversion -Wno-shorten-64-to-32 -Wno-gnu-zero-variadic-macro-arguments")

  set_property(SOURCE ${IMGUI_SOURCES} APPEND_STRING PROPERTY COMPILE_FLAGS " -Wno-float-equal -Wno-conversion -Wno-exit-time-destructors -Wno-shadow -Wno-unused-parameter")
endif()

