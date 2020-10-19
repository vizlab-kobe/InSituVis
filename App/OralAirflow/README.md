# In-situ Visualization for Oral Airflow Simulation based on OpenFOAM

## Instaltion

### KVS
KVS supports OSMesa and MPI needs to be installed.

1. Install OSMesa and MPI

OSMesa and MPI need to be install before compile the KVS. Please refer to the followin URLs to install them.<br>
- [OSMesa](https://github.com/naohisas/KVS/blob/develop/Source/SupportOSMesa/README.md)
- [MPI](https://github.com/naohisas/KVS/blob/develop/Source/SupportMPI/README.md)

2. Get KVS source codes from the GitHub repository as follows:
```
$ git clone https://github.com/naohisas/KVS.git
```

3. Modify kvs.conf as follows:
```
$ cd KVS
$ <modify kvs.conf>
```

- Change the following items from 0(disable) to 1(enable).<br>
```
KVS_ENABLE_OPENGL     = 1
KVS_SUPPORT_MPI       = 1
KVS_SUPPORT_OSMESA    = 1
```
- Change the following items to 1 if needed. <br>
```
KVS_ENABLE_OPENMP     = 1
KVS_SUPPORT_PYTHON    = 1
```

4. Compile and install the KVS
```
$ make
$ make install
```

### OpenFOAM
OpenFOAM 2.3.1 is required.

1. Get OpenFOAM 2.3.1 source codes from the following URL.

https://www.dropbox.com/s/aa8azaz2jt0inta/openfoam231.tar.gz?dl=0

2. Compile the OpenFOAM

```
$ cd <download directory>
$ tar -zxvf openfoam231.tar.gz 
＊ Change 'g++' in wmake/rules/linux64Gcc/c++ to 'mpicxx'
$ cd OpenFOAM/OpenFOAM-2.3.1
$ source etc/bashrc
$ cd src/PStream/mpi 
$ wclean &wmake 
$ cd ../../../
$ ./Allwmake
```

## Execution

### InSituVis

### rhoPimpleFoam


### rhoPimpleFoam_InSituVis

