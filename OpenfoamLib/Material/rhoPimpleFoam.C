/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    rhoPimpleFoam

Description
    Transient solver for laminar or turbulent flow of compressible fluids
    for HVAC and similar applications.

    Uses the flexible PIMPLE (PISO-SIMPLE) solution for time-resolved and
    pseudo-transient simulations.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "psiThermo.H"
#include "turbulenceModel.H"
#include "bound.H"
#include "pimpleControl.H"
#include "fvIOoptionList.H"
#include <vector>
#include <fenv.h>
#include <kvs/UnstructuredVolumeObject>
#include <KVS.py/Lib/Interpreter.h>
#include <KVS.py/Lib/String.h>
#include <KVS.py/Lib/Module.h>
#include <KVS.py/Lib/Dict.h>
#include <KVS.py/Lib/Callable.h>
#include <KVS.py/Lib/Array.h>
#include <KVS.py/Lib/Float.h>
#include <mpi.h>
#include <kvs/Timer>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
int main(int argc, char *argv[])
{
  #include "setRootCase.H"
  #include "createTime.H"
  #include "createMesh.H"
  pimpleControl pimple(mesh);

    #include "createFields.H"
    #include "createFvOptions.H"
    #include "initContinuityErrs.H"

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
  //vis
  std::string filename = "entropy.csv";
  std::ofstream writing_file;
  writing_file.open(filename, std::ios::app);
  int my_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
  float threshold = 0.0;
  float pre_entropy  = 0.0;
  std::vector<std::vector<float>> data_set;
  kvs::ValueArray<float> old_hist;
  kvs::ValueArray<float> new_hist;
  int count = 0;
  int vis_skip = 2;
  const int volume_size = 5;
  kvs::python::Interpreter interpreter;
  const char* script_file_name = "distribution"; 
  kvs::python::Module module( script_file_name );
  kvs::python::Dict dict = module.dict();
  
  const char* func_name = "main"; 
  kvs::python::Callable func( dict.find( func_name ) );

  kvs::Timer sim_timer;
  kvs::Timer vis_timer;
  kvs::Timer distribution_timer;
  float sim_time = 0.0;
  float vis_time = 0.0;
  float distribution_time = 0.0;
  
  //
  
    Info<< "\nStarting time loop\n" << endl;
    while (runTime.run())
    {
        #include "readTimeControls.H"
        #include "compressibleCourantNo.H"
        #include "setDeltaT.H"

        runTime++;

        Info<< "Time = " << runTime.timeName() << nl << endl;

        if (pimple.nCorrPIMPLE() <= 1)
        {
            #include "rhoEqn.H"
        }

        // --- Pressure-velocity PIMPLE corrector loop
	sim_timer.start();
        while (pimple.loop())
        {
            #include "UEqn.H"
            #include "EEqn.H"

            // --- Pressure corrector loop
            while (pimple.correct())
            {
                #include "pEqn.H"
            }

            if (pimple.turbCorr())
            {
                turbulence->correct();
            }
        }
	sim_timer.stop();
	sim_time = sim_timer.sec();
        runTime.write();
	{
	  fenv_t curr_excepts;
	  feholdexcept( &curr_excepts );
	  MPI_Allreduce( &sim_time, &sim_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
	  Info << "Simulation Solver time : " << sim_time << endl; 
	  #include "vis.H"
	  Info << "Conversion sim time : " << vis_time << endl;
	  Info << "Distribution time : " << distribution_time << endl;
	}

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
