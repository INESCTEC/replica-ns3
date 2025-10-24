FROM ubuntu:latest

LABEL maintainer="INESC TEC"

RUN apt update && apt install -y \
    cmake \
    g++ \
    git \
    ninja-build \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Install ns-3 and its dependencies
ARG NS3_VERSION=ns-3.44
RUN git clone --depth=1 --branch=${NS3_VERSION} https://gitlab.com/nsnam/ns-3-dev.git ns-3

WORKDIR /ns-3/contrib

ARG NS3_MLPL_VERSION=1.0.2
RUN git clone --depth=1 --branch=${NS3_MLPL_VERSION} https://gitlab.com/inesctec-ns3/ml-propagation-loss-model.git \
    && pip3 install --break-system-packages -r ./ml-propagation-loss-model/requirements.txt

ARG NS3_NS3AI_VERSION=v1.2.0
RUN git clone --depth=1 --branch=${NS3_NS3AI_VERSION} https://github.com/hust-diangroup/ns3-ai.git \
    && pip3 install --break-system-packages ./ns3-ai/py_interface

ARG NS3_TBPL_VERSION=v0.1
RUN git clone --depth=1 --branch=${NS3_TBPL_VERSION} https://gitlab.com/inesctec-ns3/trace-based-propagation-loss-model.git

ARG NS3_NR_VERSION=5g-lena-v4.0.y
RUN git clone --depth=1 --branch=${NS3_NR_VERSION} https://gitlab.com/cttc-lena/nr.git

# Copy the REPLICA files
COPY . /ns-3/scratch/replica/

WORKDIR /ns-3
ENTRYPOINT [ "/bin/bash" ]
