# REPLICA - Replicable Cellular Networking Experiments using ns-3

Developed by INESC TEC, as part of 6G-SANDBOX Open Call 2.

Copyright (C) 2024-2025, INESC TEC. `replica-example.cc` is licensed under the terms of the [GNU General Public License (GPL) v2](LICENSE).
The rest of the project files (datasets, experimental and simulation results as well as data, processing and plotting scripts, etc.) are licensed under the terms of INESC TEC CC-BY 4.0.

## Authors

This software is authored by:

* Tiago Ribeiro \[INESC TEC, Portugal\]
* Sérgio Silva \[INESC TEC, Portugal\]
* António Lopes \[INESC TEC, Portugal\]
* Guilherme Monteiro \[INESC TEC, Portugal\]
* Eduardo Nuno Almeida \[INESC TEC, Portugal\]
* Helder Fontes \[INESC TEC, Portugal\] (Principal Investigator) <helder.m.fontes@inesctec.pt>

## Description

Wireless networking R&D depends on experimentation to make realistic evaluations of networking solutions, as simulation is inherently a simplification of the real-world. However, despite more realistic, experimentation is limited in aspects where simulation excels, such as repeatability, reproducibility, and scalability.

Real wireless experiments may be difficult to repeat under the same conditions, especially the ones considering mobile endpoints. Even if repeatable, experiments may still be difficult to reproduce. Namely, other researchers may be unable to reproduce an experiment because either the testbed is i) unavailable – offline or busy running other experiments –, ii) or inaccessible at all, when the testbed is modified (i.e. experimental conditions change due to topology and hardware changes) or decommissioned. 6G-SANDBOX testbeds may also be affected by these limitations.

What if we could easily make available the same 6G-SANDBOX RF execution conditions among the research community, thereby enabling any experimental scenario to be easily repeatable and reproducible using the ns-3 simulator?

With a track record of more than 10 years and multiple scientific publications on simulation-experimentation synergy, as well as a recognized position in the ns-3 community, INESC TEC has been developing a trace-based approach that enables the recording and reproduction of past physical layer traces, such as received signal strength (RSS) on ns-3 network simulator. Until now, this approach has been validated on Wi-Fi. 6G-SANDBOX provides, now, the perfect opportunity to run this innovative approach for 5G/6G networks.

## 1. Project Status

**Completed**: Project concluded in 2025.

## 2. Technology Stack

* **Language**: Python 3, C++
* **Frameworks and Libraries**:
  * ns-3 Network Simulator (default: `ns-3.44`)
  * 5G NR module (`5g-lena-v4.0.y`)
  * XGBoost for ML models
  * NumPy, Pandas for data processing
* **Containerization**: Docker

## 3. Dependencies

### Core Dependencies

* ns-3 Network Simulator (version `ns-3.44` or compatible)
* ns-3 5G NR module (`5g-lena-v4.0.y`)
* Python 3.8+
* Docker (for containerized deployment)

Further dependencies are listed in [Install.md](Install.md).

## 4. Installation

This project contains files to build a Docker image containing the REPLICA simulation platform. The platform contains the [ns-3 Simulator](https://www.nsnam.org) and the ns-3 modules listed in the [Install.md](Install.md) file.

### Build Docker Image

To build the Docker image, run the following command:

```shell
docker build \
  [--build-arg NS3_VERSION=ns-3.44] \
  [--build-arg NS3_NR_VERSION=5g-lena-v4.0.y] \
  -t replica-ns3 .
```

In order to ensure that the Docker image is built with the latest third-party dependencies available, add the `--no-cache` flag to the command above. Otherwise, the Docker image can be generated with cached dependencies from previous builds.

By default, the Docker image is generated with the latest development version of ns-3. To use a specific version of ns-3, for instance `ns-3.44`, add the flag `--build-arg NS3_VERSION=ns-3.44` to the above command. To use a specific version of the ns-3 [5G NR module](https://gitlab.com/cttc-lena/nr), for instance `5g-lena-v4.0.y`, add the flag `--build-arg NS3_NR_VERSION=5g-lena-v4.0.y` to the above command.

### Run Docker Image

To run the Docker image, run the following command:

```shell
docker run -it -v .:/ns-3/scratch/replica replica-ns3
```

This command starts the REPLICA Docker image and mounts the current directory into the ns-3 `scratch/` folder. This enables running ns-3 simulations, editing the source-code and having access to the results directly in the host PC. In order to minimize the size of the Docker image, ns-3 is not pre-compiled. Please follow the official instructions to configure and build ns-3.

```shell
./ns3 configure
./ns3 build
```

## 5. Usage

The `replica-example` file is an ns-3 simulation scenario that represents a simple 5G network consisting of one User Equipment (UE) and one gNodeB (gNB). The primary goal is to evaluate the performance of different loss models under consistent conditions while comparing the results obtained from a real testbed.

In this experiments, the gNB was placed at 12.87 meters height from the ground, whereas the UE was 6 meters height. We tested tested scenarios for an euclidean distance between the nodes of 9 and 56 meters. Each experiment had a duration of 9 minutes (540 seconds), during each second, the simulation and the real testbed measure the throughput of the uplink and downlink connections, which were then compared to each other.

### Run an ns-3 Simulation

To run an ns-3 simulation, use the following command in the ns-3 main directory:

```shell
./ns3 run "replica-example \
  --lossModel=3gpp \
  --protocol=udp \
  --mode=downlink \
  --simulationTime=540 \
  --distance=9 \
  --verbose"
```

Arguments:

* `lossModel`: 3gpp, mlpl-xgb, trace-based, friis, fixed-rss
* `protocol`: tcp, udp
* `mode`: downlink, uplink, bidir
* `distance`: The distance between the UE and gNB. Any integer value is accepted.
* `simulationTime`: Simulation time

### Run all Simulations in Parallel

To run all simulations for all combinations of arguments, use the `run_simulations.py` Python script as follows:

```shell
python3 scratch/replica/run_simulations.py
```

## ML Propagation Loss Model

### ML Model Training

The scripts to train the ML models are located in the [ml_model/](https://gitlab.com/inesctec-ns3/ml-propagation-loss-model/-/tree/master/ml_model) directory. The documentation about training and running the ML models supporting the MLPL model are available in the following page:

[ML Model Training](https://gitlab.com/inesctec-ns3/ml-propagation-loss-model/-/blob/master/doc/ml-model-training.md)

**NOTE**: The examples and tests requires the ML models to be trained beforehand. To train the ML models with our dataset, run the following command, replacing the `--dataset-path` variable with the correspondent path in your system:

```shell
python3 ml_model/train_ml_propagation_loss_model.py \
  --dataset-path={ABSOLUTE_DIR_TO_NS3}/scratch/replica/datasets/replica-dataset \
  --mlpl-model=position \
  --ml-algorithm=xgb
```

### MLPL Model Usage in ns-3

The documentation about using the MLPL model in ns-3 is available in the following page:

[MLPL Usage in ns-3](https://gitlab.com/inesctec-ns3/ml-propagation-loss-model/-/blob/master/doc/mlpl-usage-ns3.md).

To run the our simulations with the MLPL model, first open a terminal to run the ML model as:

```shell
python3 ml_model/run_ml_propagation_loss_model.py \
  --dataset-path={ABSOLUTE_DIR_TO_NS3}/scratch/replica/datasets/replica-dataset \
  --mlpl-model=position \
  --ml-algorithm=xgb
```

Then run our simulation by specifying `--lossModel=mlpl-xgb`:

```shell
./ns3 run "replica-example \
  --lossModel=mlpl-xgb \
  --protocol=udp \
  --mode=downlink \
  --simulationTime=540 \
  --distance=9 \
  --verbose"
```

## 6. Contacts

Principal Investigator: Helder Fontes - <helder.m.fontes@inesctec.pt>

## 7. Acknowledgements

The REPLICA experiment has received funding in the context of the Open Call 2 of the 6G-SANDBOX project. 6G-SANDBOX has received funding from the Smart Networks and Services Joint Undertaking (SNS JU) under the European Union's Horizon Europe research and innovation programme under Grant Agreement No 101096328.

![6G-SANDBOX](images/6GSANDBOX.jpg)

![Logos](images/logos.jpg)
