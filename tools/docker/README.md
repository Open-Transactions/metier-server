# Métier-server Docker deployment image

## Usage

### Building the image

```
docker image build --build-arg "METIER_SERVER_COMMIT=<tag or commit hash>" .
```

This image is available on Docker Hub as [opentransactions/metier-server
](https://hub.docker.com/r/opentransactions/metier-server)

To use a specific version of libopentxs add ```--build-arg "OPENTXS_VERSION=<desired tag>"``` where the desired tag is one of the versions listed for [opentransactions/fedora](https://hub.docker.com/r/opentransactions/fedora/tags). The default value is "latest".

### Running the image

#### Configuration

A configuration directory should be mounted inside the image at /etc/metier

This directory should contain metier-server.conf and optionally otdht.json

The specified user should be able to read /etc/metier-server/metier-server.conf

/etc/metier-server/otdht.json will be created after the first run of the image

#### Storage

Storage should be mounted inside the image at /srv/metier-server and must be owned by the specified user

Optionally bulk storage may be mounted inside the image at /srv/metier-server/client_data/blockchain/common/blocks

#### Example

```
docker run \
    --read-only \
    --mount type=bind,src=/mnt/configuration,dst=/etc/metier \
    --mount type=bind,src=/mnt/ssd-storage,dst=/srv/metier-server \
    --mount type=bind,src=/mnt/hdd-storage,dst=/srv/metier-server/client_data/blockchain/common/blocks \
    --user 1000:1000 \
    --ulimit nofile=262144:262144 \
    -p 8814:8814/tcp \
    -p 8815:8815/tcp \
    -p 8816:8816/tcp \
    opentransactions/metier-server:latest
```
