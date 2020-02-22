# Transient build container
FROM alpine:latest AS builder

# Don't need "apk update" with "--no-cache".
RUN \
echo "**** install build dependencies ****" && \
apk add --no-cache bash util-linux openssl-dev sqlite-dev \
	build-base cmake gcc git make && \
echo "**** create directories ****" && \
mkdir /app /app/bin /app/conf /app/lib /app/man /app/man/man1

WORKDIR /uhub

# Hardening flags for compilation
ENV CFLAGS="-Wformat -Werror=format-security -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fPIE -pie -Wall -O2 -DALLOW_NICK_CHANGE -fstack-check"
ENV LDFLAGS="-Wl,-z,relro"

ARG BUILD=Release

# When building from the uhub repository, just copy it in
COPY . .
# When building from outside the uhub repository, grab the source
#RUN git clone https://github.com/janvidar/uhub.git /uhub

# Actually store the plugins in /app/lib, but we install a symlink from
# /libs -> /app/lib and from /usr/lib/uhub -> /app/lib to make configuration
# a bit easier and to allow existing configs to work with the container.
# /etc/uhub is also symlinked to /conf for ease of use

# Put everything in /app so it's self-contained and can be copied to the
# second-stage image in 1 layer.
# Gzipped man pages are only ~2k, so keep them in the final image.
# Use /app/conf for configs here so the start script can install missing
# configs on the first fun.
RUN \
echo "**** configure uhub ****" && \
cmake . \
	-DCMAKE_INSTALL_PREFIX=/app \
	-DCMAKE_INSTALL_MANDIR=man \
	-DPLUGIN_DIR=/libs \
	-DCONFIG_DIR=/conf \
	-DLOG_DIR=/conf \
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	-DCMAKE_BUILD_TYPE="${BUILD}" && \
echo "**** build uhub ****" && \
make -j2 && \
echo "**** run self-tests ****" && \
./autotest-bin && \
echo "**** install uhub ****" && \
make install && \
echo "**** compress man pages ****" && \
gzip /app/man/*/*.? && \
echo "**** move configs into place ****" && \
mv -v /conf/* /app/conf/ && \
echo "**** move plugins into place ****" && \
mv -v /libs/* /app/lib/ && \
echo "**** move start script into place ****" && \
mv -v tools/start-docker.sh /app/bin/start-uhub.sh


# Actual production container
FROM alpine:latest
# Add symlinks to /app/lib and /conf to work with existing configs
RUN \
echo "**** install dependencies ****" && \
apk add --no-cache bash pwgen util-linux openssl-dev sqlite-dev && \
echo "**** remove unnecessary header files ****" && \
rm -rf /usr/include /usr/lib/pkgconfig && \
echo "**** create config directory ****" && \
mkdir /conf && \
echo "**** create plugin symlinks ****" && \
ln -sv app/lib /libs && \
ln -sv /app/lib /usr/lib/uhub && \
echo "**** create config symlink ****" && \
ln -sv /conf /etc/uhub

VOLUME /conf

# So that "man uhub" and "man uhub-passwd" work in the container
ENV MANPATH=/app/man
# so that "uhub-passwd" works in the container without the full path
ENV PATH="/app/bin:${PATH}"

# Another way to override the uhub command-line arguments
ENV UHUBOPT=

# The admin/pass vars ONLY affect the first run (if there is no /conf/uhub.conf)
ENV UHUB_ADMIN=admin
# If UHUB_PASS is empty, it will be randomly generated.
# Check /conf/admin.txt for the password and how to change it.
ENV UHUB_PASS=

EXPOSE 1151

ENTRYPOINT ["/app/bin/start-uhub.sh"]
CMD []

ARG BUILD=Release
LABEL build_type=${BUILD}

COPY --from=builder /app /app

# Set workdir after copying /app so docker doesn't add another layer
WORKDIR /app
