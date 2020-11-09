{
  // すすモーメント及び化学種濃度変化を計算するヘッダーファイル
  // sprayEngineKIVAFoam.C ファイルに #include "YEqn.H" の次に入れる

  // 全領域にある PAH, M0, C2H2, OH の量を表示する     // M0 はすすの 0 次のモーメント
  dimensionedScalar totalM0 = fvc::domainIntegrate(M0);
  dimensionedScalar totalMassPAH_ = fvc::domainIntegrate(rho*Y[PAHI]);
  //dimensionedScalar totalc2h2 = fvc::domainIntegrate(rho*Y[C2H2I]);
  //dimensionedScalar totalOH = fvc::domainIntegrate(rho*Y[OHI]);
  Info << "Total PAH mass : " << totalMassPAH_.value() << endl;
  Info << "Total M0 : " << totalM0.value() << endl;
  //Info << "Total C2H2 mass : " << totalc2h2.value() << endl;
  //Info << "Total OH mass : " << totalOH.value() << endl;


  // PAH または M0 が存在する時のみにモーメント計算を行いたい
  Switch momentCalc = false;
  if (totalMassPAH_.value() > 1e-30 || totalM0.value() > 0.0)
  {
    momentCalc = true;
  }
  
  time_soot_other += clockTime.timeIncrement();  

  // // 前計算ステップからモーメント場を取得する
  // volScalarField Moment[6] = {M0, M1, M2, M3, M4, M5};

  // // 温度を const volScalarField から変換する
  // volScalarField Temp("Temp", (T));

  // // 必要な化学種の label の array を作る
  // label ch_species[7] = {PAHI, C2H2I, O2I, OHI, HI, H2I, H2OI};

  // 燃焼が起きて PAH が生成された時から計算が始まる
  if(momentCalc == true)
  {
    // 計算のためにクラスオブジェクトを定義する     // 引数の入力順番に要注意
    // pyMoment mClass(Moment, rho, Temp, p, ch_species, Y, composition);

    sootMoment.correct();    

    sootMoment.errorTestValue();
    
    time_soot_correct += clockTime.timeIncrement();  
    // クラスからモーメント拡散係数及び熱泳動係数を取得する   // すすがない場合、値が 0 になる
     volScalarField Ddif = sootMoment.Ddiffunc();
     volScalarField Dther = sootMoment.Dtherfunc();

    //tmp<volScalarField> Ddif = sootMoment.Ddiffunc();
    //tmp<volScalarField> Dther = sootMoment.Dtherfunc();
   
    //dTdx = fvc::grad(T);
    //#include "calc_dTdx.H"

    // 熱泳動速度場を定義する
    //tmp<volVectorField> Utherm = volVectorField("Utherm", (Dther*fvc::grad(T)));
    volVectorField Utherm = volVectorField("Utherm", (Dther*dTdx));
    // 熱泳動速度流出場を定義する
    surfaceScalarField phiUtherm
    (
        IOobject
        (
            "phiUtherm",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        linearInterpolate(Utherm) & mesh.Sf()
    );

    // モーメント輸送方程式に使うために速度流出場を定義する
    surfaceScalarField phiNew
    (
        IOobject
        (
            "phiNew",
            runTime.timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        linearInterpolate(U) & mesh.Sf()
    );

    
    time_soot_fields += clockTime.timeIncrement();  

    // モーメントの生成項がある大きさ（fvSolutionの条件による）を超えると計算が始まる.
    // 定義により高次のモーメントの生成項が大きいため、M0より先にそれらの計算が始まる.
    // しかし、それが物理的に不可能であるため、M0の生成が必ず先に起きるようにする.
    fvScalarMatrix M0Eqn
    (
      fvm::ddt(M0)
      + fvm::div(phiNew, M0)
      - fvm::laplacian(Ddif, M0)
      + fvm::div(phiUtherm, M0)
      ==
      sootMoment.totalMoment(0)
    );
    
    // 計算実行
    M0Eqn.relax();
    solve(M0Eqn);

    
    // M0 の生成を確認の上、高次のモーメントの計算に進む
    totalM0 = fvc::domainIntegrate(M0);

    if (totalM0.value())
    {
      // モーメント計算マトリクスの定義
      fvScalarMatrix M1Eqn
      (
          fvm::ddt(M1)
        + fvm::div(phiNew, M1)
        - fvm::laplacian(Ddif, M1)
        + fvm::div(phiUtherm, M1)
      ==
          sootMoment.totalMoment(1)
      );

      fvScalarMatrix M2Eqn
      (
          fvm::ddt(M2)
        + fvm::div(phiNew, M2)
        - fvm::laplacian(Ddif, M2)
        + fvm::div(phiUtherm, M2)
      ==
          sootMoment.totalMoment(2)
      );

      fvScalarMatrix M3Eqn
      (
          fvm::ddt(M3)
        + fvm::div(phiNew, M3)
        - fvm::laplacian(Ddif, M3)
        + fvm::div(phiUtherm, M3)
      ==
          sootMoment.totalMoment(3)
      );

      fvScalarMatrix M4Eqn
      (
          fvm::ddt(M4)
        + fvm::div(phiNew, M4)
        - fvm::laplacian(Ddif, M4)
        + fvm::div(phiUtherm, M4)
      ==
          sootMoment.totalMoment(4)
      );

      fvScalarMatrix M5Eqn
      (
          fvm::ddt(M5)
        + fvm::div(phiNew, M5)
        - fvm::laplacian(Ddif, M5)
        + fvm::div(phiUtherm, M5)
      ==
          sootMoment.totalMoment(5)
      );

      M1Eqn.relax();
      M2Eqn.relax();
      M3Eqn.relax();
      M4Eqn.relax();
      M5Eqn.relax();

      // 計算実行
      solve(M1Eqn);
      solve(M2Eqn);
      solve(M3Eqn);
      solve(M4Eqn);
      solve(M5Eqn);

      time_soot_Eqn += clockTime.timeIncrement();  
      // モーメント値が負になってはいけないため、全ての計算値をここで直す //
      sootMoment.positiveCorrect(&M0);
      sootMoment.positiveCorrect(&M1);
      sootMoment.positiveCorrect(&M2);
      sootMoment.positiveCorrect(&M3);
      sootMoment.positiveCorrect(&M4);
      sootMoment.positiveCorrect(&M5);


      time_soot_Mcorrect += clockTime.timeIncrement();  
      // 様々なフィルド値を修得する //クラスで自動更新される
      //FVSoot = sootMoment.FVSoot();
      //sootD = sootMoment.sootD();
      //Knudsen = sootMoment.Knudsen();
      
      /*volScalarField dummyS = volScalarField("dummyS", sootMoment.dummySD());
      dimensionedScalar sootAveD = fvc::domainIntegrate(dummyS);
      Info <<"Volume Integrate of soot D = " << sootAveD.value() << endl;*/

      DYsps[1] = composition.W(C2H2I)*0.001*sootMoment.DC2H2();    
      DYsps[2] = composition.W(O2I)*0.001*sootMoment.DO2();
      DYsps[3] = composition.W(HI)*0.001*sootMoment.D_H();
      DYsps[4] = composition.W(H2I)*0.001*sootMoment.D_H2();
      DYsps[5] = composition.W(OHI)*0.001*sootMoment.DOH();
      DYsps[6] = composition.W(COI)*0.001*sootMoment.D_CO();
      DYsps[7] = composition.W(H2OI)*0.001*sootMoment.DH2O();
      DYsps[8] = composition.W(PAHI)*0.001*sootMoment.DPAH();


      time_soot_DYsps += clockTime.timeIncrement();  

      //#include "linerSoot.H"


      time_soot_liner += clockTime.timeIncrement();  

      // 計算後のPAH質量を表示する
      totalM0 = fvc::domainIntegrate(M5);
      Info << "Total M5 : " << totalM0.value() << endl;
    }
  }
}
