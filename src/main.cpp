// Copyright (c) 2019-2022 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/program_options.hpp>
#include <opentxs/opentxs.hpp>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace ot = opentxs;
namespace po = boost::program_options;

using Type = ot::blockchain::Type;
using Enabled = std::map<Type, std::string>;
using Disabled = std::set<Type>;

constexpr auto all_{"all"};
constexpr auto help_{"help"};
constexpr auto home_{"data_dir"};
constexpr auto sync_public_ip_{"public_addr"};
constexpr auto sync_server_{"sync_server"};

struct Options {
    ot::Options ot_{};
    Enabled enabled_chains_{};
    bool show_help_{};
    int sync_port_{};
    bool start_sync_server_{};
    std::string sync_server_public_ip_{};
};

auto options() noexcept -> const po::options_description&;
auto lower(std::string& str) noexcept -> std::string&;
auto parse(
    const std::string& input,
    const Type type,
    Enabled& enabled,
    Disabled& disabled) noexcept -> void;
auto process_arguments(Options& opts, int argc, char** argv) noexcept -> void;
auto read_options(int argc, char** argv) noexcept -> bool;
auto variables() noexcept -> po::variables_map&;

auto main(int argc, char* argv[]) -> int
{
    auto opts = Options{};

    if (false == read_options(argc, argv)) { return 1; }

    process_arguments(opts, argc, argv);

    if (opts.show_help_) {
        std::cout << ::options() << '\n' << opts.ot_.HelpText() << '\n';

        return 0;
    }

    if (opts.start_sync_server_ && opts.sync_server_public_ip_.empty()) {
        std::cout << "Mandatory argument --public_addr not specified\n";

        return 1;
    }

    ot::api::Context::PrepareSignalHandling();
    const auto& ot = ot::InitContext(opts.ot_);
    ot.HandleSignals();
    const auto& client = ot.StartClientSession(opts.ot_, 0);

    for (const auto& [chain, seed] : opts.enabled_chains_) {
        client.Network().Blockchain().Enable(chain, seed);
    }

    if (opts.start_sync_server_) {
        constexpr auto prefix = "tcp://";
        constexpr auto internal = "0.0.0.0";
        constexpr auto sep = ":";
        const auto& port = opts.sync_port_;
        const auto nextport{port + 1};
        client.Network().Blockchain().StartSyncServer(
            std::string{prefix} + internal + sep + std::to_string(port),
            std::string{prefix} + opts.sync_server_public_ip_ + sep +
                std::to_string(port),
            std::string{prefix} + internal + sep + std::to_string(nextport),
            std::string{prefix} + opts.sync_server_public_ip_ + sep +
                std::to_string(nextport));
    }

    client.Schedule(std::chrono::seconds(30), [&]() -> void {
        static const auto widthChain = [] {
            auto out = std::size_t{0};

            for (const auto chain : ot::blockchain::DefinedChains()) {
                out =
                    std::max(out, ot::blockchain::print(chain).size());
            }

            return static_cast<int>(out + 2);
        }();
        static constexpr auto width{10};
        const auto header1 = [&] {
            auto out = std::stringstream{};
            out << std::setw(widthChain) << " ";
            out << std::setw(width) << "peer ";
            out << std::setw(width) << "block ";
            out << std::setw(width) << "block";
            out << std::setw(width) << "cfilter";
            out << std::setw(width) << "sync ";

            return out.str();
        }();
        const auto header2 = [&] {
            auto out = std::stringstream{};
            out << std::setw(widthChain) << " ";
            out << std::setw(width) << "count";
            out << std::setw(width) << "headers";
            out << std::setw(width) << "chain";
            out << std::setw(width) << "chain ";
            out << std::setw(width) << "server";

            return out.str();
        }();

        std::cout << header1 << '\n';
        std::cout << header2 << '\n';

        for (const auto& data : opts.enabled_chains_) {
            const auto& [chain, seed] = data;
            const auto& node = client.Network().Blockchain().GetChain(chain);
            const auto& headers = node.HeaderOracle();
            const auto& blocks = node.BlockOracle();
            const auto& filters = node.FilterOracle();
            const auto status = [&] {
                auto out = std::stringstream{};
                out << std::setw(widthChain) << print(data.first);
                out << std::setw(width)
                    << std::to_string(node.GetVerifiedPeerCount());
                out << std::setw(width)
                    << std::to_string(headers.BestChain().first);
                out << std::setw(width) << std::to_string(blocks.Tip().first);
                out << std::setw(width)
                    << std::to_string(
                           filters.FilterTip(filters.DefaultType()).first);
                out << std::setw(width) << std::to_string(node.SyncTip().first);

                return out.str();
            }();
            std::cout << status << '\n';
        }

        std::cout << std::endl;
    });

    ot::Join();

    return 0;
}

auto lower(std::string& s) noexcept -> std::string&
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    return s;
}

auto options() noexcept -> const po::options_description&
{
    static const auto output = [] {
        auto out = po::options_description{"Metier-server options"};
        out.add_options()(help_, "Display this message");
        out.add_options()(
            home_,
            po::value<std::string>()->default_value(
                ot::api::Context::SuggestFolder("metier-server")),
            "Path to data directory");
        out.add_options()(
            sync_server_,
            po::value<int>(),
            "Starting TCP port to use for sync server. Two ports will be "
            "allocated. Implies --blockchain_storage=2");
        out.add_options()(
            sync_public_ip_,
            po::value<std::string>(),
            "IP address or domain name where clients can connect to reach the "
            "sync server. Mandatory if --sync_server is specified.");
        out.add_options()(
            all_,
            "Enable all supported blockchains. Seed nodes may still be set by "
            "passing the option for the appropriate chain.");

        for (const auto& chain : ot::blockchain::SupportedChains()) {
            auto ticker = ot::blockchain::TickerSymbol(chain);
            auto message = std::stringstream{};
            message << "Enable " << ot::blockchain::print(chain)
                    << " blockchain.\nOptionally specify ip address of seed "
                       "node or \"off\" to disable";
            out.add_options()(
                lower(ticker).c_str(),
                po::value<std::string>()->implicit_value(""),
                message.str().c_str());
        }
        return out;
    }();

    return output;
}

auto parse(
    const std::string& input,
    const Type type,
    Enabled& enabled,
    Disabled& disabled) noexcept -> void
{
    constexpr auto off{"off"};

    if (input == off) {
        disabled.emplace(type);
    } else {
        enabled[type] = input;
    }
}

auto process_arguments(Options& opts, int argc, char** argv) noexcept -> void
{
    auto map = std::map<std::string, Type>{};

    for (const auto& chain : ot::blockchain::SupportedChains()) {
        auto ticker = ot::blockchain::TickerSymbol(chain);
        lower(ticker);
        map.emplace(std::move(ticker), chain);
    }

    auto seed = std::string{};
    auto& otargs = opts.ot_;
    otargs.SetHome(ot::api::Context::SuggestFolder("metier-server").c_str());
    otargs.ParseCommandLine(argc, argv);
    auto& enabled = opts.enabled_chains_;
    auto& syncPort = opts.sync_port_;
    auto& publicIP = opts.sync_server_public_ip_;
    auto disabled = std::set<Type>{};

    for (const auto& [name, value] : variables()) {
        if (name == help_) {
            opts.show_help_ = true;
        } else if (name == all_) {
            for (const auto chain : ot::blockchain::SupportedChains()) {
                opts.enabled_chains_[chain];
            }
        } else if (name == home_) {
            try {
                otargs.SetHome(value.as<std::string>().c_str());
            } catch (...) {
            }
        } else if (name == sync_server_) {
            try {
                syncPort = value.as<decltype(opts.sync_port_)>();
                otargs.SetBlockchainStorageLevel(2);
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
                const auto chain = map.at(lower(input));
                parse(value.as<std::string>(), chain, enabled, disabled);
            } catch (...) {
                continue;
            }
        }
    }

    for (const auto chain : disabled) { otargs.DisableBlockchain(chain); }

    opts.start_sync_server_ =
        (0 < syncPort) &&
        (std::numeric_limits<std::uint16_t>::max() > syncPort);
    otargs.SetBlockchainSyncEnabled(opts.start_sync_server_);
}

auto read_options(int argc, char** argv) noexcept -> bool
{
    try {
        const auto parsed = po::command_line_parser(argc, argv)
                                .options(options())
                                .allow_unregistered()
                                .run();
        po::store(parsed, variables());
        po::notify(variables());

        return true;
    } catch (po::error& e) {
        std::cerr << "ERROR: " << e.what() << "\n\n" << options() << std::endl;

        return false;
    }
}

auto variables() noexcept -> po::variables_map&
{
    static auto output = po::variables_map{};

    return output;
}
