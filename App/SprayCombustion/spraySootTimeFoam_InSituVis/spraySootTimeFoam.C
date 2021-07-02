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
    sprayFoam

Description
    Transient PIMPLE solver for compressible, laminar or turbulent flow with
    spray parcels.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "turbulenceModel.H"
#include "basicSprayCloud.H"
#include "psiCombustionModel.H"
#include "radiationModel.H"
#include "SLGThermo.H"
#include "pimpleControl.H"
#include "fvIOoptionList.H"

#include "wallFvPatch.H"
//#include "pyMomentClass.H"

// #include "mpi.h"
#include "List.H"
 #include "mapDistribute.H"
 #include "argList.H"
 #include "Time.H"
 #include "clockTime.H"
 #include "IPstream.H"
 #include "OPstream.H"
 #include "vector.H"
 #include "IOstreams.H"
 #include "Random.H"
 #include "Tuple2.H"

#include "fieldValue.H"
#include "omp.h"

#include "sootMoment.H"
// #include "sootMomentTest.H"

// In-situ visualization
#define IN_SITU_VIS
#if defined( IN_SITU_VIS )
#include "InSituVis.h"
#include <InSituVis/Lib.foam/FoamToKVS.h>
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //



int main(int argc, char *argv[])
{
    #include "setRootCase.H"

    #include "createTime.H"
    #include "createMesh.H"
    #include "readGravitationalAcceleration.H"
    #include "createFields.H"

    #include "createMomentFields.H"

    #include "createFvOptions.H"
    #include "createClouds.H"
    #include "createRadiationModel.H"
    #include "initContinuityErrs.H"
    #include "readTimeControls.H"
    #include "compressibleCourantNo.H"
    #include "setInitialDeltaT.H"
    #include "startSummary.H"
    pimpleControl pimple(mesh);


    //時間計測用
    scalar time_Other=0;
    scalar time_rhoEqn=0;
    scalar time_UEqn=0;
    scalar time_YEqn=0;
    scalar time_ODE=0;
    scalar time_EEqn=0;
    scalar time_pEqn=0;
    scalar time_turb=0;
    scalar time_Spray=0;
    scalar time_soot_other=0;
    scalar time_soot_correct=0;
    scalar time_soot_fields=0;
    scalar time_soot_Eqn=0;
    scalar time_soot_Mcorrect=0;
    scalar time_soot_DYsps=0;
    scalar time_soot_liner=0;

    clockTime clockTime;
    //計算時間の計測
    //clockTime clockTime;

    scalar loopN = 0;


#if defined( IN_SITU_VIS )
    // In-situ visualization setup
    Foam::messageStream::level = 0; // Disable Foam::Info
    const kvs::Indent indent(4); // indent for log stream
    local::InSituVis vis;
    if ( !vis.initialize() )
    {
        vis.log() << "ERROR: " << "Cannot initialize visualization process." << std::endl;
    }

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

    Info<< "\nStarting time loop\n" << endl;

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
        vis.simTimer().start();
#endif // IN_SITU_VIS

        Info<< "Time = " << runTime.timeName() << nl << endl;

        time_Other +=clockTime.timeIncrement(); //計算時間の計算

        parcels.evolve();

        time_Spray +=clockTime.timeIncrement(); //計算時間の計算

        #include "rhoEqn.H"

        time_rhoEqn +=clockTime.timeIncrement(); //計算時間の計算

        // --- Pressure-velocity PIMPLE corrector loop
        while (pimple.loop())
        {
            #include "UEqn.H"
            time_UEqn +=clockTime.timeIncrement(); //計算時間の計算

            #include "YEqn_.H"
            time_YEqn +=clockTime.timeIncrement(); //計算時間の計算

            #include "EEqn.H"
            time_EEqn +=clockTime.timeIncrement(); //計算時間の計算

            #include "sootMomentEqn.H"
            time_soot_other +=clockTime.timeIncrement(); //計算時間の計算


            // --- Pressure corrector loop
            while (pimple.correct())
            {
                #include "pEqn.H"
            }
            time_pEqn +=clockTime.timeIncrement(); //計算時間の計算


            if (pimple.turbCorr())
            {
                turbulence->correct();
            }
            time_turb +=clockTime.timeIncrement(); //計算時間の計算

        }

        rho = thermo.rho();

        if (runTime.write())
        {
            combustion->dQ()().write();
        }

#if defined( IN_SITU_VIS )
        vis.simTimer().stamp();
        const auto ts = vis.simTimer().last();
        const auto Ts = kvs::String::From( ts, 4 );
        vis.log() << indent << "Processing Times:" << std::endl;
        vis.log() << indent.nextIndent() << "Simulation: " << Ts << " s" << std::endl;

        // Execute in-situ visualization process
        // T: temperature
        auto& field = thermo.T();
        const auto min_value = 731.341;
        const auto max_value = 1031.611;

        // Convert OpenFOAM data to KVS data
        vis.impTimer().start();
        InSituVis::foam::FoamToKVS converter( field, false );
        using CellType = InSituVis::foam::FoamToKVS::CellType;
        auto vol = converter.exec( field, CellType::Hexahedra );
        vis.impTimer().stamp();

        vol.setMinMaxValues( min_value, max_value );

        // Execute visualization pipeline and rendering
        vis.visTimer().start();
        vis.put( vol );
        vis.exec( runTime.timeIndex() );
        vis.visTimer().stamp();

        const auto tv = vis.visTimer().last();
        const auto Tv = kvs::String::From( tv, 4 );
        vis.log() << indent.nextIndent() << "Visualization: " << Tv << " s" << std::endl;

        const auto elapsed_time = runTime.elapsedCpuTime();
        vis.log() << indent << "Elapsed Time: " << elapsed_time << " s" << std::endl;
        vis.log() << std::endl;
#endif // IN_SITU_VIS

        #include "logSummary_.H"

         Info<<"\n "
                << " time_Other " <<time_Other <<nl
                << " time_rhoEqn "<<time_rhoEqn<<nl
                << " time_UEqn  " <<time_UEqn<<nl
                << " time_YEqn  " <<time_YEqn<<nl
                << " time_EEqn  "<<time_EEqn<<nl
                << " time_pEqn  "<<time_pEqn<<nl
                << " time_turb  "<<time_turb<<nl
                << " time_Spray "<<time_Spray<<nl
                << " time_ODE "<<time_ODE<<nl
                << " time_soot_other "<<time_soot_other<<nl
                << " time_soot_correct "<<time_soot_correct<<nl
                << " time_soot_fields "<<time_soot_fields<<nl
                << " time_soot_Eqn "<<time_soot_Eqn<<nl
                << " time_soot_Mcorrect "<<time_soot_Mcorrect<<nl
                << " time_soot_DYsps "<<time_soot_DYsps<<nl
                << " time_soot_liner "<<time_soot_liner<<nl
                << endl;


        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
            // return(0);
            //

        loopN += 1;
        Info << "loop times = " << loopN << endl;

        //if (loopN > 9 ){break;}
    }

#if defined( IN_SITU_VIS )
    if ( !vis.finalize() )
    {
        vis.log() << "ERROR: " << "Cannot finalize visualization process." << std::endl;
    }
#endif // IN_SITU_VIS

    Info<< "End\n" << endl;

    return(0);
}




// ************************************************************************* //
