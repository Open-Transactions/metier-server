ARG METIER_SERVER_REPO="https://github.com/Open-Transactions/metier-server"
ARG OPENTXS_VERSION="latest"
ARG OPENTXS_BUILD_URL="opentransactions/fedora-devel:$OPENTXS_VERSION"
ARG OPENTXS_RUN_URL="opentransactions/fedora:$OPENTXS_VERSION"

FROM fedora:39 AS base

FROM base AS git
RUN --mount=type=tmpfs,target=/var/cache/dnf dnf install -y git

FROM git AS download
ARG METIER_SERVER_REPO
ARG METIER_SERVER_COMMIT
RUN mkdir -p /usr/src && git clone --recursive "${METIER_SERVER_REPO}" /usr/src/metier-server && cd /usr/src/metier-server && git reset --hard "${METIER_SERVER_COMMIT}"

ARG OPENTXS_BUILD_URL
FROM $OPENTXS_BUILD_URL AS compile
RUN --mount=type=tmpfs,target=/var/cache/dnf dnf install -y \
    boost-devel \
    boost-devel \
    cmake \
    gcc-c++ \
    libsodium-devel \
    lmdb-devel \
    ninja-build \
    openssl-devel \
    protobuf-compiler \
    protobuf-lite-devel \
    tbb-devel \
    zeromq-devel \
    zlib-devel

COPY --from=download /usr/src/metier-server /usr/src/metier-server
RUN mkdir -p /tmp/metier-server && cd /tmp/metier-server \
    && cmake \
        -GNinja \
        -DCMAKE_C_FLAGS="-O2" \
        -DCMAKE_CXX_FLAGS="-O2" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_UNITY_BUILD=ON \
        -DCMAKE_UNITY_BUILD_BATCH_SIZE=0 \
        /usr/src/metier-server \
    && cmake --build . \
    && cmake --install . \
    && rm -rf /tmp/metier-server*

ARG OPENTXS_RUN_URL
FROM $OPENTXS_RUN_URL AS run
COPY --from=compile /usr/local/bin/metier-server /usr/bin/metier-server
ENTRYPOINT [ "/usr/bin/metier-server", "--data_dir=/var/lib/metier" ]
CMD []
