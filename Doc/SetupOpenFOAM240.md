# Setup OpenFOAM 2.4.0 for Combustion Simulation

This is an instalation guide for OpenFOAM 2.4.0 on Ubuntu 18.04.

## OpenFOAM 2.4.0
First, the original version of OpenFOAM 2.4.0 needs to be installed on Ubuntu 18.04 by reffering to the following site.
- [Installation/Linux/OpenFOAM-2.4.0/Ubuntu](https://openfoamwiki.net/index.php/Installation/Linux/OpenFOAM-2.4.0/Ubuntu#Ubuntu_18.04)

If compiled successfully, OpenFOAM 2.4.0 and ThirdParty 2.4.0 will be installed as follows:
```
$ cd ~/OpenFOAM
$ ls
OpenFOAM-2.4.0  OpenFOAM-2.4.0.tgz  ThirdParty-2.4.0  ThirdParty-2.4.0.tgz
$ env | grep WM
WM_PROJECT_INST_DIR=/home/xxx/OpenFOAM
WM_THIRD_PARTY_DIR=/home/xxx/OpenFOAM/ThirdParty-2.4.0
WM_LDFLAGS=-m64
WM_CXXFLAGS=-m64 -fPIC
WM_PRECISION_OPTION=DP
WM_CC=gcc-5
WM_PROJECT_USER_DIR=/home/xxx/OpenFOAM/naohisas-2.4.0
WM_OPTIONS=linux64GccDPOpt
WM_LINK_LANGUAGE=c++
WM_OSTYPE=POSIX
WM_PROJECT=OpenFOAM
WM_CFLAGS=-m64 -fPIC
WM_ARCH=linux64
WM_COMPILER_LIB_ARCH=64
WM_COMPILER=Gcc
WM_DIR=/home/xxx/OpenFOAM/OpenFOAM-2.4.0/wmake
WM_ARCH_OPTION=64
WM_PROJECT_VERSION=2.4.0
WM_MPLIB=SYSTEMOPENMPI
WM_COMPILE_OPTION=Opt
WM_CXX=g++-5
WM_PROJECT_DIR=/home/xxx/OpenFOAM/OpenFOAM-2.4.0
```

## Customized OpenFOAM 2.4.0 for the combustion simulation solver
Next, the compiled OpenFOAM 2.4.0 needs to be replaced with the customized one (OpenFOAM-2.4.0-sakamoto.tar.gz) for the combustion simulation solver developed by Dr. Tsukasa Hori, Osaka University.

1. Untar and copy OpenFOAM-2.4.0-sakamoto.tar.gz
```
$ tar zxvf OpenFOAM-2.4.0-sakamoto.tar.gz -C $WM_PROJECT_INST_DIR
$ cd $WM_PROJECT_INST_DIR
$ mv OpenFOAM-2.4.0 OpenFOAM-2.4.0.org
$ mv OpenFOAM-2.4.0-sakamoto OpenFOAM-2.4.0
```

2. Copy some directories from the original OpenFOAM
```
$ cp -r OpenFOAM-2.4.0.org/etc OpenFOAM-2.4.0
$ cp -r OpenFOAM-2.4.0.org/wmake OpenFOAM-2.4.0
$ cp -r OpenFOAM-2.4.0.org/tutorials OpenFOAM-2.4.0
```

3. Install some packages
```
$ sudo apt-get install scotch
$ sudo apt-get install fortran-5
$ sudo apt-get install libhdf5-serial-dev
$ sudo apt-get install libptscotch-dev
```

4. Set environment variables for HDF5
```bash
// Add the following settings to ~/.bashrc
export HDF5_ARCH_PATH=/usr/lib/x86_64-linux-gnu/hdf5/serial/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HDF5_ARCH_PATH/lib
```
```
$ source ~/.bashrc
```

5. Fix some problems

- Script error related to `$(uname -s) == “Darwin”`
```
$ find . -type f -exec sed -i -e 's/== \"Darwin\"/= \"Darwin\"/g' {} \;
```

- Path error related to `H5Cpp.H: No such file`
```
$ for i in `find applications/solvers applications/utilities -name options`; do echo -n -e "EXE_INC += -I\$(HDF5_ARCH_PATH)/include\nLIB_LIBS += -L\$(HDF5_ARCH_PATH)/lib -hdf5 -lhdf5_hl -lhdf5_cpp\n" >> $i; done
```

- Path error related to `ptscotch.h: No such file`
```
$ echo -n -e "EXE_INC += -I\$(SCOTCH_ARCH_PATH)/include/\$(FOAM_MPI)\nLIB_LIBS += -L\$(SCOTCH_ARCH_PATH)/lib/\$(FOAM_MPI)\n" >> src/parallel/decompose/ptscotchDecomp/Make/options
```

- Path error related to `Eigen/Core: No such file`
```
$ echo -n -e "EXE_INC += -I\$(WM_THIRD_PARTY_DIR)/ParaView-4.1.0/Plugins/SciberQuestToolKit/eigen-3.0.3/eigen-eigen-3.0.3/\n" >> src/thermophysicalModels/chemistryModel/Make/options
```

- Compile error related to `specie class`
Include `specie.H` in `src/thermophysicalModels/chemistryModel/chemistrySolver/equil/equil.C`
```cpp
...
#include "equil.H"
#include "chemistryModel.H"
#include "specie.H" // added this line
...
```
