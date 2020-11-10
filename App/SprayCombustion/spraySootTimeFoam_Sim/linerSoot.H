// フェース中心を得るためにパッチを取得する
const polyPatch& linerPatch = mesh.boundaryMesh()[linerID];

// 時間幅を取得する
scalar deltaT = runTime.deltaTValue();

// 筒内のすす量を保存する
forAll(sMass, celli)
{
	dimensionedScalar vol = mesh.V()[celli];
	sMass[celli] = M1[celli] * vol.value();
}

// 壁に付着した量を足していく
forAll(mesh.boundary()[linerID], faceI)
{
	label fcellI = mesh.boundary()[linerID].faceCells()[faceI];

	const scalar yn = mag(linerPatch.faceCentres()[faceI] - mesh.C()[fcellI]);
        const scalar uth = mag(Utherm[fcellI]);
	dimensionedScalar vol = mesh.V()[fcellI];
	
	sMass.boundaryField()[linerID][faceI] += uth * M1[fcellI] * vol.value() * deltaT / yn;
}
