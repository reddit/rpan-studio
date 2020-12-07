FROM ubuntu:18.04
ARG QT_VERSION=5.15.2
ARG QT_MODULES=
ARG QT_HOST=linux
ARG QT_TARGET=desktop
ARG QT_ARCH=

ENV QTDIR /usr/local/Qt/${QT_VERSION}/gcc_64
ENV PATH="/usr/local/Qt/${QT_VERSION}/gcc_64/bin/:/usr/local/Qt/Tools/QtCreator/bin:${PATH}"
ENV QML2_IMPORT_PATH /usr/local/Qt/${QT_VERSION}/gcc_64/qml/

RUN apt update && \
    apt install -y  curl \
                    libdbus-1-3 \
                    libexpat1 \
                    libfontconfig1 \
                    libfreetype6 \
                    libgl1-mesa-glx \
                    libglib2.0-0 \
                    libx11-6 \
                    libx11-xcb1 \
                    libxcomposite-dev \
                    libxcb-composite0-dev \
                    libxcb-shm0-dev \
                    libxinerama-dev \
                    libxcb-xinerama0-dev \
                    python3-pip \
                    software-properties-common \
                    libavcodec-dev \
                    libavdevice-dev \
                    libavformat-dev \
                    libcurl4-openssl-dev \
                    zlib1g-dev \
                    libx264-dev \
                    libjack-dev \
                    libpulse-dev \
                    libasound-dev \
                    libmbedtls-dev \
                    libfdk-aac-dev \
                    libvlc-dev \
                    libv4l-dev \
                    libudev-dev \
                    libspeexdsp-dev \
                    libfreetype6-dev \
                    libfontconfig1-dev \
                    swig \
                    libpython3-dev \
                    libluajit-5.1-dev \
                    wget \
                    freeglut3 \
                    libegl1-mesa \
                    libcups2 \
                    libglu1-mesa-dev \
                    libxrender1 \
                    libxkbcommon-x11-0 \
                    g++ g++-5 g++-6 g++-7 g++-8 \
                    gcc gcc-5 gcc-6 gcc-7 gcc-8 \
                    build-essential \
                    git \
                    pkg-config \
                    qbs \
                    xvfb && \
    add-apt-repository -y 'ppa:ubuntu-toolchain-r/test' && \
    apt update && \
    apt install -y gcc-9 g++-9 && \
    pip3 install aqtinstall && \
    aqt install --outputdir /usr/local/Qt ${QT_VERSION} ${QT_HOST} ${QT_TARGET} ${QT_ARCH} -m ${QT_MODULES} && \
    curl -L -O 'https://github.com/Kitware/CMake/releases/download/v3.13.2/cmake-3.13.2-Linux-x86_64.sh' && \
    chmod a+x cmake-3.13.2-Linux-x86_64.sh && \
    ./cmake-3.13.2-Linux-x86_64.sh --prefix=/usr --skip-license

