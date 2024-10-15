#!/bin/sh

METIER_ROOT="/srv/metier-server"

mkdir -p "${METIER_ROOT}/client_data/blockchain/common/blocks"
touch "${METIER_ROOT}/client_data/blockchain/version.3"

if [ -e "/etc/metier-server/otdht.json" ] ; then
    if [ ! -e "${METIER_ROOT}/client_data/otdht.json" ] ; then
        echo "Using existing otdht.json from configuration directory:"
        cp -v "/etc/metier-server/otdht.json" "${METIER_ROOT}/client_data/otdht.json"
    fi
fi

if [ -e "/etc/metier-server/metier-server.conf" ] ; then
    echo "using configuration from /etc/metier-server/metier-server.conf:"
    cat "/etc/metier-server/metier-server.conf"
    source "/etc/metier-server/metier-server.conf"
else
    echo "Missing configuration file"

    exit 1
fi

/usr/bin/metier-server \
    --data_dir=${METIER_ROOT} \
    --sync_server=${START_PORT} \
    --public_addr=${PUBLIC_IP} \
    --log_level=${LOG_LEVEL} \
    --ipv4_connection_mode=${IPV4} \
    --ipv6_connection_mode=${IPV6} \
    $CHAINS

if [ ! -e "/etc/metier-server/otdht.json" ] ; then
    echo "Performing one-time backup of otdht.json:"
    cp -v "${METIER_ROOT}/client_data/otdht.json" "/etc/metier-server/otdht.json"
fi
