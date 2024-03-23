# SPDX-License-Identifier: GPL-2.0-or-later */
#
# Copyright(c) 2024 Sanpe <sanpeqf@gmail.com>
#

cmake_minimum_required(VERSION 3.12)
project(w80xprog VERSION 1.2 LANGUAGES C)

include(GNUInstallDirs)
include(CheckIncludeFiles)

add_compile_options(
    -std=gnu11
    -Wall
    -Wextra
    -Wno-unused-parameter
    -Wno-sign-compare
    -D_GNU_SOURCE
)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
add_subdirectory(bfdev)
get_property(BFDEV_INCLUDE_DIRS GLOBAL PROPERTY "BFDEV_INCLUDE_DIRS")

include_directories(
    ${BFDEV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)

file(GLOB W80XPROG_SOURCE
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.c
)

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/config.h.in
    ${CMAKE_BINARY_DIR}/generated/config.h
)

add_executable(${CMAKE_PROJECT_NAME} ${W80XPROG_SOURCE})
target_link_libraries(${CMAKE_PROJECT_NAME} bfdev)

install(TARGETS
    ${CMAKE_PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
