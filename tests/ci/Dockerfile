FROM base:image

RUN mkdir -p /build/tests/ci

WORKDIR /build

ENV CIRRUS_CI=true CIRRUS_OS=linux CIRRUS_BASE_BRANCH=master DEBIAN_FRONTEND=noninteractive

#PRE

COPY tests/ci/install-build-depends.sh tests/ci/

# Dockerfile
ARG CONFIG
ENV CONFIG=${CONFIG:-minimal}

ARG CC
ENV CC=${CC}

RUN \
echo "**** install build dependencies ****" && \
bash ./tests/ci/install-build-depends.sh

COPY . .

ENV CFLAGS="-Wno-unused-parameter -Werror -Wall -Wextra"

ARG PREPARE
ENV PREPARE=${PREPARE:-0}

RUN \
test $PREPARE = 0 || exit 0; \
echo "**** build and test ****" && \
bash ./tests/ci/build-and-test.sh

# vi: syn=dockerfile :
