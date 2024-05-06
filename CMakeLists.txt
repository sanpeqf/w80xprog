# SPDX-License-Identifier: GPL-2.0-or-later */
#
# Copyright(c) 2024 Sanpe <sanpeqf@gmail.com>
#

cmake_minimum_required(VERSION 3.12)
project(w80xprog VERSION 1.2 LANGUAGES C)

include(GNUInstallDirs)
include(CheckIncludeFiles)

set(W80XPROG_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(W80XPROG_GENERATED_PATH ${PROJECT_BINARY_DIR}/generated)

configure_file(
    ${W80XPROG_MODULE_PATH}/config.h.in
    ${W80XPROG_GENERATED_PATH}/config.h
)

add_compile_options(
    -std=gnu11
    -Wall
    -Wextra
    -Werror
    -Wno-unused-parameter
    -Wno-sign-compare
    -D_GNU_SOURCE
)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/generated
)

file(GLOB W80XPROG_SOURCE
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.c
)

include(${W80XPROG_MODULE_PATH}/bfdev.cmake)
add_executable(${CMAKE_PROJECT_NAME} ${W80XPROG_SOURCE})
target_link_libraries(${CMAKE_PROJECT_NAME} bfdev)

install(TARGETS
    ${CMAKE_PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
