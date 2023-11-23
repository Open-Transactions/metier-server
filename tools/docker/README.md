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

Persistent storage must be mounted at /var/lib/metier inside the image and the ```--user``` argument to ```docker run``` should be set to match the ownership of this directory.

#### Examples

```
docker run \
    --read-only \
    --mount type=bind,src=/var/lib/metier,dst=/metier \
    --user 1000:1000 \
    --ulimit nofile=262144:262144 \
    -p 8814:8814/tcp \
    -p 8815:8815/tcp \
    -p 8816:8816/tcp \
    opentransactions/metier-server:latest \
    --sync_server=8814 \
    --public_addr=127.0.0.1 \
    --all --bsv=off --tnbsv=off --tndash=off
```

```
docker run \
    --read-only \
    --mount type=bind,src=/var/lib/metier,dst=/metier \
    --user 1000:1000 \
    --ulimit nofile=262144:262144 \
    -p 8814:8814/tcp \
    -p 8815:8815/tcp \
    -p 8816:8816/tcp \
    opentransactions/metier-server:latest \
    --sync_server=8814 \
    --public_addr=127.0.0.1 \
    --btc=<seed node>
```
