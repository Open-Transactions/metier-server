// Copyright (c) 2019-2024 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include <boost/program_options.hpp>
#pragma GCC diagnostic pop
#include <opentxs/opentxs.hpp>

namespace metier_server
{
using namespace std::literals;

class Options
{
public:
    static auto options() noexcept
        -> boost::program_options::options_description const&;

    using Enabled = opentxs::Set<opentxs::blockchain::Type>;
    using Disabled = opentxs::Set<opentxs::blockchain::Type>;

    opentxs::api::Options ot_;
    Enabled disabled_chains_;
    Enabled enabled_chains_;
    bool show_help_;
    int sync_port_;
    bool start_sync_server_;
    opentxs::String sync_server_public_ip_;

    Options(int argc, char** argv, opentxs::alloc::Strategy& alloc) noexcept(
        false);

private:
    static constexpr auto all_ = "all";
    static constexpr auto help_ = "help";
    static constexpr auto home_ = "data_dir";
    static constexpr auto sync_public_ip_ = "public_addr";
    static constexpr auto sync_server_ = "sync_server";
    static constexpr auto off_ = "off"sv;

    static auto lower(opentxs::UnallocatedString& str) noexcept
        -> opentxs::UnallocatedString&;
    static auto read_options(int argc, char** argv) noexcept(false) -> void;
    static auto variables() noexcept -> boost::program_options::variables_map&;

    auto parse(std::string_view input, opentxs::blockchain::Type type) noexcept
        -> void;
    auto process_arguments(int argc, char** argv) noexcept(false) -> void;
};
}  // namespace metier_server
