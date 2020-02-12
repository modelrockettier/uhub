# Transient build container
FROM alpine:latest AS builder

# Don't need "apk update" with "--no-cache".
RUN \
echo "**** install build dependencies ****" && \
apk add --no-cache bash util-linux openssl-dev sqlite-dev \
	build-base cmake gcc git make && \
echo "**** create directories ****" && \
mkdir /app /app/bin /app/lib /app/man /app/man/man1 /copy

WORKDIR /uhub

ARG BUILD=Release

# When building from the uhub repository, just copy it in
COPY . .
# When building from outside the uhub repository, grab the source
#RUN git clone https://github.com/janvidar/uhub.git /uhub

# Actually store the plugins in /app/lib, but we install a symlink from
# /libs -> /app/lib and from /usr/lib/uhub -> /app/lib to make configuration
# a bit easier and to allow existing configs to work with the container.
# /etc/uhub is also symlinked to /conf for ease of use

# Move all the useful stuff into /copy so we can copy it all to the second
# stage in 1 layer
# Gzipped man pages are only ~2k, so keep them in the final image.
RUN \
echo "**** configure uhub ****" && \
cmake . \
	-DCMAKE_INSTALL_PREFIX=/app \
	-DCMAKE_INSTALL_MANDIR=man \
	-DPLUGIN_DIR=/libs \
	-DCONFIG_DIR=/conf \
	-DLOG_DIR=/conf \
	-DCMAKE_BUILD_TYPE="${BUILD}" && \
echo "**** build uhub ****" && \
make && \
echo "**** run self-tests ****" && \
./autotest-bin && \
echo "**** install uhub ****" && \
make install && \
echo "**** compress man pages ****" && \
gzip /app/man/*/*.? && \
echo "**** move plugins into place ****" && \
mv -v /libs/* /app/lib/ && \
echo "**** move uhub into place ****" && \
mv -v /app /conf /copy/


# Actual production container
FROM alpine:latest
# Add symlinks to /app/lib and /conf to work with existing configs
RUN \
echo "**** install dependencies ****" && \
apk add --no-cache bash pwgen util-linux openssl-dev sqlite-dev && \
echo "**** remove unnecessary header files ****" && \
rm -rf /usr/include /usr/lib/pkgconfig && \
echo "**** create plugin symlinks ****" && \
ln -sv app/lib /libs && \
ln -sv /app/lib /usr/lib/uhub && \
echo "**** create config symlink ****" && \
ln -sv /conf /etc/uhub

# So that "man uhub" and "man uhub-passwd" work in the container
ENV MANPATH=/app/man
# so that "uhub-passwd" works in the container without the full path
ENV PATH="/app/bin:${PATH}"

EXPOSE 1151

ENTRYPOINT ["/app/bin/uhub"]
CMD ["-c", "/conf/uhub.conf"]

COPY --from=builder /copy/ /

# Set workdir after copying /app so docker doesn't add another layer
WORKDIR /app

VOLUME /conf
