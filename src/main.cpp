// Copyright (c) 2019-2020 The Open-Transactions developers
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <opentxs/opentxs.hpp>

#include <boost/program_options.hpp>

namespace ot = opentxs;
namespace po = boost::program_options;

void cleanup_globals();
po::variables_map& variables();
po::options_description& options();

static po::variables_map* variables_{};
static po::options_description* options_{};

po::variables_map& variables()
{
    if (nullptr == variables_) { variables_ = new po::variables_map; }

    return *variables_;
}

po::options_description& options()
{
    if (nullptr == options_) {
        options_ = new po::options_description{"Options"};
    }

    return *options_;
}

void cleanup_globals()
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

void process_arguments(
    int argc,
    char* argv[],
    std::map<ot::blockchain::Type, std::string>& chains);
void read_options(int argc, char** argv);

int main(int argc, char* argv[])
{
    std::map<ot::blockchain::Type, std::string> chains{};
    read_options(argc, argv);
    process_arguments(argc, argv, chains);
    const auto args = ot::ArgList{
        {OPENTXS_ARG_HOME, {ot::api::Context::SuggestFolder("otblockchain")}},
    };
    ot::Signals::Block();
    const auto& ot = ot::InitContext(args);
    const auto& client = ot.StartClient(args, 0);

    for (const auto& [chain, seed] : chains) {
        client.Blockchain().Start(chain, seed);
    }

    auto nyms = client.Wallet().LocalNyms();
    auto reason = client.Factory().PasswordPrompt("Blockchain operation");

    if (nyms.empty()) {
        client.Wallet().Nym(reason);
        nyms = client.Wallet().LocalNyms();
    }

    OT_ASSERT(false == nyms.empty())

    for (const auto& [chain, seed] : chains) {
        for (const auto& nym : nyms) {
            if (0 == client.Blockchain().AccountList(nym, chain).size()) {
                client.Blockchain().NewHDSubaccount(
                    nym, ot::BlockchainAccountType::BIP44, chain, reason);
            }
        }
    }

    ot.HandleSignals();
    ot::Join();
    cleanup_globals();

    return 0;
}

void process_arguments(
    [[maybe_unused]] int argc,
    [[maybe_unused]] char* argv[],
    std::map<ot::blockchain::Type, std::string>& chains)
{
    std::string seed{};

    for (const auto& [name, value] : variables()) {
        if (name == "btc") {
            seed = value.as<std::string>();
            chains[ot::blockchain::Type::Bitcoin] = seed;
        } else if (name == "tnbtc") {
            seed = value.as<std::string>();
            chains[ot::blockchain::Type::Bitcoin_testnet3] = seed;
        }
        if (name == "bch") {
            seed = value.as<std::string>();
            chains[ot::blockchain::Type::BitcoinCash] = seed;
        } else if (name == "tnbch") {
            seed = value.as<std::string>();
            chains[ot::blockchain::Type::BitcoinCash_testnet3] = seed;
        }
    }
}

void read_options(int argc, char** argv)
{
    options().add_options()(
        "btc",
        po::value<std::string>()->implicit_value(""),
        "Start Bitcoin blockchain")(
        "tnbtc",
        po::value<std::string>()->implicit_value(""),
        "Start Bitcoin testnet3 blockchain")(
        "bch",
        po::value<std::string>()->implicit_value(""),
        "Start Bitcoin Cash blockchain")(
        "tnbch",
        po::value<std::string>()->implicit_value(""),
        "Start Bitcoin Cash testnet3 blockchain");

    try {
        po::store(po::parse_command_line(argc, argv, options()), variables());
        po::notify(variables());
    } catch (po::error& e) {
        std::cerr << "ERROR: " << e.what() << "\n\n" << options() << std::endl;
    }
}
