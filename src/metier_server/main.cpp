// Copyright (c) 2019-2024 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include <boost/program_options.hpp>

#pragma GCC diagnostic pop
#include <opentxs/opentxs.hpp>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <map>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wheader-hygiene"
using namespace std::literals;
#pragma GCC diagnostic pop

using Type = opentxs::blockchain::Type;
using Enabled = opentxs::Map<Type, opentxs::UnallocatedCString>;
using Disabled = opentxs::Set<Type>;

constexpr auto all_{"all"};
constexpr auto help_{"help"};
constexpr auto home_{"data_dir"};
constexpr auto sync_public_ip_{"public_addr"};
constexpr auto sync_server_{"sync_server"};

struct Options {
    opentxs::Options ot_{};
    Enabled enabled_chains_{};
    bool show_help_{};
    int sync_port_{};
    bool start_sync_server_{};
    opentxs::UnallocatedCString sync_server_public_ip_{};
};

auto options() noexcept -> boost::program_options::options_description const&;
auto lower(opentxs::UnallocatedCString& str) noexcept
    -> opentxs::UnallocatedCString&;
auto parse(
    opentxs::UnallocatedCString const& input,
    Type const type,
    Enabled& enabled,
    Disabled& disabled) noexcept -> void;
auto process_arguments(Options& opts, int argc, char** argv) noexcept -> void;
auto read_options(int argc, char** argv) noexcept -> bool;
auto variables() noexcept -> boost::program_options::variables_map&;

auto main(int argc, char* argv[]) -> int
{
    auto opts = Options{};

    if (false == read_options(argc, argv)) { return 1; }

    process_arguments(opts, argc, argv);

    if (opts.show_help_) {
        std::cout << ::options() << '\n' << opts.ot_.HelpText() << '\n';

        return 0;
    }

    if (opts.start_sync_server_) {
        if (opts.sync_server_public_ip_.empty()) {
            std::cout << "Mandatory argument --public_addr not specified\n";

            return 1;
        }

        using enum opentxs::network::blockchain::Transport;
        // TODO parse the address to see if it is ipv4 or ipv6
        opts.ot_.AddOTDHTListener(
            ipv4, opts.sync_server_public_ip_, ipv4, "0.0.0.0");
    }

    opentxs::api::Context::PrepareSignalHandling();
    auto const& ot = opentxs::InitContext(opts.ot_);
    ot.HandleSignals();
    auto const& client = ot.StartClientSession(opts.ot_, 0);
    auto const enabled = [&] {
        auto out = opentxs::Map<
            std::string_view,
            opentxs::blockchain::Type,
            opentxs::NaturalCaseCompare>{};

        for (auto const& [chain, seed] : opts.enabled_chains_) {
            client.Network().Blockchain().Enable(chain, seed);
            out.try_emplace(opentxs::blockchain::print(chain), chain);
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

    if (opts.start_sync_server_) {
        constexpr auto prefix = "tcp://";
        constexpr auto internal = "0.0.0.0";
        constexpr auto sep = ":";
        auto const& port = opts.sync_port_;
        auto const nextport{port + 1};
        client.Network().OTDHT().StartListener(
            opentxs::UnallocatedCString{prefix} + internal + sep +
                std::to_string(port),
            opentxs::UnallocatedCString{prefix} + opts.sync_server_public_ip_ +
                sep + std::to_string(port),
            opentxs::UnallocatedCString{prefix} + internal + sep +
                std::to_string(nextport),
            opentxs::UnallocatedCString{prefix} + opts.sync_server_public_ip_ +
                sep + std::to_string(nextport));
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

    opentxs::Join();

    return 0;
}

auto lower(opentxs::UnallocatedCString& s) noexcept
    -> opentxs::UnallocatedCString&
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    return s;
}

auto options() noexcept -> boost::program_options::options_description const&
{
    static auto const output = [] {
        auto out = boost::program_options::options_description{
            "Metier-server options"};
        out.add_options()(help_, "Display this message");
        out.add_options()(
            home_,
            boost::program_options::value<opentxs::UnallocatedCString>()
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
            boost::program_options::value<opentxs::UnallocatedCString>(),
            "IP address or domain name where clients can connect to reach the "
            "sync server. Mandatory if --sync_server is specified.");
        out.add_options()(
            all_,
            "Enable all supported blockchains. Seed nodes may still be set by "
            "passing the option for the appropriate chain.");

        for (auto const& chain : opentxs::blockchain::supported_chains()) {
            auto ticker = opentxs::blockchain::ticker_symbol(chain);
            auto message = std::stringstream{};
            message << "Enable " << opentxs::blockchain::print(chain)
                    << " blockchain.\nOptionally specify ip address of seed "
                       "node or \"off\" to disable";
            out.add_options()(
                lower(ticker).c_str(),
                boost::program_options::value<opentxs::UnallocatedCString>()
                    ->implicit_value(""),
                message.str().c_str());
        }
        return out;
    }();

    return output;
}

auto parse(
    opentxs::UnallocatedCString const& input,
    Type const type,
    Enabled& enabled,
    Disabled& disabled) noexcept -> void
{
    constexpr auto off{"off"};

    if (input == off) {
        disabled.emplace(type);
        enabled.erase(type);
    } else if (0u == disabled.count(type)) {
        enabled[type] = input;
    }
}

auto process_arguments(Options& opts, int argc, char** argv) noexcept -> void
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
    auto map = opentxs::Map<opentxs::UnallocatedCString, Type>{};

    for (auto const& chain : opentxs::blockchain::supported_chains()) {
        auto ticker = opentxs::blockchain::ticker_symbol(chain);
        lower(ticker);
        map.emplace(std::move(ticker), chain);
    }

    auto seed = opentxs::UnallocatedCString{};
    auto& otargs = opts.ot_;
    otargs.SetHome(
        opentxs::api::Context::SuggestFolder("metier-server").c_str());
    otargs.SetBlockchainProfile(opentxs::blockchain::Profile::server);
    otargs.ParseCommandLine(argc, argv);
    auto& enabled = opts.enabled_chains_;
    auto& syncPort = opts.sync_port_;
    auto& publicIP = opts.sync_server_public_ip_;
    auto disabled = opentxs::Set<Type>{};

    for (auto const& [name, value] : variables()) {
        if (name == help_) {
            opts.show_help_ = true;
        } else if (name == all_) {
            for (auto const chain : allChains) {
                if (0u == disabled.count(chain)) {
                    opts.enabled_chains_[chain];
                }
            }
        } else if (name == home_) {
            try {
                otargs.SetHome(value.as<opentxs::UnallocatedCString>().c_str());
            } catch (...) {
            }
        } else if (name == sync_server_) {
            try {
                syncPort = value.as<decltype(opts.sync_port_)>();
                otargs.SetBlockchainProfile(
                    opentxs::blockchain::Profile::server);
            } catch (...) {
            }
        } else if (name == sync_public_ip_) {
            try {
                publicIP = value.as<decltype(opts.sync_server_public_ip_)>();
            } catch (...) {
            }
        } else {
            try {
                auto input{name};
                auto const chain = map.at(lower(input));
                parse(
                    value.as<opentxs::UnallocatedCString>(),
                    chain,
                    enabled,
                    disabled);
            } catch (...) {
                continue;
            }
        }
    }

    for (auto const chain : disabled) { otargs.DisableBlockchain(chain); }

    opts.start_sync_server_ =
        (0 < syncPort) &&
        (std::numeric_limits<std::uint16_t>::max() > syncPort);
    otargs.SetBlockchainSyncEnabled(opts.start_sync_server_);
}

auto read_options(int argc, char** argv) noexcept -> bool
{
    try {
        auto const parsed =
            boost::program_options::command_line_parser(argc, argv)
                .options(options())
                .allow_unregistered()
                .run();
        boost::program_options::store(parsed, variables());
        boost::program_options::notify(variables());

        return true;
    } catch (boost::program_options::error& e) {
        std::cerr << "ERROR: " << e.what() << "\n\n" << options() << std::endl;

        return false;
    }
}

auto variables() noexcept -> boost::program_options::variables_map&
{
    static auto output = boost::program_options::variables_map{};

    return output;
}
