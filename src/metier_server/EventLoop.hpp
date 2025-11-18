// Copyright (c) 2019-2024 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <opentxs/opentxs.hpp>

namespace metier_server
{
using namespace std::literals;
using namespace opentxs::literals;

class EventLoop final : public opentxs::api::session::EventLoop
{
public:
    [[nodiscard]] auto Description() const noexcept -> std::string_view final;

    [[nodiscard]] auto get_deleter() noexcept -> delete_function final;
    [[nodiscard]] auto RequestedTimerCount() const noexcept
        -> std::size_t final;
    [[nodiscard]] auto Run(
        opentxs::api::Session const& api,
        opentxs::util::eventloop::state::PreInit& state,
        opentxs::util::eventloop::Socket& subscribe,
        std::span<opentxs::util::eventloop::Socket> internal,
        std::span<opentxs::util::eventloop::Socket> external,
        opentxs::alloc::Strategy& alloc) noexcept(false) -> bool final;
    [[nodiscard]] auto Run(
        opentxs::api::Session const& api,
        opentxs::util::eventloop::state::Init& state,
        opentxs::alloc::Strategy& alloc) noexcept(false) -> bool final;
    [[nodiscard]] auto Run(
        opentxs::api::Session const& api,
        opentxs::util::eventloop::state::ProcessMessage& state,
        opentxs::util::eventloop::SocketIndex index,
        std::optional<opentxs::util::eventloop::MessageType> type,
        opentxs::util::eventloop::Message&& message,
        opentxs::alloc::Strategy& alloc) noexcept(false) -> bool final;

    EventLoop(
        opentxs::api::Session const& api,
        opentxs::alloc::Strategy& alloc,
        allocator_type) noexcept(false);

    ~EventLoop() final;

private:
    using EnabledChains = opentxs::Vector<opentxs::blockchain::Type>;

    static constexpr auto status_timer_index_ = 0_uz;
    static constexpr auto print_interval_ = 6s;
    static constexpr auto print_status_ =
        opentxs::first_user_defined_work_type_ + 0U;
    static constexpr auto status_print_width_ = 10;

    EnabledChains const enabled_chains_;
    opentxs::api::network::blockchain::Stats const stats_;
    int const chain_print_width_;

    static auto sort_enabled_chains(
        opentxs::api::Session const& api,
        opentxs::alloc::Strategy& alloc) noexcept(false) -> EnabledChains;

    auto print_status(
        opentxs::util::eventloop::state::ProcessMessage& state) noexcept(false)
        -> void;
    template <typename State>
    auto reset_status_timer(State& state) noexcept(false) -> void
    {
        get_timer(state, status_timer_index_)
            .Activate(state, print_interval_, print_status_);
    }
};
}  // namespace metier_server
