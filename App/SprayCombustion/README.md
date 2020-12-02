# In-situ Visualization for Spray Flame Combustion based on OpenFOAM

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
    - Change the following items from 1(enable) to 0(disable).<br>
    ```
    KVS_SUPPORT_GLU       = 0
    KVS_SUPPORT_GLUT      = 0
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
OpenFOAM 2.4.0 is required. Here is an installation guid of the patched OpenFOAM 2.4.0 for Ubuntu 18.04.

1. Install original OpenFOAM 2.4.0 and ThirdParty 2.4.0 for Ubuntu 18.04 based on the following installation.

    - [Installation/Linux/OpenFOAM-2.4.0/Ubuntu](https://openfoamwiki.net/index.php/Installation/Linux/OpenFOAM-2.4.0/Ubuntu#Ubuntu_18.04)

2. Get OpenFOAM 2.4.0 source codes, which are patched by Dr Tsukasa Hori, Osaka University, from the following URL.

    - [OpenFOAM-2.4.0-sakamoto.tar.gz](https://app.box.com/s/6mjk18x70p4jm0dfex0q6ba4aarxhsqq)

2. Compile the patched OpenFOAM after backing up the original one.
    ```
    // Back up the original OpenFOAM
    $ tar zxvf OpenFOAM-2.4.0-sakamoto.tar.gz -C $WM_PROJECT_INST_DIR
    $ cd $WM_PROJECT_INST_DIR
    $ mv OpenFOAM-2.4.0 OpenFOAM-2.4.0.org
    $ mv OpenFOAM-2.4.0-sakamoto OpenFOAM-2.4.0

    // Copy some setting files
    $ cp -r OpenFOAM-2.4.0.org/etc OpenFOAM-2.4.0
    $ cp -r OpenFOAM-2.4.0.org/wmake OpenFOAM-2.4.0
    $ cp -r OpenFOAM-2.4.0.org/tutorials OpenFOAM-2.4.0

    // Install some packages by using apt-get with sudo
    $ sudo apt-get install scotch
    $ sudo apt-get install fortran-5
    $ sudo apt-get install libhdf5-serial-dev
    $ sudo apt-get install libptscotch-dev

    // Set some environment parameters on the terminal
    $ export HDF5_ARCH_PATH=/usr/lib/x86_64-linux-gnu/hdf5/serial/lib
    $ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HDF5_ARCH_PATH/lib
    // or add them to ~/.bashrc and source it on the terminal
    $ source ~/.bashrc

    // Move to the directory
    $ cd OpenFOAM-2.4.0

    // Modify '$(uname -s) == "Darwin"' related errors
    $ find . -type f -exec sed -i -e 's/== \"Darwin\"/= \"Darwin\"/g' {} \;

    // Modify H5Cpp.H related errors
    $ for i in `find applications/solvers applications/utilities -name options`; do echo -n -e "EXE_INC += -I\$(HDF5_ARCH_PATH)/include\nLIB_LIBS += -L\$(HDF5_ARCH_PATH)/lib -lhdf5 -lhdf5_hl -lhdf5_cpp\n" >> $i; done

    // Modify ptscotch.h related errors
    $ echo -n -e "EXE_INC += -I\$(SCOTCH_ARCH_PATH)/include/\$(FOAM_MPI)\nLIB_LIBS += -L\$(SCOTCH_ARCH_PATH)/lib/\$(FOAM_MPI)\n" >> src/parallel/decompose/ptscotchDecomp/Make/options

    // Modify Eigen/Core related errors
    $ echo -n -e "EXE_INC += -I\$(WM_THIRD_PARTY_DIR)/ParaView-4.1.0/Plugins/SciberQuestToolKit/eigen-3.0.3/eigen-eigen-3.0.3/\n" >> src/thermophysicalModels/chemistryModel/Make/options

    // Modify 'specie' related errors
    // Add the following codes to src/thermophysicalModels/chemistryModel/chemistrySolver/equil/equil.C
    // ...
    // #include "equil.H"
    // #include "chemistryModel.H"
    // #include "specie.H" // added this line
    // ...

    // Compile the source codes
    $ export QT_SELECT=qt4
    $ ./Allwmake > log.make 2>&1

    // Check execution of the compiled OpenFOAM
    $ of240
    $ tut
    $ cd incompressible/icoFoam/cavity
    $ blockMesh
    $ icoFoam
    $ paraFoam -builtin
    ```

## Execution

### InSituVis

1. Get the InSituVis source codes from the GitHub repository as follows:
    ```
    $ git clone https://github.com/vizlab-kobe/InSituVis.git
    ```

2. Move to the application directory.
    ```
    $ cd App/SprayCombustion
    ```

### spraySootTimeFoam_InSituVis
The compilation and execuation of the application are done in a separate terminal.

- Compilation<br>
    ```
    $ cd spraySootTimeFoam_InSituVis
    $ ./make.sh
    * 'clear.sh' is a shell script for removing the compiled files.
    ```

- Execution<br>
    ```
    $ cd sprayH_2mm_O2_15
    $ ./run_insitu.sh
    ```

### spraySootTimeFoam
The spraySootTimeFoam is an original program based OpenFOAM for the spray combustion simulation. This program can be compile and execute same as the spraySootTimeFoam_InSituVis program. This program doesn't require the KVS and OSMesa.
