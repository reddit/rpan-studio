#!/bin/bash

set -e

apt update

if [ ! -f /project/tmp/linuxdeployqt ] ; then
    wget https://github.com/probonopd/linuxdeployqt/releases/download/7/linuxdeployqt-7-x86_64.AppImage -O /project/tmp/linuxdeployqt
fi
if [ ! -x /project/tmp/linuxdeployqt ] ; then
    chmod +x /project/tmp/linuxdeployqt
fi
if [ ! -f /project/tmp/ldq/squashfs-root/AppRun ] ; then
    mkdir -p /project/tmp/ldq
    cd /project/tmp/ldq
    ../linuxdeployqt --appimage-extract
fi

if [ ! -f /project/tmp/appimagetool ] ; then
    wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage -O /project/tmp/appimagetool
fi
if [ ! -x /project/tmp/appimagetool ] ; then
    chmod +x /project/tmp/appimagetool
fi
if [ ! -f /project/tmp/ait/squashfs-root/AppRun ] ; then
    mkdir -p /project/tmp/ait
    cd /project/tmp/ait
    ../appimagetool --appimage-extract
fi

cd /project/build
if [ "${CLEAN_BUILD}" == "1" ] ; then
    echo "CLEAN BUILD"
    rm -rf /project/build/*
fi

cmake ../src \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DREDDIT_HASH="${REDDIT_HASH}" \
        -DREDDIT_CLIENTID="${REDDIT_CLIENTID}" \
        -DREDDIT_SECRET="${REDDIT_SECRET}" \
        -DREDDIT_HMAC_GLOBAL_VERSION="${REDDIT_HMAC_GLOBAL_VERSION}" \
        -DREDDIT_HMAC_PLATFORM="${REDDIT_HMAC_PLATFORM}" \
        -DREDDIT_HMAC_TOKEN_VERSION="${REDDIT_HMAC_TOKEN_VERSION}" \
        -DOBS_VERSION_OVERRIDE="${OBS_VERSION_OVERRIDE}"

rm -rf /project/AppDir/*

make -j${BUILD_THREADS:-1}
make DESTDIR=/project/AppDir -j${BUILD_THREADS:-1} install

apt-get -y download libpython3.6-minimal libpython3.6-stdlib
( cd /project/AppDir ; dpkg -x /project/build/libpython3.6-minimal*deb . )
( cd /project/AppDir ; dpkg -x /project/build/libpython3.6-stdlib*deb . )
( cd /project/AppDir/usr ; ln -s lib/obs-scripting/* . )
( cd /project/AppDir/usr/lib/python3.6 ; ln -s plat-x86_64-linux-gnu/_sysconfigdata_m.py . )

sed -i -e 's|../../obs-plugins/64bit|././././lib/obs-plugins|g' /project/AppDir/usr/lib/libobs.so.0

rm -rf /project/AppDir/usr/share/metainfo

cp /project/scripts/{AppRun,rpan-studio.{desktop,png}} /project/AppDir/

/project/tmp/ldq/squashfs-root/AppRun \
                /project/AppDir/usr/bin/obs \
                -unsupported-allow-new-glibc \
                -bundle-non-qt-libs \
                -verbose=1 \
                -executable=/project/AppDir/usr/bin/obs-ffmpeg-mux \
                -executable=/project/AppDir/usr/lib/obs-plugins/decklink-ouput-ui.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/frontend-tools.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/image-source.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/linux-alsa.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/linux-decklink.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/linux-jack.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/linux-pulseaudio.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/linux-v4l2.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/obs-ffmpeg.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/obs-filters.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/obs-libfdk.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/obs-outputs.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/obs-transitions.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/obs-x264.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/rtmp-services.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/text-freetype2.so \
                -executable=/project/AppDir/usr/lib/obs-plugins/vlc-video.so \
                -executable=/project/AppDir/usr/lib/libobs-opengl.so.0

/project/tmp/ait/squashfs-root/AppRun \
                --comp=xz \
                -v \
                /project/AppDir \
                /project/build/rpan-studio.AppImage


