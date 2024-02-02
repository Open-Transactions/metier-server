# Copyright (c) 2019-2024 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.26.0)
cmake_policy(
  SET
  CMP0091
  NEW
)
cmake_policy(
  SET
  CMP0126
  NEW
)
project(metier-server)

# -----------------------------------------------------------------------------
# Setup

find_package(
  otcommon
  3
  CONFIG
  REQUIRED
)
otcommon_set_build_type()
otcommon_require_out_of_source_build()

# -----------------------------------------------------------------------------
# Set option defaults

if(CMAKE_BUILD_TYPE
   STREQUAL
   "Debug"
)
  set(METIER_SERVER_PEDANTIC_DEFAULT ON)
  set(CMAKE_VERBOSE_MAKEFILE ON)
else()
  set(METIER_SERVER_PEDANTIC_DEFAULT OFF)
endif()

# -----------------------------------------------------------------------------
# Options

option(
  METIER_SERVER_PEDANTIC_BUILD
  "Treat compiler warnings as errors."
  ${METIER_SERVER_PEDANTIC_DEFAULT}
)

# -----------------------------------------------------------------------------
# Set compiler options

otcommon_set_project_language_standard(
  99
  20
  ${METIER_SERVER_PEDANTIC_BUILD}
)

# -----------------------------------------------------------------------------
# Print system information and build options

otcommon_print_build_details(METIER_SERVER_PEDANTIC_BUILD "")

# -----------------------------------------------------------------------------
# Dependencies

otcommon_find_system_libraries()
find_package(
  opentxs
  CONFIG
  REQUIRED
)
find_package(
  Boost
  REQUIRED
  program_options
)

# -----------------------------------------------------------------------------
# Build source

add_subdirectory(src)
