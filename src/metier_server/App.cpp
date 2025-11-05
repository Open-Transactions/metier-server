// Copyright (c) 2019-2024 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "metier_server/App.hpp"  // IWYU pragma: associated

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
    auto const enabled = [&] {
        auto out = opentxs::Map<
            std::string_view,
            opentxs::blockchain::Type,
            opentxs::NaturalCaseCompare>{};

        for (auto const& chain : options_.enabled_chains_) {
            if (client.Network().Blockchain().Enable(chain)) {
                out.try_emplace(opentxs::blockchain::print(chain), chain);
            } else {

                throw std::runtime_error{
                    "unable to enable "s.append(print(chain))};
            }
        }

        return out;
    }();
    auto const sorted = [&] {
        auto out = opentxs::Vector<opentxs::blockchain::Type>{};
        out.reserve(enabled.size());
        std::ranges::copy(
            enabled | std::views::values, std::back_inserter(out));

        return out;
    }();

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

    client.Schedule(
        6s,
        [chains = sorted,
         stats = client.Network().Blockchain().Stats()]() -> void {
            static auto const widthChain = [] {
                auto out = std::size_t{0};

                for (auto const chain : opentxs::blockchain::defined_chains()) {
                    out =
                        std::max(out, opentxs::blockchain::print(chain).size());
                }

                return static_cast<int>(out + 2);
            }();
            static constexpr auto width{10};
            auto out = std::stringstream{};

            {
                out << std::setw(widthChain) << " ";
                out << std::setw(width) << "overall ";
                out << std::setw(width) << "peer ";
                out << std::setw(width) << "block ";
                out << std::setw(width) << "block";
                out << std::setw(width) << "cfheader";
                out << std::setw(width) << "cfilter";
                out << '\n';
            }

            {
                out << std::setw(widthChain) << " ";
                out << std::setw(width) << "progress";
                out << std::setw(width) << "count";
                out << std::setw(width) << "headers";
                out << std::setw(width) << "chain";
                out << std::setw(width) << "chain ";
                out << std::setw(width) << "chain ";
                out << '\n';
            }

            for (auto const& chain : chains) {
                out << std::setw(widthChain) << print(chain);
                out << std::setw(width - 1) << std::fixed
                    << std::setprecision(2) << stats.Progress(chain) << "%";
                out << std::setw(width) << stats.PeerCount(chain);
                out << std::setw(width) << stats.BlockHeaderTip(chain).height_;
                out << std::setw(width) << stats.BlockTip(chain).height_;
                out << std::setw(width) << stats.CfheaderTip(chain).height_;
                out << std::setw(width) << stats.CfilterTip(chain).height_;
                out << '\n';
            }

            std::cout << out.str() << std::endl;
        });
    opentxs::join();

    return 0;
}
}  // namespace metier_server
