# BUILD redisfab/redisbloom:${VERSION}-${ARCH}-${OSNICK}

ARG REDIS_VER=6.2.2

# OSNICK=focal|bionic|xenial|stretch|buster
ARG OSNICK=buster

# OS=debian:buster-slim|debian:stretch-slim|ubuntu:bionic
ARG OS=debian:buster-slim

# ARCH=x64|arm64v8|arm32v7
ARG ARCH=x64

ARG PACK=0
ARG TEST=0

#----------------------------------------------------------------------------------------------
FROM redisfab/redis:${REDIS_VER}-${ARCH}-${OSNICK} AS redis
FROM ${OS} AS builder

ARG OSNICK
ARG OS
ARG ARCH
ARG REDIS_VER

RUN echo "Building for ${OSNICK} (${OS}) for ${ARCH}"

WORKDIR /build
COPY --from=redis /usr/local/ /usr/local/

ADD . /build

RUN ./deps/readies/bin/getpy3
RUN ./system-setup.py
RUN bash -l -c "make all -j"
RUN bash -l -c "make test"

ARG PACK
ARG TEST

RUN mkdir -p bin/artifacts
RUN set -e ;\
    if [ "$PACK" = "1" ]; then bash -l -c "make pack"; fi
RUN set -e ;\
    if [ "$TEST" = "1" ]; then \
        bash -l -c "TEST= make test" ;\
		rm -f /build/tests/flow/logs/*.rdb ;\
        tar -C /build/tests/flow/logs/ -czf /build/bin/artifacts/tests-flow-logs-${ARCH}-${OSNICK}.tgz . ;\
    fi

#----------------------------------------------------------------------------------------------
FROM redisfab/redis:${REDIS_VER}-${ARCH}-${OSNICK}

ARG OSNICK
ARG OS
ARG ARCH
ARG REDIS_VER
ARG PACK

ENV LIBDIR /usr/lib/redis/modules
WORKDIR /data
RUN mkdir -p "$LIBDIR"

RUN mkdir -p /var/opt/redislabs/artifacts
RUN chown -R redis:redis /var/opt/redislabs
COPY --from=builder /build/bin/artifacts/ /var/opt/redislabs/artifacts

COPY --from=builder /build/redisbloom.so "$LIBDIR"

EXPOSE 6379
CMD ["redis-server", "--loadmodule", "/usr/lib/redis/modules/redisbloom.so"]
