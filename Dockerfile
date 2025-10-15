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

RUN git clone --depth=1 --branch=v1.2.0 https://github.com/hust-diangroup/ns3-ai.git
RUN git clone --depth=1 https://gitlab.com/inesctec-ns3/ml-propagation-loss-model.git \
    && pip3 install --break-system-packages -r ./ml-propagation-loss-model/requirements.txt \
    && pip3 install --break-system-packages ./ns3-ai/py_interface
RUN git clone --depth=1 https://gitlab.com/inesctec-ns3/trace-based-propagation-loss-model.git

ARG NS3_NR_VERSION=5g-lena-v4.0.y
RUN git clone --depth=1 --branch=${NS3_NR_VERSION} https://gitlab.com/cttc-lena/nr.git

# Copy the REPLICA files
COPY . /ns-3/scratch/replica/

WORKDIR /ns-3
ENTRYPOINT [ "/bin/bash" ]
