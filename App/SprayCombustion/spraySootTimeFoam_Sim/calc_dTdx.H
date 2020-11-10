// liner の ID を取得する
//label linerID = mesh.boundaryMesh().findPatchID("liner");

// 熱伝導率を取得する
volScalarField kappa("kappa", (thermo.kappa()));

//const polyPatch& cpatch = mesh.boundaryMesh()[linerID];

// シリンダ壁のエンタルピー勾配を取得する
const fvPatchScalarField& hew = thermo.he().boundaryField()[linerID];
const scalarField dHdn(mag(hew.snGrad()));

// シリンダ壁の有効熱拡散率を取得する
volScalarField alphaEff("alphaEff", (turbulence->alphaEff()));
const scalarField alphaEffW = alphaEff.boundaryField()[linerID];

forAll(mesh.boundary()[linerID], faceI)
{
    label fCellI = mesh.boundary()[linerID].faceCells()[faceI];

    scalar temp = (-1)*alphaEffW[faceI]*dHdn[faceI]/kappa[fCellI];

    vector nface = mesh.Sf().boundaryField()[linerID][faceI];
    scalar nfValue = mesh.magSf().boundaryField()[linerID][faceI];

    dTdx[fCellI] = temp * nface / nfValue;
    
    //vector term1 = cpatch.faceCentres()[faceI];
    //vector term2 = mesh.C()[fCellI];
    //dTdx[fCellI] = temp * (term1 - term2) / mag(term1 - term2);    // temp は負

}

