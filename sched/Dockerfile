FROM ubuntu:18.04

RUN DEBIAN_FRONTEND=noninteractive \
        apt-get update && \
        apt-get -y install tzdata

RUN apt-get -y install \
        git make gdb python3-dev \
        libbsd-dev libc6-dev gcc-multilib linux-libc-dev \
        seabios qemu-system-x86

RUN apt-get -y install wget vim

RUN wget http://launchpadlibrarian.net/508305356/qemu-system-x86_2.11+dfsg-1ubuntu7.34_amd64.deb && \
    dpkg -i qemu-system-x86_2.11+dfsg-1ubuntu7.34_amd64.deb && \
    rm -rf qemu-system-x86_2.11+dfsg-1ubuntu7.34_amd64.deb

COPY . /sched

WORKDIR /sched
