// モーメント場を定義するためのヘッダーファイル
// sprayEngineKIVAFoam.C ファイルに #include "createFields.H" の次に入れる
// 計算に必要な幾つかの化学種のidを取得するlabelも定義している

///// Moment terms /////// M0, M1, M2, M3, M4, M5 ///////////////
// volScalarField M0
// (
// 	IOobject
//     	(
//         	"M0",
//             	runTime.timeName(),
//             	mesh,
//             	IOobject::MUST_READ,
//             	IOobject::AUTO_WRITE
//         ),
//        	 mesh
//         //dimensionedScalar("M0", dimless, 0)
// );

// volScalarField M1
// (
//         IOobject
//         (
//             	"M1",
//             	runTime.timeName(),
//             	mesh,
//             	IOobject::MUST_READ,
//             	IOobject::AUTO_WRITE
//         ),
//         mesh
//         //dimensionedScalar("M1", dimless, 0)
// );

// volScalarField M2
// (
//         IOobject
//         (
//             	"M2",
//             	runTime.timeName(),
//             	mesh,
//             	IOobject::MUST_READ,
//             	IOobject::AUTO_WRITE
//         ),
//         mesh
//         //dimensionedScalar("M2", dimless, 0)
// );

// volScalarField M3
// (
//         IOobject
//         (
//             	"M3",
//             	runTime.timeName(),
//             	mesh,
//             	IOobject::MUST_READ,
//             	IOobject::AUTO_WRITE
//         ),
//         mesh
//         //dimensionedScalar("M3", dimless, 0)
// );

// volScalarField M4
// (
//         IOobject
//         (
//             	"M4",
//             	runTime.timeName(),
//             	mesh,
//             	IOobject::MUST_READ,
//             	IOobject::AUTO_WRITE
//         ),
//         mesh
//         //dimensionedScalar("M4", dimless, 0)
// );

// volScalarField M5
// (
//         IOobject
//         (
//             	"M5",
//             	runTime.timeName(),
//             	mesh,
//             	IOobject::MUST_READ,
//             	IOobject::AUTO_WRITE
//         ),
//         mesh
//         //dimensionedScalar("M5", dimless, 0)
// );
///////////////////////////////////////////////////////////

// // すすの体積分率
/* volScalarField FVSoot
 (
         IOobject
         (
             	"FVSoot",
             	runTime.timeName(),
             	mesh,
             	IOobject::NO_READ,
             	IOobject::AUTO_WRITE
         ),
         mesh,
         dimensionedScalar("FVSoot", dimless, 0)
 );*/

// // 化学種の消費速度
// volScalarField DYi
// (
//         IOobject
//         (
//             	"DYi",
//             	runTime.timeName(),
//             	mesh,
//             	IOobject::NO_READ,
//             	IOobject::NO_WRITE
//         ),
//         mesh,
//         dimensionedScalar("DYi", dimensionSet(1, -3, -1, 0, 0, 0, 0), 0)
// );

// // すす粒子の平均直径
/* volScalarField sootD
 (
         IOobject
         (
             	"sootD",
             	runTime.timeName(),
             	mesh,
             	IOobject::NO_READ,
             	IOobject::AUTO_WRITE
         ),
         mesh,
         dimensionedScalar("sootD", dimensionSet(0, 1, 0, 0, 0, 0, 0), 0)
 );*/

// // すす粒子に対するKnudsen数(気体の平均自由行程と粒子直径の比)
/* volScalarField Knudsen
 (
         IOobject
         (
             	"Knudsen",
             	runTime.timeName(),
             	mesh,
             	IOobject::NO_READ,
             	IOobject::AUTO_WRITE
         ),
         mesh,
         dimensionedScalar("Knudsen", dimless, 0)
 );*/

///////////////////////////////////////////////////////////////////////

label linerID = mesh.boundaryMesh().findPatchID("liner"); 

//// chemical species index //////
label C2H2I  = combustion->thermo().composition().species()["C2H2"];
label O2I  = combustion->thermo().composition().species()["O2"];
label HI  = combustion->thermo().composition().species()["H"];
label H2I  = combustion->thermo().composition().species()["H2"];
label OHI  = combustion->thermo().composition().species()["OH"];
label COI  = combustion->thermo().composition().species()["CO"];
label H2OI  = combustion->thermo().composition().species()["H2O"];
label PAHI  = combustion->thermo().composition().species()["PAH"];      // 反応機構がPAHを含むと前提

///////////////////////////////////////////////////////////////////////
// すすとの反応に関わる化学種の群れ
label sreacSpecies[8] = {C2H2I, O2I, HI, H2I, OHI, COI, H2OI, PAHI};


// PtrList<volScalarField> Moment(5);

// Moment.set(0,M0);
// Moment[1]=M1;
// Moment[2]=M2;
// Moment[3]=M3;
// Moment[4]=M4;
// Moment[5]=M5;

labelList sreacSpecies_(8);
sreacSpecies_[0]=C2H2I;
sreacSpecies_[1]=O2I;
sreacSpecies_[2]=HI;
sreacSpecies_[3]=H2I;
sreacSpecies_[4]=OHI;
sreacSpecies_[5]=COI;
sreacSpecies_[6]=H2OI;
sreacSpecies_[7]=PAHI;

labelList ch_species_(7);
ch_species_[0]=PAHI;
ch_species_[1]=C2H2I;
ch_species_[2]=O2I;
ch_species_[3]=OHI;
ch_species_[4]=HI;
ch_species_[5]=H2I;
ch_species_[6]=H2OI;


Info<<"Initiation of sootMoment class"<<endl;

sootMoment sootMoment(mesh, rho, T, p, ch_species_, Y, composition);

Info<<"Initiation of sootMoment class end"<<endl;


PtrList<volScalarField>& Moments =sootMoment.Moments();

// volScalarField& M0=sootMoment.M0();
// volScalarField& M1=sootMoment.M1();
// volScalarField& M2=sootMoment.M2();
// volScalarField& M3=sootMoment.M3();
// volScalarField& M4=sootMoment.M4();
// volScalarField& M5=sootMoment.M5();


volScalarField& M0=Moments[0];
volScalarField& M1=Moments[1];
volScalarField& M2=Moments[2];
volScalarField& M3=Moments[3];
volScalarField& M4=Moments[4];
volScalarField& M5=Moments[5];

volScalarField sMass
(
         IOobject
         (
             	"sMass",
             	runTime.timeName(),
             	mesh,
             	IOobject::MUST_READ,
             	IOobject::AUTO_WRITE
         ),
         mesh,
         dimensionedScalar("sMass", dimless, 0)
);

volScalarField& FVSoot=sootMoment.FVSoot();
volScalarField& DYi=sootMoment.DYi();
volScalarField& sootD=sootMoment.sootDAve();
volScalarField& Knudsen=sootMoment.Knudsen();


// 上の群れに対して計算する消費速度
volScalarField DYsps[9] = {DYi, DYi, DYi, DYi, DYi, DYi, DYi, DYi, DYi};



// FatalErrorIn("test")
//            << abort(FatalError);
    
