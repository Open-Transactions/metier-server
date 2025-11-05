// Copyright (c) 2019-2024 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <opentxs/opentxs.hpp>

#include "metier_server/Options.hpp"

namespace metier_server
{
class App
{
public:
    auto Run() noexcept(false) -> int;

    App(int argc, char** argv, opentxs::alloc::Strategy& alloc) noexcept(false);

private:
    opentxs::alloc::Strategy& alloc_;
    Options const options_;

    auto print_help() noexcept(false) -> int;
    auto run_daemon() noexcept(false) -> int;
};
}  // namespace metier_server
