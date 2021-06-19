// Copyright (c) 2019-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/program_options.hpp>
#include <opentxs/opentxs.hpp>
#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace ot = opentxs;
namespace po = boost::program_options;

using Type = ot::blockchain::Type;
using Enabled = std::map<Type, std::string>;
using Disabled = std::set<std::string>;

struct Options {
    ot::ArgList ot_{};
    Enabled enabled_chains_{};
    bool show_help_{};
    int sync_port_{};
    bool start_sync_server_{};
    std::string sync_server_public_ip_{};
};

po::variables_map* variables_{};
po::options_description* options_{};

auto cleanup_globals() noexcept -> void;
auto options() noexcept -> po::options_description&;
auto lower(std::string& str) noexcept -> std::string&;
auto parse(
    const std::string& input,
    const Type type,
    Enabled& enabled,
    Disabled& disabled) noexcept -> void;
auto process_arguments(Options& opts) noexcept -> void;
auto read_options(int argc, char** argv) noexcept -> bool;
auto variables() noexcept -> po::variables_map&;

int main(int argc, char* argv[])
{
    auto opts = Options{};

    if (false == read_options(argc, argv)) { return 1; }

    process_arguments(opts);

    if (opts.show_help_) {
        std::cout << options() << "\n";

        return 0;
    }

    if (opts.start_sync_server_ && opts.sync_server_public_ip_.empty()) {
        std::cout << "Mandatory argument --public_addr not specified\n";

        return 1;
    }

    ot::Signals::Block();
    const auto& ot = ot::InitContext(opts.ot_);
    ot.HandleSignals();
    const auto& client = ot.StartClient(opts.ot_, 0);

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

    ot::Join();
    cleanup_globals();

    return 0;
}

auto cleanup_globals() noexcept -> void
{
    if (nullptr != variables_) {
        delete variables_;
        variables_ = nullptr;
    }

    if (nullptr != options_) {
        delete options_;
        options_ = nullptr;
    }
}

auto lower(std::string& s) noexcept -> std::string&
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });

    return s;
}

auto options() noexcept -> po::options_description&
{
    if (nullptr == options_) {
        options_ = new po::options_description{"Options"};
    }

    return *options_;
}

auto parse(
    const std::string& input,
    const Type type,
    Enabled& enabled,
    Disabled& disabled) noexcept -> void
{
    constexpr auto off{"off"};

    if (input == off) {
        disabled.emplace(std::to_string(static_cast<std::uint32_t>(type)));
    } else {
        enabled[type] = input;
    }
}

constexpr auto all_{"all"};
constexpr auto help_{"help"};
constexpr auto home_{"data_dir"};
constexpr auto block_storage_{"block_storage"};
constexpr auto sync_server_{"sync_server"};
constexpr auto sync_public_ip_{"public_addr"};
constexpr auto log_level_{OPENTXS_ARG_LOGLEVEL};

auto process_arguments(Options& opts) noexcept -> void
{
    auto map = std::map<std::string, Type>{};

    for (const auto& chain : ot::blockchain::SupportedChains()) {
        auto ticker = ot::blockchain::TickerSymbol(chain);
        lower(ticker);
        map.emplace(std::move(ticker), chain);
    }

    auto seed = std::string{};
    auto& otargs = opts.ot_;
    auto& enabled = opts.enabled_chains_;
    auto& syncPort = opts.sync_port_;
    auto& publicIP = opts.sync_server_public_ip_;
    auto& disabled = otargs[OPENTXS_ARG_DISABLED_BLOCKCHAINS];
    auto& home = otargs[OPENTXS_ARG_HOME];
    auto blockLevel = int{0};
    auto logLevel = int{0};

    for (const auto& [name, value] : variables()) {
        if (name == help_) {
            opts.show_help_ = true;
        } else if (name == all_) {
            for (const auto chain : ot::blockchain::SupportedChains()) {
                opts.enabled_chains_[chain];
            }
        } else if (name == block_storage_) {
            try {
                blockLevel =
                    std::max(blockLevel, value.as<decltype(blockLevel)>());
            } catch (...) {
            }
        } else if (name == home_) {
            try {
                home.emplace(value.as<std::string>());
            } catch (...) {
            }
        } else if (name == sync_server_) {
            try {
                syncPort = value.as<decltype(opts.sync_port_)>();
                blockLevel = 2;
            } catch (...) {
            }
        } else if (name == sync_public_ip_) {
            try {
                publicIP = value.as<decltype(opts.sync_server_public_ip_)>();
            } catch (...) {
            }
        } else if (name == log_level_) {
            try {
                logLevel = value.as<decltype(logLevel)>();
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

    if (0 == disabled.size()) {
        otargs.erase(OPENTXS_ARG_DISABLED_BLOCKCHAINS);
    }

    if (0 == home.size()) { otargs.erase(OPENTXS_ARG_HOME); }

    otargs[OPENTXS_ARG_BLOCK_STORAGE_LEVEL].emplace(std::to_string(blockLevel));
    opts.start_sync_server_ =
        (0 < syncPort) &&
        (std::numeric_limits<std::uint16_t>::max() > syncPort);

    if (opts.start_sync_server_) {
        otargs[OPENTXS_ARG_BLOCKCHAIN_SYNC].emplace("");
    }

    otargs[OPENTXS_ARG_LOGLEVEL].emplace(std::to_string(logLevel));
}

auto read_options(int argc, char** argv) noexcept -> bool
{
    options().add_options()(help_, "Display this message");
    options().add_options()(
        home_,
        po::value<std::string>()->default_value(
            ot::api::Context::SuggestFolder("otblockchain")),
        "Path to data directory");
    options().add_options()(
        block_storage_,
        po::value<int>(),
        "Block persistence level.\n    0: do not save any blocks\n    1: save "
        "blocks downloaded by the wallet\n    2: download and save all blocks");
    options().add_options()(
        sync_server_,
        po::value<int>(),
        "Starting TCP port to use for sync server. Two ports will be "
        "allocated. Implies --block_storage=2");
    options().add_options()(
        sync_public_ip_,
        po::value<std::string>(),
        "IP address or domain name where clients can connect to reach the sync "
        "server. Mandatory if --sync_server is specified.");
    options().add_options()(
        log_level_,
        po::value<int>(),
        "Log verbosity. Valid values are -1 through 5. Higher numbers are more "
        "verbose. Default value is 0");
    options().add_options()(
        all_,
        "Enable all supported blockchains. Seed nodes still be set by passing "
        "the option for the appropriate chain.");

    for (const auto& chain : ot::blockchain::SupportedChains()) {
        auto ticker = ot::blockchain::TickerSymbol(chain);
        auto message = std::stringstream{};
        message << "Enable " << ot::blockchain::DisplayString(chain)
                << " blockchain.\nOptionally specify ip address of seed node "
                   "or \"off\" to disable";
        options().add_options()(
            lower(ticker).c_str(),
            po::value<std::string>()->implicit_value(""),
            message.str().c_str());
    }

    try {
        po::store(po::parse_command_line(argc, argv, options()), variables());
        po::notify(variables());

        return true;
    } catch (po::error& e) {
        std::cerr << "ERROR: " << e.what() << "\n\n" << options() << std::endl;

        return false;
    }
}

auto variables() noexcept -> po::variables_map&
{
    if (nullptr == variables_) { variables_ = new po::variables_map; }

    return *variables_;
}
