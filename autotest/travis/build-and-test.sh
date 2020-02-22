#!/bin/sh

set -x
set -e

export CFLAGS="$(dpkg-buildflags --get CFLAGS) $(dpkg-buildflags --get CPPFLAGS)"
export LDFLAGS="$(dpkg-buildflags --get LDFLAGS) -Wl,--as-needed"

mkdir -p builddir
cd builddir

CMAKEOPTS="..
           -DCMAKE_INSTALL_PREFIX=/usr"

if [ "${CONFIG}" = "full" ]; then
    CMAKEOPTS="${CMAKEOPTS}
               -DCMAKE_BUILD_TYPE=Debug
               -DLOWLEVEL_DEBUG=ON
               -DSSL_SUPPORT=ON
               -DUSE_OPENSSL=ON
               -DADC_STRESS=ON"
else
    CMAKEOPTS="${CMAKEOPTS}
               -DCMAKE_BUILD_TYPE=Release
               -DLOWLEVEL_DEBUG=OFF
               -DSSL_SUPPORT=OFF
               -DADC_STRESS=OFF"
fi


cmake ${CMAKEOPTS} \
      -DCMAKE_C_FLAGS="${CFLAGS}" \
      -DCMAKE_EXE_LINKER_FLAGS="${LDFLAGS}"
make VERBOSE=1


make VERBOSE=1 autotest-bin
./autotest-bin


sudo make install
du -shc /etc/uhub/ /usr/bin/uhub* /usr/lib/uhub/

