# Grouping nodes with known value differences: A lossless UCT-based abstraction algorithm


## Purpose

This is the repository accompanying the paper "Grouping nodes with known value differences: A lossless UCT-based abstraction algorithm" which contains the code to reproduce the experiments and the results of the paper.

## Cite this work

If you use this work, please cite it as:

```bibtex
@misc{kvdauct,
      title={Grouping Nodes With Known Value Differences: A Lossless UCT-based Abstraction Algorithm}, 
      author={Robin Schmöcker and Alexander Dockhorn and Bodo Rosenhahn},
      year={2025},
      eprint={2510.25388},
      archivePrefix={arXiv},
      primaryClass={cs.AI},
      url={https://arxiv.org/abs/2510.25388}, 
}
```

## Abstract

A core challenge of Monte Carlo Tree Search (MCTS) is its sample efficiency,
which can be addressed by building and using state and/or state-action pair abstractions
in parallel to the tree search, such that information can be shared among
nodes of the same layer. On the Go Abstractions in Upper Confidence bounds
applied to Trees (OGA-UCT) is the state-of-the-art MCTS abstraction algorithm
for deterministic environments that builds its abstraction using the Abstractions
of State-Action Pairs (ASAP) framework, which aims to detect states and stateaction
pairs with the same value under optimal play by analysing the search graph.
ASAP, however, requires two state-action pairs to have the same immediate reward,
which is a rigid condition that limits the number of abstractions that can be
found and thereby the sample efficiency. In this paper, we break with the paradigm
of grouping value-equivalent states or state-action pairs and instead group states
and state-action pairs with possibly different values as long as the difference between
their values can be inferred. We call this abstraction framework Known
Value Difference Abstractions (KVDA), which infers the value differences by
analysis of the immediate rewards and modifies OGA-UCT to use this framework
instead. The modification is called KVDA-UCT, which detects significantly more
abstractions than OGA-UCT, introduces no additional parameter, and outperforms
OGA-UCT on a variety of deterministic environments and parameter settings.
## Installation

To build the project from source, you will need a C++ compiler supporting the C++20 standard or higher (a lower standard probably works too but we have not tested that). The project
is self-contained and does not require any additional installation.

To compile with [CMake](https://cmake.org/) you need to have CMake installed on your system. A `CMakeLists.txt` file is already provided for configuring the build.

**Steps:**

1. **Clone the repository:**
    ```bash
    git clone https://github.com/codebro634/KVDA_UCT.git
    cd KVDA_UCT
    ```

2. **Create a build directory (optional but recommended):**
    ```bash
    mkdir build
    cd build
    ```

3. **Generate build files using CMake:**
    ```bash
    cmake -DCMAKE_CXX_COMPILER=/path/to/your/c++-compiler -DCMAKE_C_COMPILER=/path/to/your/c-compiler ..
    ```

4. **Compile the project:**
    ```bash
    cmake --build .
    ```
   *This will invoke the underlying build system (e.g., `make` or `ninja`) to compile the source code.*

If no errors occur, two compiled binaries `KVDADebug` and `KVDARelease` should now be available in the `build` directory. The former has been compiled with debug
compiler flags and the latter with aggressive optimization.

## Usage

The program is called with the following arguments:

`--seed`: The seed for the random number of generator. Running the program with the same seed will produce the same results.

`--n`: The number of episodes to run.

`--model`: The abbreviation for the model to use. Example values are
`sa` for SysAdmin, `gol` for Game of Life, `aa` for AcademicAdvising or  `tam` for Tamarisk. The ful list of abbreviations is found in main.cpp in the getModel method.

`--margs`: The arguments for the model. Mostly this is a game map to be specified which can be found in the
`resources` folder.

`--agent`:  Which agent to use. The only options are `random` and `oga`. As part of its parametrization OGA can become KVDA-UCT.

`--aargs`: The arguments for the agent. A list of required and optional arguments can be found in main.cpp in getAgent.

The following shows an example call of running KVDA-UCT with 500 Mcts-iterations.

```bash
--seed 42 -n 10  --model gol --margs map=../resources/GameOfLifeMaps/1_Anand.txt  --agent oga --aargs iterations=500 --aargs smart_reward_handling=1
```

