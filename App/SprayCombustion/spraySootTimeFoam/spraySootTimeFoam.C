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

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;

    while (runTime.run())
    {
        #include "readTimeControls.H"
        #include "compressibleCourantNo.H"
        #include "setDeltaT.H"

        runTime++;

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

    Info<< "End\n" << endl;

    return(0);
}




// ************************************************************************* //
