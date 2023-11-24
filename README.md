# Métier-server

Métier-server is a non-validating blockchain full node based on [libopentxs](https://github.com/Open-Transactions/opentxs) which operates on multiple networks simultaneously.

It provides additional features on top of the baseline P2P protocol of the respective reference implementations of the supported blockchains which support libopentxs-based wallet applications.

## Build Instructions

The recommended way of deploying Métier-server is via [Docker](tools/docker).

Métier-server may also be built standalone using the CMake build system. The basic steps are:

    mkdir build
    cd build
    cmake ..
    cmake --build .
    cmake --install .

## Usage

The ```--public_addr``` argument should be the ip address via which Métier-server will be reachable by its intended users. Other than for testing this usually should be a globally routable address.

If Métier-server crashes due to an inability to create sockets then the ```nofile``` ulimit is set too low.

Métier-server will listen on three tcp ports:

    * Two sequential ports beginning at the one specified with the --sync_server argument
    * Port 8816

The most current list of supported chains and their associated command line arguments, as well as the full list of libopentxs arguments, can be obtained by passing --help.


```
Metier-server options:
  --help                                Display this message
  --data_dir arg (=/home/user/.metier-server/)
                                        Path to data directory
  --sync_server arg                     Starting TCP port to use for sync
                                        server. Two ports will be allocated.
  --public_addr arg                     IP address or domain name where clients
                                        can connect to reach the sync server.
                                        Mandatory if --sync_server is
                                        specified.
  --all                                 Enable all supported blockchains. Seed
                                        nodes may still be set by passing the
                                        option for the appropriate chain.
  --btc arg                             Enable Bitcoin blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --tnbtc arg                           Enable Bitcoin (testnet3) blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --bch arg                             Enable Bitcoin Cash blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --tnbch arg                           Enable Bitcoin Cash (testnet3)
                                        blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --ltc arg                             Enable Litecoin blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --tnltc arg                           Enable Litecoin (testnet4) blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --pkt arg                             Enable PKT blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --bsv arg                             Enable Bitcoin SV blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --tnbsv arg                           Enable Bitcoin SV (testnet3)
                                        blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --xec arg                             Enable eCash blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --tnxec arg                           Enable eCash (testnet3) blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --tn4bch arg                          Enable Bitcoin Cash (testnet4)
                                        blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --dash arg                            Enable Dash blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
  --tndash arg                          Enable Dash (testnet3) blockchain.
                                        Optionally specify ip address of seed
                                        node or "off" to disable
```
