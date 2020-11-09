// PAH とすすの量を表すことにした
// sprayFoam.C において logSummary.H の代わりにこのヘッダーファイルを使用する

Info<< "Mean pressure:" << p.weightedAverage(mesh.V()).value() << endl;
Info<< "Mean temperature:" << T.weightedAverage(mesh.V()).value() << endl;
Info<< "Mean u':"
    << (sqrt((2.0/3.0)*turbulence->k()))().weightedAverage(mesh.V()).value()
    << endl;


scalar cellV=0.0;
scalar cellkappa=0.0;
scalar totalMass=0.0;
scalar totaldQ=0.0;
scalar totalvfuel=0.0;
scalar totalCO2=0.0;
scalar Soot = 0.0;
scalar PAHmass = 0.0;
forAll(mesh.cells(),cid)
{
//	const cell &c = mesh.cells()[cid];
	cellV +=mesh.V()[cid];
	totalMass += rho[cid]*mesh.V()[cid];
	totaldQ   += dQ[cid];
	totalvfuel +=Y[0][cid]*rho[cid]*mesh.V()[cid];
	totalCO2 +=Y[2][cid]*rho[cid]*mesh.V()[cid];
    Soot += M1[cid]*mesh.V()[cid];
    PAHmass += Y[PAHI][cid]*rho[cid]*mesh.V()[cid];
}

reduce(totalMass,sumOp<scalar>());
reduce(totaldQ,sumOp<scalar>());
reduce(totalvfuel,sumOp<scalar>());
reduce(Soot,sumOp<scalar>());
reduce(PAHmass,sumOp<scalar>());

QALL+=totaldQ*runTime.deltaTValue();
totalCO2=totalCO2/totalMass;
scalar sootM = Soot*12*1.6605*1e-27;

Info<<"total heat release: "<<QALL<<endl;

Info<< "cell Volume V:"
    << cellV
    << endl;


Info<< "totalMass:"  << totalMass << endl;
Info<< "totalFuel:"  << totalvfuel << endl;

Info<< "Mean Cp/Cv':"
    << (thermo.Cp()/thermo.Cv())().weightedAverage(mesh.V()).value()
    << endl;
Info<< "soot mass [kg] : " << sootM << endl;

//熱流束の計算
{
    //#include "wallFvPatch.H"
     //volScalarField& he = thermo.he();
     
     surfaceScalarField heatFlux
    (
        fvc::interpolate
        (
            turbulence->alphaEff()
        )*fvc::snGrad(thermo.he())
    );

    const surfaceScalarField::GeometricBoundaryField& patchHeatFlux =
    heatFlux.boundaryField();


    Info<< "Wall Heat Fluxes [W]" << endl;
    forAll(patchHeatFlux, patchi)
    {   
        if (isA<wallFvPatch>(mesh.boundary()[patchi]))
        {   
            Info<< mesh.boundary()[patchi].name()
                << " " 
                << gSum
                    (   
                        mesh.magSf().boundaryField()[patchi]
                        *patchHeatFlux[patchi]
                    )   
                << endl;
        }   
    }   

}

const scalar pave= p.weightedAverage(mesh.V()).value();
const scalar Tave= T.weightedAverage(mesh.V()).value();
const scalar kave= (sqrt((2.0/3.0)*turbulence->k()))().weightedAverage(mesh.V()).value();
const scalar kappaave=  (thermo.Cp()/thermo.Cv())().weightedAverage(mesh.V()).value();
const scalar maxT_ = max(T).value();
const scalar minT_ = min(T).value();

logSummaryFile
    << runTime.timeName() << tab
    << pave << tab
    << Tave << tab
    << maxT_ << tab
    << minT_ << tab
    << kave << tab
    << cellV << tab
    << kappaave << tab
    << totalMass <<tab
    << totaldQ <<tab
    << QALL <<tab
    << totalvfuel<<tab
    << totalCO2 << tab
    << PAHmass << tab
    << sootM
    << endl;
