# InSituVis
InSituVis provides an in-situ visualization framework that enables easy and effective connection of numerical simulation solvers and visualization systems in a high-performance computing environment.

## Installation

### Pre-requisities

The follwoing packages needs to be installed before compiling SmokeRingVis.
- [KVS](https://github.com/naohisas/KVS)
- OSMesa
- MPI

#### KVS
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
    - Change the following items from 1(enable) to 0(disable).<br>
    ```
    KVS_SUPPORT_GLU       = 0
    KVS_SUPPORT_GLUT      = 0
    ```
    - Change the following items to 1 if needed. <br>
    ```
    KVS_ENABLE_OPENMP     = 1
    ```

4. Compile and install the KVS
    ```
    $ make
    $ make install
    ```
