// Copyright (c) 2019-2024 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "metier_server/EventLoop.hpp"  // IWYU pragma: associated

namespace metier_server
{
EventLoop::EventLoop(
    opentxs::api::Session const& api,
    opentxs::alloc::Strategy& alloc,
    allocator_type) noexcept(false)
    : opentxs::api::session::EventLoop(alloc.result_)
    , enabled_chains_(sort_enabled_chains(api, alloc))
    , stats_(api.Network().Blockchain().Stats(alloc), alloc.result_)
    , chain_print_width_([&] {
        if (enabled_chains_.empty()) {

            return 0;
        } else {
            auto const get_width = [](auto type) noexcept {
                return static_cast<int>(print(type).size());
            };
            auto const max = std::ranges::max(enabled_chains_, {}, get_width);

            return get_width(max);
        }
    }())
{
}

auto EventLoop::Description() const noexcept -> std::string_view
{
    return "Metier-server"sv;
}

auto EventLoop::get_deleter() noexcept -> delete_function
{
    return opentxs::pmr::Deleter::Factory(*this);
}

auto EventLoop::print_status(
    opentxs::util::eventloop::state::ProcessMessage& state) noexcept(false)
    -> void
{
    auto out = std::stringstream{};

    {
        out << std::setw(chain_print_width_) << " ";
        out << std::setw(status_print_width_) << "overall ";
        out << std::setw(status_print_width_) << "peer ";
        out << std::setw(status_print_width_) << "block ";
        out << std::setw(status_print_width_) << "block";
        out << std::setw(status_print_width_) << "cfheader";
        out << std::setw(status_print_width_) << "cfilter";
        out << '\n';
    }

    {
        out << std::setw(chain_print_width_) << " ";
        out << std::setw(status_print_width_) << "progress";
        out << std::setw(status_print_width_) << "count";
        out << std::setw(status_print_width_) << "headers";
        out << std::setw(status_print_width_) << "chain";
        out << std::setw(status_print_width_) << "chain ";
        out << std::setw(status_print_width_) << "chain ";
        out << '\n';
    }

    auto const print_status = [this, &out](auto const chain) {
        out << std::setw(chain_print_width_) << print(chain);
        out << std::setw(status_print_width_ - 1) << std::fixed
            << std::setprecision(2) << stats_.Progress(chain) << "%";
        out << std::setw(status_print_width_) << stats_.PeerCount(chain);
        out << std::setw(status_print_width_)
            << stats_.BlockHeaderTip(chain).height_;
        out << std::setw(status_print_width_) << stats_.BlockTip(chain).height_;
        out << std::setw(status_print_width_)
            << stats_.CfheaderTip(chain).height_;
        out << std::setw(status_print_width_)
            << stats_.CfilterTip(chain).height_;
        out << '\n';
    };
    std::ranges::for_each(enabled_chains_, print_status);
    std::cout << out.str() << std::endl;
    reset_status_timer(state);
}

auto EventLoop::Run(
    opentxs::api::Session const&,
    opentxs::util::eventloop::state::PreInit&,
    opentxs::util::eventloop::Socket&,
    std::span<opentxs::util::eventloop::Socket>,
    std::span<opentxs::util::eventloop::Socket>,
    opentxs::alloc::Strategy&) noexcept(false) -> bool
{
    return true;
}

auto EventLoop::Run(
    opentxs::api::Session const&,
    opentxs::util::eventloop::state::Init& state,
    opentxs::alloc::Strategy&) noexcept(false) -> bool
{
    reset_status_timer(state);

    return true;
}

auto EventLoop::Run(
    opentxs::api::Session const&,
    opentxs::util::eventloop::state::ProcessMessage& state,
    opentxs::util::eventloop::SocketIndex,
    std::optional<opentxs::util::eventloop::MessageType> type,
    opentxs::util::eventloop::Message&&,
    opentxs::alloc::Strategy&) noexcept(false) -> bool
{
    using enum opentxs::util::WorkType;

    switch (auto const t = type.value_or(value(Unknown)); t) {
        case print_status_: {
            print_status(state);
        } break;
        default: {

            throw std::runtime_error{
                "received message of unsupported type "s.append(
                    opentxs::print_work_type(t))};
        }
    }

    return true;
}

auto EventLoop::sort_enabled_chains(
    opentxs::api::Session const& api,
    opentxs::alloc::Strategy& alloc) noexcept(false) -> EnabledChains
{
    auto sorted = [&] {
        auto out = opentxs::Map<
            std::string_view,
            opentxs::blockchain::Type,
            opentxs::NaturalCaseCompare>{alloc.work_};
        constexpr auto to_name = [](auto const& chain) {
            return std::make_pair(print(chain), chain);
        };
        auto const enabled =
            api.Network().Blockchain().EnabledChains(alloc.WorkOnly());
        std::ranges::transform(enabled, std::inserter(out, out.end()), to_name);

        return out;
    }();
    auto out = EnabledChains{alloc.result_};
    std::ranges::copy(sorted | std::views::values, std::back_inserter(out));

    return out;
}

EventLoop::~EventLoop() = default;
}  // namespace metier_server
