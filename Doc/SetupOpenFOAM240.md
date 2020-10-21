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

1. Untar OpenFOAM-2.4.0-sakamoto.tar.gz and copy the untared source codes to $WM_PROJECT_INST_DIR
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
```
$ export HDF5_ARCH_PATH=/usr/lib/x86_64-linux-gnu/hdf5/serial/lib
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HDF5_ARCH_PATH/lib
```
