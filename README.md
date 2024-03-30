# Functionally-Complete Boolean Logic in Real DRAM Chips: Experimental Characterization and Analysis
This repository source code of our [HPCA'24 paper](https://arxiv.org/pdf/2402.18736.pdf).

> Ismail Emir Yüksel, Yahya Can Tugrul Ataberk Olgun, F. Nisa Bostancı, A. Giray Yaglıkçı, Geraldo F. Oliveira, Haocong Luo, Juan Gómez-Luna, Mohammad Sadrosadati, Onur Mutlu, "Functionally-Complete Boolean Logic in Real DRAM Chips: Experimental Characterization and Analysis", HPCA'24.



Please use the following citation to cite our study if the repository is useful for you.
```
@inproceedings{yuksel2024functionally,
    title={{Functionally-Complete Boolean Logic in Real DRAM Chips: Experimental Characterization and Analysis}},
    author={Yuksel, Ismail Emir and Tugrul, Yahya Can and Olgun, Ataberk and Bostanci, F. Nisa and Yaglikci, A. Giray and de Oliveira, Geraldo F. and Luo, Haocong and Luna, Juan Gomez and Sadrosadati, Mohammad and Mutlu, Onur},
    year={2024},
    booktitle={{HPCA}}
}
```
## Prerequisite
Our real DRAM chip characterization is based on the open-source FPGA-based DRAM characterization infrastructure [DRAM Bender](https://github.com/CMU-SAFARI/DRAM-Bender). Please check out and follow the installation instructions of [DRAM Bender](https://github.com/CMU-SAFARI/DRAM-Bender).

The software dependencies for the characterization are:
- GNU Make, CMake 3.10+
- `c++-17` build toolchain (tested with `gcc-9`)
- Python 3.9+
- `pip` packages `pandas`, `scipy`, `matplotlib`, and `seaborn`

## Hardware Setup
Our real DRAM chip characterization infrastructure consists of the following components:
- A host x86 machine with a PCIe 3.0 x16 slot
- An FPGA board with a DIMM/SODIMM slot supported by [DRAM Bender](https://github.com/CMU-SAFARI/DRAM-Bender) (e.g., Xilinx Alveo U200)
- Heater pads attached to the DRAM module under test
- A temperature controller (e.g., MaxWell FT200) programmable by the host machine connected to the heater pads

## Directory Structure
```
DRAM-Bender                 # A fork of DRAM Bender that contains the characterization program
  └ sources           
    └ apps           
      └ fcdram_artifact     # Source code of the characterization program    
analysis                    # Scripts to post-process, analyze, and plot the characterization data 
  └ plots                   # Directory that will contain the final plots
experimental_data           # Directory that will contain all experimental data
```

## Running the Characterization Experiments
We provide a python script `DRAM-Bender/sources/apps/fcdram_artifact/run_all_scripts.py` to drive all characterization experiments. This script contains all parameters we used in our experiments (including all temperature values). This script does NOT include the instructions to actually set the temperature of the DRAM chip, because they differ from different temperature controllers. The user is responsible for handling the communication between the host machine and the temperature controller.
### Step 0: Get a persistent shell session
The real DRAM chip characterization takes a long period of time. To run all our characterization experiments, a completion time of 3-4 weeks is expected. Therefore, it is recommended to run the characterization experiment script in a persistent shell session (e.g., using a terminal multiplexer like screen, `tmux`).

```
  $ cd DRAM-Bender/sources/apps/fcdram_artifact
```
### Step 1: Start the experiment
```
  $ python3 run_all_scripts.py ${MODULE}
```
Executing the python script will start all characterization experiments. The results are saved to `experimental_data/${MODULE}`.

## Analyzing and Plotting the Characterization Results
```
  $ cd analysis/
```
Open the Jupyter notebook `paper_plots.ipynb` and execute all cells. Executing the notebook will generate all plots in the `analysis/plots` directory.

## Contact:
Ismail Emir Yuksel (ismail.yuksel [at] safari [dot] ethz [dot] ch)  