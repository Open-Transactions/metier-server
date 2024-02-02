# Copyright (c) 2019-2024 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(cxx-sources main.cpp)

add_executable(${PROJECT_NAME} ${cxx-sources})
otcommon_configure_cxx_target(${PROJECT_NAME})
otcommon_define_signed_overflow(${PROJECT_NAME})

target_link_libraries(
  ${PROJECT_NAME} PUBLIC opentxs::libopentxs Boost::program_options
)

install(TARGETS ${PROJECT_NAME})
