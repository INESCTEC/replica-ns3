# Installation to Run Simulations

This page explains how to install ns-3 and its dependencies to run simulations for REPLICA.

## ns-3 Setup

### Install ns-3

To set up ns-3, first download the source code from the website:

```shell
git clone --depth=1 --branch=ns-3.44 https://gitlab.com/nsnam/ns-3-dev.git
```

### ns-3 Dependencies

Then, download and configure the ns-3 modules needed in REPLICA. All modules are stored in the `contrib/` directory of ns-3.

```shell
cd contrib/
```

The following dependencies are needed:

* [nr - 5G LENA](https://gitlab.com/cttc-lena/nr):

  ```shell
  git clone --branch=5g-lena-v4.0.y https://gitlab.com/cttc-lena/nr.git
  ```

* [ns3-ai](https://github.com/hust-diangroup/ns3-ai/tree/main):

  ```shell
  git clone --branch=v1.2.0 https://github.com/hust-diangroup/ns3-ai.git
  ```

* [ML Propagation Loss Model (MLPL)](https://gitlab.com/inesctec-ns3/ml-propagation-loss-model):

  ```shell
  git clone https://gitlab.com/inesctec-ns3/ml-propagation-loss-model.git
  ```

* [Trace-Based Propagation Loss Model](https://gitlab.com/inesctec-ns3/trace-based-propagation-loss-model):

  ```shell
  git clone https://gitlab.com/inesctec-ns3/trace-based-propagation-loss-model.git
  ```

## Python Setup

In order to train the MLPL model and run scripts to manage the dataset and results, Python needs to be set up.

First, create a new virtual environment and activate it with the following command:

```shell
python3 -m venv venv/
source venv/bin/activate
```

Then, install the dependencies listed in the MLPL module:

```shell
pip install -r ./contrib/ml-propagation-loss-model/requirements.txt
```

Also, install ns3-ai Python package:

```shell
pip3 install ./contrib/ns3-ai/py_interface
```

## Build and Configure ns-3

Now that ns-3 is installed, configure it and build it with the following command:

```shell
./ns3 configure --enable-examples --enable-tests
./ns3 build
```
