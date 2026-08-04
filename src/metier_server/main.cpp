// Copyright (c) 2019-2024 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "metier_server/App.hpp"
#include "metier_server/external/opentxs.hpp"

auto main(int argc, char* argv[]) -> int
{
    opentxs::init_terminate_handler();

    try {
        auto alloc = opentxs::alloc::Strategy{};

        return metier_server::App{argc, argv, alloc}.Run();
    } catch (std::exception const& e) {
        opentxs::LogError()(e.what()).Flush();

        return 1;
    } catch (...) {
        opentxs::LogError()("unknown exception").Flush();

        return 1;
    }
}
