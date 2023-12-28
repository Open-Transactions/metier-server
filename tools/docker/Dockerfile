ARG DOWNLOAD_ALPINE_VERSION="3.18.5"
ARG METIER_SERVER_REPO="https://github.com/Open-Transactions/metier-server"
ARG OPENTXS_VERSION="latest"
ARG OPENTXS_BUILD_URL="opentransactions/alpine-sdk:$OPENTXS_VERSION"
ARG OPENTXS_RUN_URL="opentransactions/alpine-runtime:$OPENTXS_VERSION"

FROM alpine:${DOWNLOAD_ALPINE_VERSION} AS download
RUN mkdir -p /usr/src
RUN --mount=type=tmpfs,target=/var/cache/apk apk add \
    bzip2 \
    git \
    lzo \
    p7zip \
    rsync \
    wget \
    xz

FROM download AS metier_server-download
ARG METIER_SERVER_REPO
ARG METIER_SERVER_COMMIT
RUN mkdir -p /usr/src && git clone --recursive "${METIER_SERVER_REPO}" /usr/src/metier-server && cd /usr/src/metier-server && git reset --hard "${METIER_SERVER_COMMIT}"

FROM $OPENTXS_BUILD_URL AS compile
COPY --from=metier_server-download /usr/src/metier-server /usr/src/metier-server
RUN --mount=type=tmpfs,target=/tmp/metier-server /usr/bin/cmake \
        -S /usr/src/metier-server \
        -B /tmp/metier-server \
        -GNinja \
        -DCMAKE_C_FLAGS="-O2" \
        -DCMAKE_CXX_FLAGS="-O2" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_UNITY_BUILD=ON \
        -DCMAKE_UNITY_BUILD_BATCH_SIZE=0 \
    && /usr/bin/cmake --build /tmp/metier-server \
    && /usr/bin/cmake --install /tmp/metier-server

FROM $OPENTXS_RUN_URL AS run
COPY --from=compile /usr/local/bin/metier-server /usr/bin/metier-server
ENTRYPOINT [ "/usr/bin/metier-server", "--data_dir=/var/lib/metier" ]
CMD []
