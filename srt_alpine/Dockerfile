FROM        alpine:3.5 AS base

RUN     apk  add --no-cache --update tcl openssl-dev pkgconfig gcc build-base cmake bash coreutils gcc ca-certificates
FROM        base AS build

# Compile SRT from sources ----------------

COPY srt_142/ /root/srt
RUN cd /root/srt/ && \
  ./configure --enable-experimental-bonding && \
  make

#RUN mkdir /stats

# Clean up
#RUN apk del .build-deps

WORKDIR /root/srt 
# Start
ENTRYPOINT [ "/root/srt/srt-live-transmit" ]

