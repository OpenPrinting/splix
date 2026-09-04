FROM debian:stable-slim

RUN sed -i 's/^Types: deb$/Types: deb deb-src/' \
        /etc/apt/sources.list.d/debian.sources \
    && apt-get update -qq \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y -qq \
        build-essential \
        cups-ppdc \
        dpkg-dev \
        libcups2-dev \
        pkg-config \
        python3 \
    && mkdir -p /tmp/jbig /opt/jbig \
    && cd /tmp/jbig \
    && apt-get source -qq jbigkit \
    && make -s -C jbigkit-*/libjbig libjbig85.a \
    && cp jbigkit-*/libjbig/libjbig85.a /opt/jbig/ \
    && rm -rf /tmp/jbig /var/lib/apt/lists/*

ENV LIBRARY_PATH=/opt/jbig
WORKDIR /src
