FROM ubuntu:bionic-20200630

LABEL org.opencontainers.image.authors="szohar@bnl.gov"

#build with
#docker build -t vitis:2020.1 --build-arg UID=`id --user` docker/ 

ARG UID

RUN adduser --disabled-password --gecos '' vitisuser && \
    echo "dash dash/sh boolean false" | debconf-set-selections && \
    DEBIAN_FRONTEND=noninteractive dpkg-reconfigure dash && \
    usermod -u ${UID} vitisuser

RUN \
  dpkg --add-architecture i386 && apt update && \
  DEBIAN_FRONTEND=noninteractive apt install -y \
  dialog \
  gawk \
  zlib1g \
  libncurses-dev \
  xterm \
  libtool \
  texinfo \
  zlib1g-dev \
  autoconf \
  lib32stdc++6 \
  libgtk2.0-0:i386 \
  libx11-6:i386 \
  libfontconfig1:i386 \
  libx11-6:i386 \
  libxext6:i386 \
  libxrender1:i386 \
  libsm6:i386 \
  libswt-gtk-4-jni \
  libswt-gtk-4-java \
  libqtgui4:i386 \
  libtinfo-dev \
  libboost-dev \ 
  software-properties-common \
  wget \
  libxtst6 \
  build-essential \
  libglib2.0-0 \
  libsm6 \
  libxi6 \
  libxrender1 \
  libxrandr2 \
  libfreetype6 \
  libfontconfig \
  git \ 
  net-tools \ 
  unzip \
  gcc \
  g++ \
  u-boot-tools \
  vim \
  rsync \
  gzip \
  unzip \
  chrpath \
  pax \
  sudo \
  apt-utils \
  gnupg \
  locales \
  gcc-multilib \
  tmux \
  screen \
  lsb-release \
  gdb \
  lib32stdc++6 \
  opencl-headers \
  x11-apps \
  python \
  python-pip \ 
  python-yaml \
  python3 \ 
  python3-pip \
  python3-yaml && \
  DEBIAN_FRONTEND=noninteractive add-apt-repository ppa:xorg-edgers/ppa -y && \
  DEBIAN_FRONTEND=noninteractive apt update && \
  DEBIAN_FRONTEND=noninteractive apt install -y \
  libgl1-mesa-glx \
  libgl1-mesa-dri \
  libgl1-mesa-dev && \
  DEBIAN_FRONTEND=noninteractive add-apt-repository --remove ppa:xorg-edgers/ppa && \
  rm -rf /var/lib/apt/lists/* && \
  locale-gen en_US.UTF-8 && update-locale && \
  usermod -aG sudo vitisuser && \
  echo "vitisuser ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers && \
  python3 -m pip install --upgrade pip

USER vitisuser
WORKDIR /home/vitisuser
ENV HOME=/home/vitisuser LANG=en_US.UTF-8
