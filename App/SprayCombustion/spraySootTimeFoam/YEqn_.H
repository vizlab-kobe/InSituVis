// すすと反応すす化学種の消費速度を保存式に入れた
// sprayFoam.C において YEqn.H の代わりにこのヘッダーファイルを使用する

tmp<fv::convectionScheme<scalar> > mvConvection
(
    fv::convectionScheme<scalar>::New
    (
        mesh,
        fields,
        phi,
        mesh.divScheme("div(phi,Yi_h)")
    )
);

{

    time_YEqn +=clockTime.timeIncrement(); //計算時間の計算

    combustion->correct();
    dQ = combustion->dQ();

    time_ODE +=clockTime.timeIncrement(); //計算時間の計算

    
    label inertIndex = -1;
    volScalarField Yt(0.0*Y[0]);

    forAll(Y, i)
    {
        if (Y[i].name() != inertSpecie)
        {
            volScalarField& Yi = Y[i];

            label yy = 0;
            for (label k = 0; k < 8; k++)
            {
		        if (i == sreacSpecies[k])
                {
		            yy = k+1;
		        }
	        }

            fvScalarMatrix YEqn
            (
                fvm::ddt(rho, Yi)
              + mvConvection->fvmDiv(phi, Yi)
              - fvm::laplacian(turbulence->muEff(), Yi)
             ==
                parcels.SYi(i, Yi)
              + combustion->R(Yi)
	          - DYsps[yy]
              + fvOptions(rho, Yi)
            );

            YEqn.relax();

            fvOptions.constrain(YEqn);

            YEqn.solve(mesh.solver("Yi"));

            fvOptions.correct(Yi);

            Yi.max(0.0);
            Yt += Yi;
        }
        else
        {
            inertIndex = i;
        }
    }

    Y[inertIndex] = scalar(1) - Yt;
    Y[inertIndex].max(0.0);
}
