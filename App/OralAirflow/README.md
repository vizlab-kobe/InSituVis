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

## Execution

### InSituVis

### rhoPimpleFoam


### rhoPimpleFoam_InSituVis

