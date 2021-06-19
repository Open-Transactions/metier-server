metier-server
==============

### Build Instructions

metier-server uses the CMake build system. The basic steps are:

    mkdir build
    cd build
    cmake ..
    make
    make install

This assumes you have [opentxs](https://github.com/Open-Transactions/opentxs)
installed and available on the system.


### Usage

```
Options:
  --help                                Display this message
  --data_dir arg (=$HOME/.metier-server/)
                                        Path to data directory
  --block_storage arg                   Block persistence level.
                                            0: do not save any blocks
                                            1: save blocks downloaded by the
                                        wallet
                                            2: download and save all blocks
  --sync_server arg                     Starting TCP port to use for sync
                                        server. Two ports will be allocated.
                                        Implies --block_storage=2
  --public_addr arg                     IP address or domain name where clients
                                        can connect to reach the sync server.
                                        Mandatory if --sync_server is
                                        specified.
  --log_level arg                       Log verbosity. Valid values are -1
                                        through 5. Higher numbers are more
                                        verbose. Default value is 0
  --all                                 Enable all supported blockchains. Seed
                                        nodes still be set by passing the
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
```

### Dependencies

[Open Transactions library](https://github.com/Open-Transactions/opentxs)

### Docker

A docker image is available in the [opentxs-docker](https://github.com/Open-Transactions/docker/tree/master/metier-server) repository.
