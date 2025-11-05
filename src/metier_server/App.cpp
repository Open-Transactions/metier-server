// Copyright (c) 2019-2024 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "metier_server/App.hpp"  // IWYU pragma: associated

#include "metier_server/EventLoop.hpp"

namespace metier_server
{
using namespace std::literals;

App::App(int argc, char** argv, opentxs::alloc::Strategy& alloc) noexcept(false)
    : alloc_(alloc)
    , options_(argc, argv, alloc_)
{
}

auto App::print_help() noexcept(false) -> int
{
    std::cout << options_.options() << '\n' << options_.ot_.HelpText() << '\n';

    return 0;
}

auto App::Run() noexcept(false) -> int
{
    if (options_.show_help_) {

        return print_help();
    } else {

        return run_daemon();
    }
}

auto App::run_daemon() noexcept(false) -> int
{
    opentxs::api::Context::PrepareSignalHandling();
    auto const& ot = opentxs::start(options_.ot_);
    ot.HandleSignals();
    auto const& client = ot.StartClientSession(options_.ot_, 0);

    if (options_.start_sync_server_) {
        constexpr auto prefix = "tcp://";
        constexpr auto internal = "0.0.0.0";
        constexpr auto sep = ":";
        auto const& port = options_.sync_port_;
        auto const nextport{port + 1};
        auto const started = client.Network().OTDHT().StartListener(
            opentxs::String{prefix, alloc_.work_}
                .append(internal)
                .append(sep)
                .append(std::to_string(port)),
            opentxs::String{prefix, alloc_.work_}
                .append(options_.sync_server_public_ip_)
                .append(sep)
                .append(std::to_string(port)),
            opentxs::String{prefix, alloc_.work_}
                .append(internal)
                .append(sep)
                .append(std::to_string(nextport)),
            opentxs::String{prefix, alloc_.work_}
                .append(options_.sync_server_public_ip_)
                .append(sep)
                .append(std::to_string(nextport)));

        if (false == started) {

            throw std::runtime_error{"failed to start otdht listener"};
        }
    }

    for (auto const& chain : options_.enabled_chains_) {
        if (false == client.Network().Blockchain().Enable(chain).IsValid()) {

            throw std::runtime_error{"unable to enable "s.append(print(chain))};
        }
    }

    auto const running = client.StartSessionEventLoop(
        [](auto const& api, auto& alloc) {
            return std::allocate_shared<EventLoop>(alloc.result_, api, alloc);
        },
        {},
        {},
        1,
        alloc_);

    if (false == running) {

        throw std::runtime_error{"failed to start event loop"};
    }

    opentxs::join();

    return 0;
}
}  // namespace metier_server
