// Copyright (c) 2019-2024 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "metier_server/Options.hpp"  // IWYU pragma: associated

namespace metier_server
{
using namespace std::literals;
using namespace opentxs::literals;

Options::Options(
    int argc,
    char** argv,
    opentxs::alloc::Strategy& alloc) noexcept(false)
    : ot_()
    , disabled_chains_(alloc.result_)
    , enabled_chains_(alloc.result_)
    , show_help_()
    , sync_port_()
    , start_sync_server_()
    , sync_server_public_ip_(alloc.result_)
{
    read_options(argc, argv);
    process_arguments(argc, argv);
}

auto Options::lower(opentxs::UnallocatedString& s) noexcept
    -> opentxs::UnallocatedString&
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    return s;
}

auto Options::options() noexcept
    -> boost::program_options::options_description const&
{
    static auto const output = [] {
        auto out = boost::program_options::options_description{
            "Metier-server options"};
        out.add_options()(help_, "Display this message");
        out.add_options()(
            home_,
            boost::program_options::value<opentxs::UnallocatedString>()
                ->default_value(
                    opentxs::api::Context::SuggestFolder("metier-server")),
            "Path to data directory");
        out.add_options()(
            sync_server_,
            boost::program_options::value<int>(),
            "Starting TCP port to use for sync server. Two ports will be "
            "allocated.");
        out.add_options()(
            sync_public_ip_,
            boost::program_options::value<opentxs::UnallocatedString>(),
            "IP address or domain name where clients can connect to reach the "
            "sync server. Mandatory if --sync_server is specified.");
        out.add_options()(
            all_,
            boost::program_options::value<opentxs::UnallocatedString>(),
            "(no arguments) Enable all supported blockchains. Seed nodes may "
            "still be set by "
            "passing the option for the appropriate chain.\nThe optional "
            "argument \"off\" may be passed to disable all previously-enabled "
            "chains.");

        for (auto const& chain : opentxs::blockchain::supported_chains()) {
            auto ticker = opentxs::blockchain::ticker_symbol(chain);
            auto message = std::stringstream{};
            message << "Enable " << opentxs::blockchain::print(chain)
                    << " blockchain.\nOptionally specify ip address of seed "
                       "node or \"off\" to disable";
            out.add_options()(
                lower(ticker).c_str(),
                boost::program_options::value<opentxs::UnallocatedString>()
                    ->implicit_value(""),
                message.str().c_str());
        }
        return out;
    }();

    return output;
}

auto Options::parse(
    std::string_view input,
    opentxs::blockchain::Type type) noexcept -> void
{
    if (off_ == input) {
        disabled_chains_.emplace(type);
        enabled_chains_.erase(type);
    } else {
        enabled_chains_.emplace(type);
        disabled_chains_.erase(type);

        if (false == input.empty()) {
            ot_.AddBlockchainNativePeer(type, input);
        }
    }
}

auto Options::process_arguments(int argc, char** argv) noexcept(false) -> void
{
    static auto const librarySupported =
        opentxs::blockchain::supported_chains();
    static auto const excludeFromAll = [&] {
        using enum opentxs::blockchain::Type;

        return std::set<opentxs::blockchain::Type>{
            BitcoinSV, BitcoinSV_testnet3};
    }();
    static auto const allChains = [&] {
        auto out = std::vector<opentxs::blockchain::Type>();
        out.reserve(librarySupported.size());
        std::set_difference(
            librarySupported.begin(),
            librarySupported.end(),
            excludeFromAll.begin(),
            excludeFromAll.end(),
            std::back_inserter(out));

        return out;
    }();
    static auto const tickers = [] {
        auto out = opentxs::
            Map<opentxs::UnallocatedString, opentxs::blockchain::Type>{};

        for (auto const& chain : opentxs::blockchain::supported_chains()) {
            auto ticker = opentxs::blockchain::ticker_symbol(chain);
            lower(ticker);
            out.emplace(std::move(ticker), chain);
        }

        return out;
    }();
    ot_.SetHome(opentxs::api::Context::SuggestFolder("metier-server").c_str());
    ot_.SetBlockchainProfile(opentxs::blockchain::Profile::server);
    ot_.ParseCommandLine(argc, argv);

    for (auto const& [name, value] : variables()) {
        if (name == help_) {
            show_help_ = true;
        } else if (name == all_) {
            for (auto const chain : allChains) {
                try {
                    if (off_ == value.as<opentxs::UnallocatedString>()) {
                        enabled_chains_.erase(chain);
                        disabled_chains_.emplace(chain);
                    } else if (false == disabled_chains_.contains(chain)) {
                        enabled_chains_.emplace(chain);
                    }
                } catch (...) {
                    continue;
                }
            }
        } else if (name == home_) {
            try {
                ot_.SetHome(value.as<opentxs::UnallocatedString>().c_str());
            } catch (...) {
            }
        } else if (name == sync_server_) {
            try {
                sync_port_ = 0;
                ot_.SetBlockchainProfile(opentxs::blockchain::Profile::server);
            } catch (...) {
            }
        } else if (name == sync_public_ip_) {
            try {
                sync_server_public_ip_ =
                    value.as<decltype(sync_server_public_ip_)>();
            } catch (...) {
            }
        } else {
            try {
                auto input{name};
                auto const chain = tickers.at(lower(input));
                parse(value.as<opentxs::UnallocatedString>(), chain);
            } catch (...) {
                continue;
            }
        }
    }

    for (auto const chain : disabled_chains_) { ot_.DisableBlockchain(chain); }

    start_sync_server_ =
        (0 < sync_port_) &&
        (std::numeric_limits<std::uint16_t>::max() > sync_port_);
    ot_.SetBlockchainSyncEnabled(start_sync_server_);

    if (start_sync_server_) {
        if (sync_server_public_ip_.empty()) {

            throw std::invalid_argument{
                "mandatory argument --public_addr not specified"};
        }

        using enum opentxs::network::blockchain::Transport;
        // TODO parse the address to see if it is ipv4 or ipv6
        ot_.AddOTDHTListener(ipv4, sync_server_public_ip_, ipv4, "0.0.0.0");
    }
}

auto Options::read_options(int argc, char** argv) noexcept(false) -> void
{
    auto const parsed = boost::program_options::command_line_parser(argc, argv)
                            .options(options())
                            .allow_unregistered()
                            .run();
    boost::program_options::store(parsed, variables());
    boost::program_options::notify(variables());
}

auto Options::variables() noexcept -> boost::program_options::variables_map&
{
    static auto output = boost::program_options::variables_map{};

    return output;
}
}  // namespace metier_server
