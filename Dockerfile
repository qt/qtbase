# !! Only run this if you agree with Qt's license - -confirm-license -opensource will accept the license quietly. !!

# Docker Build: docker build -t qt6 .
# Running this image with GPUs requires Nvidia Docker.
# Docker Run: docker run -it --rm --gpus all -v /tmp/.X11-unix:/tmp/.X11-unix -v "${XAUTHORITY}":/tmp/.docker.xauth
#                        -e XAUTHORITY=/tmp/.docker.xauth -e DISPLAY=$DISPLAY qt6 bash
# If X passthrough fails: rm -rf /tmp/dockerxauth 2> /dev/null && touch /tmp/dockerxauth &&
#                         xauth nlist $DISPLAY | sed -e 's/^..../ffff/' | xauth -f /tmp/dockerxauth nmerge -

FROM nvidia/opengl:1.0-glvnd-runtime-ubuntu20.04

LABEL maintainer="The Qt Company"

ARG QT_VERSION_SHORT=6.0
ARG QT_VERSION_LONG=${QT_VERSION_SHORT}.1

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/New_York

ENV QT_BASE_DIR="/opt/qt6"
ENV QT_SELECT=${QT_VERSION_SHORT}
ENV QTLIBDIR="${QT_BASE_DIR}/lib"
ENV QTTOOLDIR="${QT_BASE_DIR}/bin"
ENV PATH="${QT_BASE_DIR}/bin/:$PATH"

# Docs:         https://doc.qt.io/qt-6/linux.html
# Requirements: https://doc.qt.io/qt-6/linux-requirements.html
RUN apt-get update && \
    apt-get install -y \
    build-essential clang libclang-dev cmake gcc g++ wget \
    dbus \
    qtchooser \
    python3 python3-pip \
    libgl1-mesa-dev libglu1-mesa-dev libgssapi-krb5-2 \
    libfontconfig1-dev \
    libfreetype6-dev \
    libx11-dev libx11-xcb-dev \
    libxcb-keysyms1-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev libxcb-sync0-dev libxcb-xfixes0-dev \
    libxcb-shape0-dev libxcb-randr0-dev libxcb-render-util0-dev libxcb-glx0-dev libxcb-xinerama0 \
    libxcb1-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxkbcommon-x11-dev \
    libxrender-dev && \
    rm -rf /var/lib/apt/lists/*

# Releases: https://download.qt.io/official_releases/qt/
# Add -static if you would like to build statically - please pay special attention to licensing if you do
# Should be able to use cmake --build . --parallel instead of make if desired
RUN cd /tmp && wget https://download.qt.io/official_releases/qt/${QT_VERSION_SHORT}/${QT_VERSION_LONG}/single/qt-everywhere-src-${QT_VERSION_LONG}.tar.xz && \
    tar -xf qt-everywhere-src-${QT_VERSION_LONG}.tar.xz && tar -xf qt-everywhere-src-${QT_VERSION_LONG}.tar.xz && \
    rm qt-everywhere-src-${QT_VERSION_LONG}.tar.xz && \
    cd /tmp/qt-everywhere-src-${QT_VERSION_LONG} && export QT6PREFIX=${QT_BASE_DIR} && mkdir ${QT_BASE_DIR} && \
    ./configure -prefix ${QT6PREFIX} -release -confirm-license -opensource -prefix ${QT_BASE_DIR} && \
    make -j8 && make install && echo "export PATH=${QT_BASE_DIR}/bin/:\${PATH}" >> /root/.bashrc

RUN python3 -m pip install -U pip && python3 -m pip install pyside6
