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
auto process_arguments(
    Enabled& enabled,
    Disabled& disabled,
    int& blockLevel,
    bool& help) noexcept -> void;
auto read_options(int argc, char** argv) noexcept -> void;
auto variables() noexcept -> po::variables_map&;

int main(int argc, char* argv[])
{
    auto enabled = Enabled{};
    auto disabled = Disabled{};
    auto blockLevel = int{0};
    auto help{false};
    read_options(argc, argv);
    process_arguments(enabled, disabled, blockLevel, help);
    const auto args = ot::ArgList{
        {OPENTXS_ARG_BLOCK_STORAGE_LEVEL, {std::to_string(blockLevel)}},
        {OPENTXS_ARG_HOME, {ot::api::Context::SuggestFolder("otblockchain")}},
        {OPENTXS_ARG_DISABLED_BLOCKCHAINS, disabled},
    };

    if (help) {
        std::cout << options() << "\n";

        return 0;
    }

    ot::Signals::Block();
    const auto& ot = ot::InitContext(args);
    ot.HandleSignals();
    const auto& client = ot.StartClient(args, 0);

    auto nyms = client.Wallet().LocalNyms();
    auto reason = client.Factory().PasswordPrompt("Blockchain operation");

    if (nyms.empty()) {
        client.Wallet().Nym(reason);
        nyms = client.Wallet().LocalNyms();
    }

    OT_ASSERT(false == nyms.empty())

    auto cache =
        std::map<ot::proto::ContactItemType, std::atomic<std::int64_t>>{};
    auto nymID = client.Factory().NymID();
    auto address = std::string{};

    for (const auto& [chain, seed] : enabled) {
        client.Blockchain().Enable(chain, seed);

        for (const auto& nym : nyms) {
            nymID = nym;
            auto accounts = client.Blockchain().AccountList(nym, chain);

            if (0 == accounts.size()) {
                client.Blockchain().NewHDSubaccount(
                    nym, ot::BlockchainAccountType::BIP44, chain, reason);
                accounts = client.Blockchain().AccountList(nym, chain);
            }

            OT_ASSERT(0 < accounts.size())

            const auto& id = *accounts.begin();
            const auto& account = client.Blockchain().HDSubaccount(nym, id);
            const auto& first = account.BalanceElement(
                ot::api::client::blockchain::Subchain::External, 0);
            address =
                first.Address(ot::api::client::blockchain::AddressStyle::P2PKH);
            ot::LogNormal("First receiving address: ")(address).Flush();

            {
                auto& widget = client.UI().AccountList(nym);
                widget.SetCallback([&]() {
                    auto print = [&](const auto& row) {
                        auto& previous = cache[row->Unit()];
                        auto current = row->Balance();

                        if (previous != current) {
                            ot::LogOutput(row->NotaryName())(": ")(
                                row->DisplayBalance())
                                .Flush();
                        }

                        previous = current;
                    };
                    auto row = widget.First();

                    if (false == row->Valid()) { return; }

                    print(row);

                    while (false == row->Last()) {
                        row = widget.Next();
                        print(row);
                    }
                });
            }
        }
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

constexpr auto help_{"help"};
constexpr auto block_storage_{"block_storage"};

auto process_arguments(
    Enabled& enabled,
    Disabled& disabled,
    int& blockLevel,
    bool& help) noexcept -> void
{
    auto map = std::map<std::string, Type>{};

    for (const auto& chain : ot::blockchain::SupportedChains()) {
        auto ticker = ot::blockchain::TickerSymbol(chain);
        lower(ticker);
        map.emplace(std::move(ticker), chain);
    }

    auto seed = std::string{};

    for (const auto& [name, value] : variables()) {
        if (name == help_) {
            help = true;
        } else if (name == block_storage_) {
            try {
                blockLevel = value.as<int>();
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
}

auto read_options(int argc, char** argv) noexcept -> void
{
    options().add_options()(help_, "Display this message");
    options().add_options()(
        block_storage_,
        po::value<int>(),
        "Block persistence level.\n    0: do not save any blocks\n    1: save "
        "blocks downloaded by the wallet\n    2: download and save all blocks");

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
    } catch (po::error& e) {
        std::cerr << "ERROR: " << e.what() << "\n\n" << options() << std::endl;
    }
}

auto variables() noexcept -> po::variables_map&
{
    if (nullptr == variables_) { variables_ = new po::variables_map; }

    return *variables_;
}
