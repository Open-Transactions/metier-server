# Copyright (c) 2019-2020 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(cxx-sources main.cpp)

add_executable(otblockchain ${cxx-sources})

target_link_libraries(
  otblockchain
  PRIVATE
    Threads::Threads
    opentxs
    Boost::program_options
)

if(Qt5_FOUND)
  target_link_libraries(
    otblockchain
    PRIVATE
      Qt5::Core
      Qt5::Gui
      Qt5::Qml
      Qt5::Widgets
  )
endif()

install(TARGETS otblockchain)
