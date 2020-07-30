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

// rhoPimpleFoam_PostHocVis: Headers
// {
#include <kvs/mpi/Communicator>
#include <kvs/mpi/Logger>
#include <kvs/Timer>
#include <kvs/String>
#include "local/CreateOutputDirectory.h"
#include "local/CreateUnstructuredVolumeObject.h"
// }

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

    // rhoPimpleFoam_PostHocVis: Parameter settings
    // {
    Foam::messageStream::level = 0; // Disable Foam::Info
    kvs::mpi::Communicator world( MPI_COMM_WORLD );
    kvs::mpi::Logger logger( world );
    kvs::Timer timer;
    const kvs::Indent indent(4);
    const int root = 0;
    const int size = world.size();
    const int rank = world.rank();
    // }

    // rhoPimpleFoam_PostHocVis: Create output directories
    // {
    const auto output_dirname = local::CreateOutputDirectory( world, "Output", "Proc" );
    if ( output_dirname.empty() )
    {
        logger( root ) << "ERROR: " << "Cannot create output directory." << std::endl;
        world.abort();
    }
    // }

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    // rhoPimpleFoam_PostHocVis: Output messages
    // {
    // Info<< "\nStarting time loop\n" << endl;
    const auto start_time = runTime.startTime().value();
    const auto start_time_index = runTime.startTimeIndex();
    const auto end_time = runTime.endTime().value();
    const auto end_time_index = static_cast<int>( end_time / runTime.deltaT().value() );
    logger( root ) << std::endl;
    logger( root ) << "STARTING TIME LOOP" << std::endl;
    logger( root ) << indent << "Start time and index: " << start_time << ", " << start_time_index << std::endl;
    logger( root ) << indent << "End time and index: " << end_time << ", " << end_time_index << std::endl;
    logger( root ) << std::endl;
    // }

    while (runTime.run())
    {
        #include "readTimeControls.H"
        #include "compressibleCourantNo.H"
        #include "setDeltaT.H"

        runTime++;

        // rhoPimpleFoam_PostHocVis: Output messages
        // {
        // Info<< "Time = " << runTime.timeName() << nl << endl;
        const auto current_time = runTime.timeName();
        const auto current_time_index = runTime.timeIndex();
        logger( root ) << "LOOP[" << current_time_index << "/" << end_time_index << "]: " << std::endl;
        logger( root ) << indent << "T: " << current_time << std::endl;
        logger( root ) << indent << "End T: " << end_time << std::endl;
        logger( root ) << indent << "Delta T: " << runTime.deltaT().value() << std::endl;
        // }

        // rhoPimpleFoam_PostHocVis: Start timer
        // {
        timer.start();
        // }

        if (pimple.nCorrPIMPLE() <= 1)
        {
            #include "rhoEqn.H"
        }

        // --- Pressure-velocity PIMPLE corrector loop
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

        runTime.write();

        // rhoPimpleFoam_PostHocVis: Stop timer
        // {
        timer.stop();
        // }

        // rhoPimpleFoam_PostHocVis: Output messages
        // {
        const auto ts = timer.sec();
        const auto Ts = kvs::String::ToString( ts, 4 );
        logger( root ) << indent << "Processing Times:" << std::endl;
        logger( root ) << indent.nextIndent() << "Simulation: " << Ts << " s" << std::endl;
        // }

        // rhoPimpleFoam_PostHocVis: Import mesh and field
        // {
        timer.start();
        auto* volume = local::CreateUnstructuredVolumeObject( mesh, p ); // p: pressure value
        timer.stop();
        // }

        // rhoPimpleFoam_PostHocVis: Output messages
        // {
        const auto ti = timer.sec();
        const auto Ti = kvs::String::ToString( ti, 4 );
        logger( root ) << indent.nextIndent() << "Import: " << Ti << " s" << std::endl;
        // }

        // rhoPimpleFoam_PostHocVis: Output KVSML
        timer.start();
        const std::string output_basename("output");
        const std::string output_filename = output_basename + "_" + current_time + ".kvsml";
        volume->write( output_dirname + output_filename );
        timer.stop();
        delete volume;

        // rhoPimpleFoam_PostHocVis: Output messages
        // {
        const auto to = timer.sec();
        const auto To = kvs::String::ToString( to, 4 );
        logger( root ) << indent.nextIndent() << "Write: " << To << " s" << std::endl;
        // }

        // rhoPimpleFoam_PostHocVis: Output messages
        // {
        const auto tt = ts + ti + to;
        const auto Tt = kvs::String::ToString( tt, 4 );
        logger( root ) << indent.nextIndent() << "---" << std::endl;
        logger( root ) << indent.nextIndent() << "Total: " << Tt << " s" << std::endl;
        // }

        // rhoPimpleFoam_PostHocVis: Output messages
        // {
        // Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
        //     << "  ClockTime = " << runTime.elapsedClockTime() << " s"
        //     << nl << endl;
        const auto elapsed_time = runTime.elapsedCpuTime();
        logger( root ) << std::endl;
        logger( root ) << "Elapsed Time: " << elapsed_time << " s" << std::endl;
        logger( root ) << std::endl;
        // }
    }

    //Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
