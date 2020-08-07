/*---------------------------------------------------------------------------* \
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

// rhoPimpleFoam_InSituVis: Headers
// {
#include "Visualization.h"
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

    // rhoPimpleFoam_InSituVis: Parameter settings
    // {
    Foam::messageStream::level = 0; // Disable Foam::Info
    const kvs::Indent indent(4);
    local::Visualization vis( MPI_COMM_WORLD );
    vis.setSize( 512, 512 );
    vis.setOutputImageEnabled( true );
    vis.setOutputSubImageEnabled( false, false, false );
    vis.setOutputDirectoryName( "Output", "Proc_" );
    if ( !vis.initialize() )
    {
        vis.log() << "ERROR: " << "Cannot initialize visualization process." << std::endl;
        vis.world().abort();
    }
    // }

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    // rhoPimpleFoam_InSituVis: Output messages
    // {
    // Info<< "\nStarting time loop\n" << endl;
    const auto start_time = runTime.startTime().value();
    const auto start_time_index = runTime.startTimeIndex();
    const auto end_time = runTime.endTime().value();
    const auto end_time_index = static_cast<int>( end_time / runTime.deltaT().value() );
    vis.log() << std::endl;
    vis.log() << "STARTING TIME LOOP" << std::endl;
    vis.log() << indent << "Start time and index: " << start_time << ", " << start_time_index << std::endl;
    vis.log() << indent << "End time and index: " << end_time << ", " << end_time_index << std::endl;
    vis.log() << std::endl;
    // }

    while (runTime.run())
    {
        #include "readTimeControls.H"
        #include "compressibleCourantNo.H"
        #include "setDeltaT.H"

        runTime++;

        // rhoPimpleFoam_InSituVis: Output loop information
        // {
        // Info<< "Time = " << runTime.timeName() << nl << endl;
        const auto current_time = runTime.timeName();
        const auto current_time_index = runTime.timeIndex();
        vis.log() << "LOOP[" << current_time_index << "/" << end_time_index << "]: " << std::endl;
        vis.log() << indent << "T: " << current_time << std::endl;
        vis.log() << indent << "End T: " << end_time << std::endl;
        vis.log() << indent << "Delta T: " << runTime.deltaT().value() << std::endl;
        // }

        // rhoPimpleFoam_InSituVis: Start simulation
        // {
        kvs::Timer timer;
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

//        if ( output_sim ) runTime.write();
        runTime.write();

        // rhoPimpleFoam_InSituVis: End simulation
        // {
        timer.stop();
        const auto ts = timer.sec();
        const auto Ts = kvs::String::From( ts, 4 );
        vis.log() << indent << "Processing Times:" << std::endl;
        vis.log() << indent.nextIndent() << "Simulation: " << Ts << " s" << std::endl;
        // }

        timer.start();
        vis.exec( runTime, mesh, U );
        timer.stop();
        const auto tv = timer.sec();
        const auto Tv = kvs::String::From( tv, 4 );
        vis.log() << indent.nextIndent() << "Visualization: " << Tv << " s" << std::endl;

        // rhoPimpleFoam_InSituVis: Output messages
        // {
        // Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
        //     << "  ClockTime = " << runTime.elapsedClockTime() << " s"
        //     << nl << endl;
        const auto elapsed_time = runTime.elapsedCpuTime();
        vis.log() << std::endl;
        vis.log() << "Elapsed Time: " << elapsed_time << " s" << std::endl;
        vis.log() << std::endl;
        // }
    }

    // rhoPimpleFoam_InSituVis: Destory image compositor
    // {
    if ( !vis.finalize() )
    {
        vis.log() << "ERROR: " << "Cannot finalize visualization process." << std::endl;
        vis.world().abort();
    }
    // }

    //Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
