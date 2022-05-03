# kitchen_sim

A C++ CLI that simulates the operation of a single kitchen given a stream of JSON orders.

# Requirements

- Bazel (see [here](https://docs.bazel.build/versions/master/install.html))
- C++ compiler (must support C++17)

Tested on Windows and Ubuntu 20.04 on WSL2.

# Building

Main CLI:

> bazel build :kitchen_sim

Model libraries:

> bazel build model:all

# Usage

> kitchen_sim --json_path=\<path>

Run with smaller shelf capacities:
> kitchen_sim --json_path=<path> --kitchen_size='SMALL'
 
Run with different ingestion rates:
> kitchen_sim --json_path=<path> --orders_per_second=20

# Testing

> bazel test model:all

Additional variants of the provided `orders.json` file are included under `data/`.