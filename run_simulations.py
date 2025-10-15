"""
Run ns-3 simulations for all combinations of parameters.
"""

import argparse
import concurrent.futures
import itertools
import os
import subprocess

#######################################
# SIMULATION PARAMETERS
#######################################
LOSS_MODELS = [
    "3gpp",
    "trace-based",
    "mlpl-xgb",
    # "friis",
    # "fixed-rss",
]

PROTOCOLS = [
    "udp",
    "tcp",
]

MODES = [
    "uplink",
    "downlink",
    # "bidir",
]

DISTANCES = [
    9,
    56,
]

SIMULATION_TIME = 540


#######################################
# FUNCTIONS
#######################################
def run_simulations(ns3_dir: str, verbose: bool, jobs: int = 1) -> None:
    """
    Run all simulations in parallel.

    Args
    ----
        ns3_dir: ns-3 base directory.
        verbose: Show output from ns-3 simulations.
        jobs: Number of parallel jobs. By default, run 1 job in parallel.
    """

    print("-- Building ns-3")
    build_ns3(verbose, ns3_dir)

    print("-- Starting ns-3 simulations")
    with concurrent.futures.ProcessPoolExecutor(jobs) as executor:
        for loss_model, protocol, mode, distance in itertools.product(
            LOSS_MODELS,
            PROTOCOLS,
            MODES,
            DISTANCES,
        ):
            executor.submit(
                run_ns3_simulation,
                loss_model,
                protocol,
                mode,
                distance,
                SIMULATION_TIME,
                verbose,
                ns3_dir,
            )

    print("-- Finished all simulations")


def run_ns3_simulation(
    loss_model: str,
    protocol: str,
    mode: str,
    distance: float,
    simulation_time: float,
    verbose: bool,
    ns3_dir: str,
) -> None:
    """
    Run a single ns-3 simulation.

    Args
    ----
        loss_model: Loss model.
        protocol: Protocol.
        mode: Mode.
        distance: Distance.
        simulation_time_s: Simulation time.
        verbose: Show output from ns-3 simulation.
        ns3_dir: ns-3 base directory.
    """

    cmd = [
        "./ns3",
        "run",
        "--no-build",
        '"',
        "replica-example",
        f"--lossModel={loss_model}",
        f"--protocol={protocol}",
        f"--mode={mode}",
        f"--simulationTime={simulation_time}",
        f"--distance={distance}",
        '"',
    ]

    print(f"Starting simulation: {loss_model=}, {protocol=}, {mode=}, {distance=}")

    try:
        proc = subprocess.run(
            " ".join(cmd),
            check=True,
            shell=True,
            text=True,
            capture_output=verbose,
            cwd=ns3_dir,
        )

        if verbose:
            print(proc.stdout)
            print(proc.stderr)

        print(f"Finished simulation: {loss_model=}, {protocol=}, {mode=}, {distance=}")

    except subprocess.CalledProcessError as e:
        print(f"Error running simulation {loss_model=}, {protocol=}, {mode=}, {distance=}")
        print(e)


def build_ns3(verbose: bool, ns3_dir: str) -> None:
    """
    Build ns-3.

    Args
    ----
        verbose: Show output from ns-3 build.
        ns3_dir: ns-3 base directory.
    """

    try:
        proc = subprocess.run(
            ["./ns3", "build"],
            check=True,
            text=True,
            capture_output=verbose,
            cwd=ns3_dir,
        )

        if verbose:
            print(proc.stdout)
            print(proc.stderr)

    except subprocess.CalledProcessError as e:
        print(f"Error building ns-3: {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Run ns-3 simulations for all combinations of parameters."
    )

    parser.add_argument(
        "--ns3-dir",
        type=str,
        default=".",
        help="ns-3 base directory (by default, it is the current directory)",
    )

    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Show output from ns-3 simulations",
    )

    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=max(1, os.cpu_count() - 1),
        help="Number of parallel jobs",
    )

    args = parser.parse_args()
    run_simulations(
        ns3_dir=args.ns3_dir,
        verbose=args.verbose,
        jobs=args.jobs,
    )
