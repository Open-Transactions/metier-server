otblockchain
==============

### Build Instructions

otblockchain uses the CMake build system. The basic steps are:

    mkdir build
    cd build
    cmake ..
    make
    make install

This assumes you have [opentxs](https://github.com/Open-Transactions/opentxs)
installed and available on the system.


### Usage

These commands will bring up otblockchain and connect to the specified chains using the specified initial peer. If an initial peer is not specified, localhost will be used.

    otblockchain --btc=a.b.c.d
    otblockchain --tnbch
    otblockchain --bch=a.b.c.d --tnbch=e.f.g.h --btc --tnbtc i.j.k.l

### Dependencies

[Open Transactions library](https://github.com/Open-Transactions/opentxs)
