{
    //Foam::clockTime clockEEqn;

    //scalar time0=clockEEqn.timeIncrement();

    volScalarField& he = thermo.he();

    fvScalarMatrix EEqn
    (
        fvm::ddt(rho, he) + mvConvection->fvmDiv(phi, he)
      + fvc::ddt(rho, K) + fvc::div(phi, K)
      + (
            he.name() == "e"
          ? fvc::div
            (
                fvc::absolute(phi/fvc::interpolate(rho), U),
                p,
                "div(phiv,p)"
            )
          : -dpdt
        )
      - fvm::laplacian(turbulence->alphaEff(), he)
     ==
        parcels.Sh(he)
      + radiation->Sh(thermo)
      + combustion->Sh()
      + fvOptions(rho, he)
    );


    //   fvScalarMatrix EEqn
    // (
    //     fvm::ddt(rho, he)
    //   + mvConvection->fvmDiv(phi, he)
    //   + fvc::ddt(rho, K) + fvc::div(phi, K)
    //   + (
    //         he.name() == "e"
    //       ? fvc::div
    //         (
    //             fvc::absolute(phi/fvc::interpolate(rho), U),
    //             p,
    //             "div(phiv,p)"
    //         )
    //       : -dpdt
    //     )
    //   - fvm::laplacian(turbulence->alphaEff(), he)
    // );



    

    //scalar time1=clockEEqn.timeIncrement();

    EEqn.relax();
    //scalar time2=clockEEqn.timeIncrement();

    fvOptions.constrain(EEqn);
    //scalar time3=clockEEqn.timeIncrement();

    EEqn.solve();
    //scalar time4=clockEEqn.timeIncrement();

    fvOptions.correct(he);
    //scalar time5=clockEEqn.timeIncrement();

    thermo.correct();
    //scalar time6=clockEEqn.timeIncrement();

    radiation->correct();
    //scalar time7=clockEEqn.timeIncrement();


    Info<< "T gas min/max   = " << min(T).value() << ", "
        << max(T).value() << endl;

    //scalar time8=clockEEqn.timeIncrement();

    /*Info
    << "time1 "<<time1<<nl
    << "time2 "<<time2<<nl
    << "time3 "<<time3<<nl
    << "time4 "<<time4<<nl
    << "time5 "<<time5<<nl
    << "time6 "<<time6<<nl
    << "time7 "<<time7<<nl
    << "time8 "<<time7<<nl
    << endl;*/

}
