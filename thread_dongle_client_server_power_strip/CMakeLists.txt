#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(thread_coap_client_server_dongle_power_strip)

FILE(GLOB app_sources src/*.c src/client/*.c src/server/*.c src/relays/*.c)
# NORDIC SDK APP START
target_sources(app PRIVATE ${app_sources})

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/relays
)

target_include_directories(app PUBLIC ../thread_dongle_server/interface)
# NORDIC SDK APP END