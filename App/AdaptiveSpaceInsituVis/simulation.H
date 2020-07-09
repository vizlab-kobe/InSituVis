#include "readTimeControls.H"
#include "compressibleCourantNo.H"
#include "setDeltaT.H"

runTime++;

Info << "Time = " << runTime.timeName() << nl << endl;

if ( pimple.nCorrPIMPLE() <= 1 )
{
    #include "rhoEqn.H"
}

// --- Pressure-velocity PIMPLE corrector loop
while ( pimple.loop() )
{
    #include "UEqn.H"
    #include "EEqn.H"

    // --- Pressure corrector loop
    while ( pimple.correct() )
    {
        #include "pEqn.H"
    }

    if (pimple.turbCorr())
    {
        turbulence->correct();
    }
}

runTime.write();

