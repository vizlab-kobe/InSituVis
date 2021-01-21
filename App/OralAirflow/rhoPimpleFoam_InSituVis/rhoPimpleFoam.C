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

// In-situ visualization
#include "InSituVis.h"
#include <InSituVis/Lib.foam/FoamToKVS.h>
#define IN_SITU_VIS

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

#if defined( IN_SITU_VIS )
    // In-situ visualization setup
    Foam::messageStream::level = 0; // Disable Foam::Info
    const kvs::Indent indent(4); // indent for log stream
    kvs::Timer timer; // timer for measuring sim and vis processing times
    local::InSituVis vis( MPI_COMM_WORLD );
    vis.importBoundaryMesh( "./constant/triSurface/realistic-cfd3.stl" );
    //vis.setPipeline( local::InSituVis::OrthoSlice() );
    //vis.setPipeline( local::InSituVis::Isosurface() );
    if ( !vis.initialize() )
    {
        vis.log() << "ERROR: " << "Cannot initialize visualization process." << std::endl;
        vis.world().abort();
    }
#endif // IN_SITU_VIS

#if defined( IN_SITU_VIS )
    // Time-loop information
    const auto start_time = runTime.startTime().value();
    const auto start_time_index = runTime.startTimeIndex();
    const auto end_time = runTime.endTime().value();
    const auto end_time_index = static_cast<int>( end_time / runTime.deltaT().value() );
    vis.log() << std::endl;
    vis.log() << "STARTING TIME LOOP" << std::endl;
    vis.log() << indent << "Start time and index: " << start_time << ", " << start_time_index << std::endl;
    vis.log() << indent << "End time and index: " << end_time << ", " << end_time_index << std::endl;
    vis.log() << std::endl;
#endif // IN_SITU_VIS

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    while (runTime.run())
    {
        #include "readTimeControls.H"
        #include "compressibleCourantNo.H"
        #include "setDeltaT.H"

        runTime++;

#if defined( IN_SITU_VIS )
        // Loop information
        const auto current_time = runTime.timeName();
        const auto current_time_index = runTime.timeIndex();
        vis.log() << "LOOP[" << current_time_index << "/" << end_time_index << "]: " << std::endl;
        vis.log() << indent << "T: " << current_time << std::endl;
        vis.log() << indent << "End T: " << end_time << std::endl;
        vis.log() << indent << "Delta T: " << runTime.deltaT().value() << std::endl;
        timer.start(); // begin sim.
#endif // IN_SITU_VIS

        Info<< "Time = " << runTime.timeName() << nl << endl;

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

#if defined( IN_SITU_VIS )
        timer.stop(); // end sim.
        const auto ts = timer.sec();
        const auto Ts = kvs::String::From( ts, 4 );
        vis.log() << indent << "Processing Times:" << std::endl;
        vis.log() << indent.nextIndent() << "Simulation: " << Ts << " s" << std::endl;

        // Execute in-situ visualization process
        timer.start(); // begin vis.
        {
            // p: pressure
            auto& field = p;
            const auto min_value = 9.94 * 10000.0;
            const auto max_value = 1.02 * 100000.0;
            //vis.setMinMaxValues( 9.94 * 10000.0, 1.02 * 100000.0 );

            // U: velocity
            //auto& field = U;
            //vis.setMinMaxValues( 0.0224, 70.9 );

            // T: temperature
            //auto& field = thermo.T();
            //vis.setMinMaxValues( 293, 295 );

            // Convert OpenFOAM data to KVS data
            InSituVis::foam::FoamToKVS converter( field );
            using CellType = InSituVis::foam::FoamToKVS::CellType;
            auto vol_tet = converter.exec( vis.world(), field, CellType::Tetrahedra );
            auto vol_hex = converter.exec( vis.world(), field, CellType::Hexahedra );
            auto vol_pri = converter.exec( vis.world(), field, CellType::Prism );
            auto vol_pyr = converter.exec( vis.world(), field, CellType::Pyramid );

            vol_tet.setName("Tet");
            vol_hex.setName("Hex");
            vol_pri.setName("Pri");
            vol_pyr.setName("Pyr");

            //vol_tet->print( vis.log() << std::endl );
            //vol_hex->print( vis.log() << std::endl );
            //vol_pri->print( vis.log() << std::endl );
            //vol_pyr->print( vis.log() << std::endl );

            vol_tet.setMinMaxValues( min_value, max_value );
            vol_hex.setMinMaxValues( min_value, max_value );
            vol_pri.setMinMaxValues( min_value, max_value );
            vol_pyr.setMinMaxValues( min_value, max_value );

            // Execute visualization pipeline and rendering
            vis.put( vol_tet );
            vis.put( vol_hex );
            vis.put( vol_pri );
            vis.put( vol_pyr );
            vis.exec( runTime.timeIndex() );

//            delete vol_tet;
//            delete vol_hex;
//            delete vol_pri;
//            delete vol_pyr;
        }
        timer.stop(); // end vis.

        const auto tv = timer.sec();
        const auto Tv = kvs::String::From( tv, 4 );
        vis.log() << indent.nextIndent() << "Visualization: " << Tv << " s" << std::endl;

        const auto elapsed_time = runTime.elapsedCpuTime();
        vis.log() << indent << "Elapsed Time: " << elapsed_time << " s" << std::endl;
        vis.log() << std::endl;
#endif // IN_SITU_VIS

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

#if defined( IN_SITU_VIS )
    if ( !vis.finalize() )
    {
        vis.log() << "ERROR: " << "Cannot finalize visualization process." << std::endl;
        vis.world().abort();
    }
#endif // IN_SITU_VIS

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
