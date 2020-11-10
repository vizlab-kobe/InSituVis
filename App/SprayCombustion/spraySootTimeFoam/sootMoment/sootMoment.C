// モーメントの計算に必要なクラス及びモーメントの修正を行う関数を定義したヘッダーファイル
// sprayEngineKIVAFoam.C において #include "wallFvPatch.H" の次に、main 関数が始まる前に入れておく

#include "sootMoment.H"
// #include<cmath>


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //



 Foam::volScalarField Foam::sootMoment::normM(int i)
{
//Info << "nomM start for " << i << endl;
    volScalarField tempM("tempM", (T_/makeTDimless)); // dummy expression
    tempM *= scalar(0);

    forAll(tempM, cellI)
    {
        if(Mold_[0][cellI] > MSMALL_ && Mold_[i][cellI] > MSMALL_)
        {
            tempM[cellI] = Mold_[i][cellI]/Mold_[0][cellI];
        }
    }

    forAll(tempM.boundaryField(), patchi)
    {
            fvPatchScalarField &fbp = tempM.boundaryField()[patchi];
        const fvPatchScalarField &fbM0 = Mold_[0].boundaryField()[patchi];
        const fvPatchScalarField &fbMi = Mold_[i].boundaryField()[patchi];
        const fvPatch &patch = fbp.patch();

        forAll(patch, facei){
            if (fbM0[facei] > MSMALL_ && fbMi[facei] > MSMALL_)
                fbp[facei] = fbMi[facei]/fbM0[facei];
        }
    }
//Info << "normM end" << endl;
    return tempM;
}


       
Foam::volScalarField Foam::sootMoment::Rr_(int r)
{
    // 生成項の次元を合わせるためこれで割る
    dimensionedScalar rrDimCorrect = dimensionedScalar("rrDimCorrect", dimensionSet(2,-6,1,0,0,0,0), 1.0);
   
    volScalarField sum("sum", (T_/makeTDimless/makeTimeDimless));
    sum *= scalar(0);

    // scalarField alphaPAH(T_.size(),-1);
    // scalarField c_PAH(T_.size(),-1);

    // alphaPAH=( 2.2*Foam::sqrt( (M_PI*kB_*T_/makeTDimless)/mPAH_ ) * Foam::pow( (2.0*dPAH_),2.0 ) );
    // c_PAH = (rho_*1000.0*(Y_[PAHI_]) / composition_.W(PAHI_));           // Pyreneのモル濃度 (mol/m3)

    // alphaPAH (volScalarField("alphaPAH", (2.2*Foam::sqrt((M_PI*kB_*T_/makeTDimless)/mPAH_)*Foam::pow((2.0*dPAH_),2.0)))),

    for (int k = 0; k <= r; k++)
    {
        // scalar alphaPAH= (2.2*Foam::sqrt((M_PI*kB_*T_/makeTDimless)/mPAH_)*Foam::pow((2.0*dPAH_),2.0)));
        sum += nPr(r,k)*0.5*Foam::pow((2.0*CPAH_),scalar(r))*alphaPAH_* c_PAH_ * c_PAH_ *Na_ *Na_/rrDimCorrect;
        // sum += nPr(r,k)*0.5*Foam::pow((2.0*CPAH_),scalar(r))*alphaPAH* c_PAH * c_PAH *Na_ *Na_;
    }

    return sum;
}




Foam::volScalarField Foam::sootMoment::Mmix()
{
    volScalarField sum("sum", (T_/makeTDimless)); // dummy expression
    sum *= scalar(0);

    forAll(sum, cellI)
    {
        forAll(Y_, i)
        {
            // if (Y_[i][cellI] > VSMALL)
                sum[cellI] +=  Y_[i][cellI]/composition_.W(i) ;
        }


        if (sum[cellI] < VSMALL)
        {
            Info << "Ave. MMass " << sum[cellI] << " unstable internally " << endl;
        } 
        else
        {
            sum[cellI] = 1.0/sum[cellI];
            // 平均モル質量が 0.01g から 300g の間ではないとおかしい
            if (sum[cellI] < 0.1 || sum[cellI] > 300)
            {
                sum[cellI] = 0.0;
            }
            else
            {
                // FatalErrorIn("sootMoment")
                //  <<"平均モル質量のエラー"
                //  << abort(FatalError);
            }
            
        }
    }

    forAll(sum.boundaryField(), patchi)
    {
        const fvPatchScalarField &fbp_ = sum.boundaryField()[patchi];
        fvPatchScalarField &fbp = const_cast<fvPatchScalarField&>(sum.boundaryField()[patchi]);
        const fvPatch &patch = fbp_.patch();
        
        forAll(patch, facei)
        {
            //const label owner = patch.faceCells()[facei];
            forAll(Y_, j)
            {
                const fvPatchScalarField &fby = Y_[j].boundaryField()[patchi];
                if (fby[facei] > VSMALL) fbp[facei] +=  (fby[facei]/composition_.W(j));
            }

            if (fbp_[facei] < VSMALL)
            {
                Info << "Average molar mass is unstable on boundary " << patchi << endl;
            }
            else
            {
                fbp[facei]=1.0/fbp[facei];
                // 平均モル質量が 0.1g から 300g の間ではないとおかしい
                if (fbp_[facei] < 0.1 || fbp_[facei] > 300)
                    fbp[facei] = 0.0;
            }
        }
    }
    return sum;
}


void Foam::sootMoment::calcDaveS()
{
    // volScalarField DaveS_("DaveS_", (T_*makeLDimless/makeTDimless)); // dummy expression
    // DaveS_ *= scalar(0);
  
    forAll(DaveS_, cellI)
    {
        if(Mold_[0][cellI] > MSMALL_ && Mold_[1][cellI] > MSMALL_)
        {
            DaveS_[cellI]= Foam::pow(6.0*Mc_*Mold_[1][cellI]/(M_PI*rhoS_*Mold_[0][cellI]), 1.0/3.0);

            if(DaveS_[cellI] < 1e-9) DaveS_[cellI] = 0.0;
        }
    }

    forAll(DaveS_.boundaryField(), patchi)
    {
        const fvPatchScalarField &fbp_ = DaveS_.boundaryField()[patchi];
         fvPatchScalarField &fbp = const_cast<fvPatchScalarField&>(DaveS_.boundaryField()[patchi]);

        const fvPatchScalarField &fbM0 = Mold_[0].boundaryField()[patchi];
        const fvPatchScalarField &fbM1 = Mold_[1].boundaryField()[patchi];
        const fvPatch &patch = fbp_.patch();

        forAll(patch, facei)
        {
            if (fbM0[facei] > MSMALL_ && fbM1[facei] > MSMALL_)
            {
                //fbp[facei] = Da*Foam::sqrt((2*fbM1[facei])/(3*fbM0[facei]));
                fbp[facei] = Foam::pow(6.0*Mc_*fbM1[facei]/(M_PI*rhoS_*fbM0[facei]), 1.0/3.0); //thridに置き換え予定

                if(fbp_[facei] < 1e-9)
                    fbp[facei] = 0.0;
            }
        }
    }

    //return ;
}


// lamda_ (volScalarField("lamda_", (2.0*corrLamdaDim*nu_*rho_/(p_*Foam::sqrt(8.0*Mmix()*0.001*makeTDimless/(M_PI*R_*T_)))))),

void Foam::sootMoment::calcLamda()
{
    dimensionedScalar corrLamdaDim = dimensionedScalar("corrLamdaDim", dimensionSet(0,3,-2,0,0,0,0), 1.0);
    lamda_=2.0*corrLamdaDim*nu_*rho_/(p_*Foam::sqrt(8.0*Mmix()*0.001*makeTDimless/(M_PI*R_*T_)));
}

void Foam::sootMoment::calcKn()
{
  
    forAll(Kn_, cellI)
    {
        if(DaveS_[cellI] > MSMALL_){
            Kn_[cellI] = 2.0*lamda_[cellI]/DaveS_[cellI];
        }
    }

    forAll(Kn_.boundaryField(), patchi)
    {
        fvPatchScalarField &fbp = Kn_.boundaryField()[patchi];
        fvPatchScalarField &fbD = DaveS_.boundaryField()[patchi];
        fvPatchScalarField &fbL = lamda_.boundaryField()[patchi];
        const fvPatch &patch = fbp.patch();

        forAll(patch, facei)
        {
            // if(fbD[facei] > MSMALL_)
                fbp[facei] = 2.0*fbL[facei]/(fbD[facei]+VSMALL);
        }
    }

    //return ;
}


Foam::volScalarField Foam::sootMoment::MU_calc(int r)
{
    switch(r)
    {
        case 0:
            return mu_0;

        case 1:
            return mu_1;

        case 2:
            return mu_2;

        case 3:
            return mu_3;

        case 4:
            return mu_4;

        case 5:
            return mu_5;
    }
}


// // ///// 一項ラグランジュ補間を求める関数 ///////////////
Foam::volScalarField Foam::sootMoment::lng1(scalar x, int rmax)
{
    volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimless, 0.0)
    );
    
    //  =volScalarField("result", (T_/makeTDimless));     // dummy expression
    // result *= scalar(0);

    if ( 0.0 < x && x < 2.0)  // rmax-2 までのモーメント値を用いて log(u_x) の補間値を求める	// rmax-2 for x<3
    { 
        for (int i=0; i < 3; i++)
        {
            // volScalarField term = volScalarField("term", (T_/makeTDimless));     // dummy expression
            // term *= scalar(0);

             volScalarField term
            (
                IOobject
                (
                    "term",
                    mesh_.time().timeName(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                ),
                mesh_,
                dimensionedScalar("term", dimless, 0.0)
            );


            volScalarField moment("moment", (MU_calc(i)));


            forAll(result, cellI)
            {
                //Info << "term[cellI] = " << term[cellI] << endl;	

                if(moment[cellI] > MSMALL_)
                {
                    term[cellI] = Foam::log(moment[cellI]);

                    for (int j=0; j < 3; j++)
                    {
                        if (j != i)
                        {
                            term[cellI] = term[cellI]*(x - scalar(j))/(scalar(i - j) + VSMALL);
                        }
                    }
                    result[cellI] += term[cellI];
                }
            }

    //Info << "good here1" << endl;

            forAll(result.boundaryField(), patchi){
                fvPatchScalarField &fbp = const_cast<fvPatchScalarField&>(result.boundaryField()[patchi]);
                fvPatchScalarField &fbT = const_cast<fvPatchScalarField&>(term.boundaryField()[patchi]);
                const fvPatchScalarField &fbM = moment.boundaryField()[patchi];
                const fvPatch &patch = fbp.patch();

                forAll(patch, facei)
                {
                    if(fbM[facei] > MSMALL_)
                    {
                        fbT[facei] = Foam::log(fbM[facei]);
                        for (int j=0; j < 3; j++)
                        {
                            if (j != i)
                            {
                                fbT[facei] = fbT[facei]*(x - scalar(j))/(scalar(i - j) + VSMALL);
                            }
                        }
                        fbp[facei] += fbT[facei];
                    }
                }
            }
        }
    }
    else if (2.0 < x && x < 4.0 )  // rmax-2 までのモーメント値を用いて log(u_x) の補間値を求める	// rmax-2 for x<3
    { 
        for (int i=2; i < 5; i++)
        {
             volScalarField term
            (
                IOobject
                (
                    "term",
                    mesh_.time().timeName(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                ),
                mesh_,
                dimensionedScalar("term", dimless, 0.0)
            );

            volScalarField moment("moment", (MU_calc(i)));
            
            forAll(term,aiu){
                    if(moment[aiu] > MSMALL_){
                        term[aiu] = Foam::log(moment[aiu]);

                        for (int j=2; j < 5; j++){
                        if (j != i){
                            term[aiu] = term[aiu]*(x - scalar(j))/(scalar(i - j) + VSMALL);
                        }
                    }
                        result[aiu] += term[aiu];
                }
            }

            forAll(result.boundaryField(), patchi)
            {
                fvPatchScalarField &fbp = result.boundaryField()[patchi];
                fvPatchScalarField &fbT = term.boundaryField()[patchi];
                fvPatchScalarField &fbM = moment.boundaryField()[patchi];
                const fvPatch &patch = fbp.patch();

                forAll(patch, facei)
                {
                    if(fbM[facei] > MSMALL_){
                        fbT[facei] = Foam::log(fbM[facei]);
                        for (int j=2; j < 5; j++){
                            if (j != i){
                                fbT[facei] = fbT[facei]*(x - scalar(j))/(scalar(i - j) + VSMALL);
                            }
                        }
                        fbp[facei] += fbT[facei];
                    }
                }
            }
        }
    }
    else if (x > 4.0)  // rmax-3 から rmax までのモーメント値を用いて log(u_x) の補間値を求める
    { 
        for (int i = 3; i < rmax; i++){
             volScalarField term
            (
                IOobject
                (
                    "term",
                    mesh_.time().timeName(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                ),
                mesh_,
                dimensionedScalar("term", dimless, 0.0)
            );
            volScalarField moment("moment", (MU_calc(i)));
            
            forAll(term,aiu){
                    if(moment[aiu] > MSMALL_){
                        term[aiu] = Foam::log(moment[aiu]);

                        for (int j = 3; j < rmax; j++){
                        if (j != i){
                            term[aiu] = term[aiu]*(x - scalar(j))/(scalar(i - j) + VSMALL);
                        }
                    }
                        result[aiu] += term[aiu];
                }
            }

            forAll(result.boundaryField(), patchi){
                fvPatchScalarField &fbp = result.boundaryField()[patchi];
                fvPatchScalarField &fbT = term.boundaryField()[patchi];
                fvPatchScalarField &fbM = moment.boundaryField()[patchi];
                const fvPatch &patch = fbp.patch();

                forAll(patch, facei){
                    if(fbM[facei] > MSMALL_){
                        fbT[facei] = Foam::log(fbM[facei]);
                        for (int j = 3; j < rmax; j++){
                            if (j != i){
                                fbT[facei] = fbT[facei]*(x - scalar(j))/(scalar(i - j) + VSMALL);
                            }
                        }
                        fbp[facei] += fbT[facei];
                    }
                }
            }
        }
    }
    else /// 負の場合 log(u_0), log(u_1), log(u_2) の値を用いて補間値を求める
    {
        for (int i=0; i < 3; i++)
        {
             volScalarField term
            (
                IOobject
                (
                    "term",
                    mesh_.time().timeName(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                ),
                mesh_,
                dimensionedScalar("term", dimless, 0.0)
            );
            volScalarField moment("moment", (MU_calc(i)));

            forAll(term, cellI){
                if(moment[cellI] > MSMALL_){
                    term[cellI] = Foam::log(moment[cellI]);
                    for (int j=0; j < 3; j++){
                        if (j != i){
                            term[cellI] = term[cellI]*(x - scalar(j))/(scalar(i - j) + VSMALL);
                        }
                    }
                    result[cellI] += term[cellI];
                }
            }

            forAll(result.boundaryField(), patchi){
                fvPatchScalarField &fbp = result.boundaryField()[patchi];
                fvPatchScalarField &fbT = term.boundaryField()[patchi];
                fvPatchScalarField &fbM = moment.boundaryField()[patchi];
                const fvPatch &patch = fbp.patch();

                forAll(patch, facei){
                    if(fbM[facei] > MSMALL_){
                        fbT[facei] = Foam::log(fbM[facei]);
                        for (int j=0; j < 3; j++){
                            if (j != i){
                                fbT[facei] = fbT[facei]*(x - scalar(j))/(scalar(i - j) + VSMALL);
                            }
                        }
                        fbp[facei] += fbT[facei];
                    }
                }
            }
        }
    }

    // 上の補間によって得られた値は log(u_x) である. 
    // これを u_x として返したい. しかも、中に 0 になった値をそのまま 0 として返したい. 
    // volScalarField result2 = volScalarField("result2", (T_/makeTDimless));       // dummy expression
    // result2 *= scalar(0);

     volScalarField result2
    (
        IOobject
        (
            "result2",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result2", dimless, 0.0)
    );

    
    forAll(result2, cellI)
    {
        if (Foam::mag(result[cellI]) > VSMALL)
        {
    /* if (x > 0.0 && x < 5.0){
    if (u2[cellI] > MSMALL_)
        if (result[cellI] > Foam::log(u2[cellI]))
            result[cellI] = Foam::log(u1[cellI]) + ((x - scalar(int(x)))*(Foam::log(u2[cellI]) - Foam::log(u1[cellI])));
    }*/
            result2[cellI] = Foam::exp(result[cellI]);

            if (result2[cellI] < VSMALL)
                result2[cellI] = 0.0;
        }                    
    }

    forAll(result2.boundaryField(), patchi)
    {
        fvPatchScalarField &fbp = result2.boundaryField()[patchi];
        fvPatchScalarField &fbM = result.boundaryField()[patchi];
        // fvPatchScalarField &fbM1 = u1.boundaryField()[patchi];
        // fvPatchScalarField &fbM2 = u2.boundaryField()[patchi];
        const fvPatch &patch = fbp.patch();

        forAll(patch, facei)
        {
            if(Foam::mag(fbM[facei]) > VSMALL)
            {
            /*if (x > 0.0 && x < 5.0){
                if (fbM1[facei] > MSMALL_ && fbM2[facei] > MSMALL_)
                    if(fbM[facei] > Foam::log(fbM2[facei]))
                    fbM[facei] = Foam::log(fbM1[facei]) + ((x - scalar(int(x)))*(Foam::log(fbM2[facei]) - Foam::log(fbM1[facei])));
            }*/
                fbp[facei] = Foam::exp(fbM[facei]);

            if (fbp[facei] < VSMALL)
                fbp[facei] = 0.0;
            }
        }
    }
//Info << "lng1 end" << endl;
    return result2; // but is this being correct ?
}





// // // Knudsen 数が 0.01 より小さい時の衝突項
Foam::volScalarField Foam::sootMoment::Gr001(int r)
{

    volScalarField result
    (
            IOobject
            (
                "result",
                mesh_.time().timeName(),
                    mesh_,
                    IOobject::NO_READ,
                    IOobject::NO_WRITE
            ),
            mesh_,
            dimensionedScalar("result", dimensionSet(0,0,-1,0,0,0,0), 0.0)
    );


    if (r == 0)  /// u_2 まで補間を行う
    {
        // volScalarField tempM
        // (
        //     IOobject
        //     (
        //         "tempM",
        //         mesh_.time().timeName(),
        //             mesh_,
        //             IOobject::NO_READ,
        //             IOobject::NO_WRITE
        //     ),
        //     mesh_,
        //     dimensionedScalar("tempM", dimless, 0.0)
        // );

           result=Kc*(1.0 + (lng1(1.0/3.0, 6)*lng1(-1.0/3.0, 6)) + (Kc1_*(lng1(-1.0/3.0, 6) + (lng1(1.0/3.0, 6)*lng1(-2.0/3.0, 6)))))*Mold_[0]*Mold_[0]/makeTimeDimless;

           return result;

//dimensionedScalar test1 = fvc::domainIntegrate(tempM);
//Info << "Gr001(0) = " << test1.value() << endl;

        // return tempM;
    }
    else if (r==1){
        // volScalarField result = volScalarField("result",(T_/makeTDimless/makeTimeDimless));      // dummy expression
        // result *= scalar(0);
        return result;
    }

    else{
        // volScalarField result = volScalarField("result",(T_/makeTDimless/makeTimeDimless));      // dummy expression
        // result *= scalar(0);

        for (int k = 1; k < r; k++){
            volScalarField term1 = volScalarField(
                "term1",
                ((2.0*MU_calc(k)*MU_calc(r-k)) + (lng1(scalar(k)+(1.0/3.0), 6)*lng1(scalar(r-k)-(1.0/3.0), 6)) + (lng1(scalar(k)-(1.0/3.0), 6)*lng1(scalar(r-k)+(1.0/3.0), 6)))
            );
            volScalarField term2 = volScalarField(
                "term2",
                ((lng1(scalar(k)-(1.0/3.0), 6)*MU_calc(r-k)) + (MU_calc(k)*lng1(scalar(r-k)-(1.0/3.0), 6)) + (lng1(scalar(k)+(1.0/3.0), 6)*lng1(scalar(r-k)-(2.0/3.0), 6)) + (lng1(scalar(k)-(2.0/3.0), 6)*lng1(scalar(r-k)+(1.0/3.0), 6)))
            );
            result += nPr(r, k)*(term1 + (Kc1_*term2))/makeTimeDimless;
        }

//dimensionedScalar test2 = fvc::domainIntegrate(result);
//Info << "Gr001(" << r <<  ") result = " << test2.value() << endl;

        return volScalarField(result*Kc*Mold_[0]*Mold_[0]/2.0);
    }
}

// // 二項補間を行うための補助関数など
Foam::volScalarField Foam::sootMoment::f0xy(int x, int y, int rmax)
{
    volScalarField term1 = volScalarField("term1", (lng1(scalar(x)+(1.0/6.0), rmax)*lng1(scalar(y)-(0.5), rmax)));
    volScalarField term2 = volScalarField("term2", (lng1(scalar(y)+(1.0/6.0), rmax)*lng1(scalar(x)-(0.5), rmax)));
    volScalarField term3 = volScalarField("term3", (lng1(scalar(x)-(1.0/6.0), rmax)*lng1(scalar(y)-(1.0/6.0), rmax)));
    return volScalarField((term1 + term2 + (2.0*term3)));
}

Foam::volScalarField Foam::sootMoment::f1xy(int x, int y, int rmax)
{
    volScalarField term1 = volScalarField("term1", (lng1(scalar(x)+(7.0/6.0), rmax)*lng1(scalar(y)-(1.0/2.0), rmax) + lng1(scalar(x)+(1.0/6.0), rmax)*lng1(scalar(y)+(1.0/2.0), rmax)));
    volScalarField term2 = volScalarField("term2", (lng1(scalar(y)+(7.0/6.0), rmax)*lng1(scalar(x)-(1.0/2.0), rmax) + lng1(scalar(y)+(1.0/6.0), rmax)*lng1(scalar(x)+(1.0/2.0), rmax)));
    volScalarField term3 = volScalarField("term3", ((lng1(scalar(x)+(5.0/6.0), rmax)*lng1(scalar(y)-(1.0/6.0), rmax)) + (lng1(scalar(y)+(5.0/6.0), rmax)*lng1(scalar(x)-(1.0/6.0), rmax))));
    return volScalarField((term1 + term2 + (2.0*term3)));
}

Foam::volScalarField Foam::sootMoment::f2xy(int x, int y, int rmax)
{
    volScalarField term1 =volScalarField("term1", (lng1(scalar(x)+(13.0/6.0), rmax)*lng1(scalar(y)-(1.0/2.0), rmax) + 2.0*lng1(scalar(x)+(7.0/6.0), rmax)*lng1(scalar(y)+(1.0/2.0), rmax) + lng1(scalar(x)+(1.0/6.0), rmax)*lng1(scalar(y)+(3.0/2.0), rmax)));
    volScalarField term2 =volScalarField("term2", (lng1(scalar(y)+(13.0/6.0), rmax)*lng1(scalar(x)-(1.0/2.0), rmax) + 2.0*lng1(scalar(y)+(7.0/6.0), rmax)*lng1(scalar(x)+(1.0/2.0), rmax) + lng1(scalar(y)+(1.0/6.0), rmax)*lng1(scalar(x)+(3.0/2.0), rmax)));
    volScalarField term3 = volScalarField("term3", ((lng1(scalar(x)+(11.0/6.0), rmax)*lng1(scalar(y)-(1.0/6.0), rmax)) + (2.0*lng1(scalar(x)+(5.0/6.0), rmax)*lng1(scalar(y)+(5.0/6.0), rmax)) + (lng1(scalar(x)-(1.0/6.0), rmax)*lng1(scalar(y)+(11.0/6.0), rmax))));
    return volScalarField((term1 + term2 + (2.0*term3)));
}

Foam::volScalarField Foam::sootMoment::f3xy(int x, int y, int rmax)
{
    volScalarField term1 = volScalarField("temp1", ( lng1(scalar(x)+(19.0/6.0), rmax)*lng1(scalar(y)-(1.0/2.0), rmax) + 3.0*lng1(scalar(x)+(13.0/6.0), rmax)*lng1(scalar(y)+(1.0/2.0),rmax) + 3.0*lng1(scalar(x)+(7.0/6.0), rmax)*lng1(scalar(y)+(3.0/2.0), rmax) +lng1(scalar(x)+(1.0/6.0), rmax)*lng1(scalar(y)+(5.0/2.0), rmax) ));
    volScalarField term2 = volScalarField("temp2", ( lng1(scalar(y)+(19.0/6.0), rmax)*lng1(scalar(x)-(1.0/2.0), rmax) + 3.0*lng1(scalar(y)+(13.0/6.0), rmax)*lng1(scalar(x)+(1.0/2.0),rmax) + 3.0*lng1(scalar(y)+(7.0/6.0), rmax)*lng1(scalar(x)+(3.0/2.0), rmax) +lng1(scalar(y)+(1.0/6.0), rmax)*lng1(scalar(x)+(5.0/2.0), rmax) ));
    volScalarField term3 = volScalarField("term3", ((lng1(scalar(x)+(17.0/6.0), rmax)*lng1(scalar(y)-(1.0/6.0), rmax)) + (3.0*lng1(scalar(x)+(11.0/6.0), rmax)*lng1(scalar(y)+(5.0/6.0), rmax)) + (3.0*lng1(scalar(x)+(5.0/6.0), rmax)*lng1(scalar(y)+(11.0/6.0), rmax)) +(lng1(scalar(x)-(1.0/6.0), rmax)*lng1(scalar(y)+(17.0/6.0), rmax))));
    return volScalarField((term1 + term2 + (2.0*term3)));
}

// //////// グリード関数 f_l(x,y) のための二項ラグランジュ補間を求める関数 ///////////
// /// 常に l = 1/2 
Foam::volScalarField Foam::sootMoment::lng2(int x, int y, int rmax)
{
       volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimless, 0.0)
    );


    if ((x==0&&y==0)||(x==1&&y==1)||(x==1&&y==2)||(x==2&&y==2)||(x==2&&y==1)){
        volScalarField moment[4] = {f0xy(x,y,rmax), f1xy(x,y,rmax), f2xy(x,y,rmax), f3xy(x,y,rmax)};

        for (int i=0; i < 4; i++)
        {
            // volScalarField termY = volScalarField("termY", (T_/makeTDimless));       // dummy expression
            // termY *= scalar(0);
                
            volScalarField termY
            (
                IOobject
                (
                    "termY",
                    mesh_.time().timeName(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                ),
                mesh_,
                dimensionedScalar("termY", dimless, 0.0)
            );
        
        
            forAll(termY, cellI){
                if(moment[i][cellI] > VSMALL){
                    termY[cellI] = Foam::log(moment[i][cellI]);
                    for (int j=0; j < 4; j++){
                        if (j != i){
                            //termY[cellI] = termY[cellI]*(mapS(1.0/2.0) - mapS(scalar(j)))/(mapS(scalar(i)) - mapS(scalar(j)));
                            termY[cellI] = termY[cellI]*((1.0/2.0) - scalar(j))/(scalar(i) - scalar(j) + VSMALL);
                        }
                    }
                    result[cellI] += termY[cellI];
                }
            }

            forAll(result.boundaryField(), patchi)
            {
                fvPatchScalarField &fbp = result.boundaryField()[patchi];
                fvPatchScalarField &fbM = moment[i].boundaryField()[patchi];
                fvPatchScalarField &fbT = termY.boundaryField()[patchi];
                const fvPatch &patch = fbp.patch();

                forAll(patch, facei)
                {
                    if(fbM[facei] > VSMALL)
                    {
                        fbT[facei] = Foam::log(fbM[facei]);

                        for (int j=0; j < 4; j++)
                        {
                            if (j != i)
                            {
                                //fbT[facei] = fbT[facei]*(mapS(1.0/2.0) - mapS(scalar(j)))/(mapS(scalar(i)) - mapS(scalar(j)));
                                fbT[facei] = fbT[facei]*((1.0/2.0) - scalar(j))/(scalar(i - j) + VSMALL);
                            }
                        }
                        fbp[facei] += fbT[facei];
                    }
                }
            }
        }
    }

    else if ((x==1&&y==3)||(x==2&&y==3)||(x==3&&y==1)||(x==3&&y==2)){
        volScalarField moment[3] = {f0xy(x,y,rmax), f1xy(x,y,rmax), f2xy(x,y,rmax)};

        for (int i=0; i < 3; i++)
        {
        
            volScalarField termY
            (
                IOobject
                (
                    "termY",
                    mesh_.time().timeName(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                ),
                mesh_,
                dimensionedScalar("termY", dimless, 0.0)
            );
        


            forAll(termY, cellI)
            {
                if(moment[i][cellI] > VSMALL)
                {
                    termY[cellI] = Foam::log(moment[i][cellI]);

                    for (int j=0; j < 3; j++)
                    {
                        if (j != i)
                        {
                            termY[cellI] = termY[cellI]*((1.0/2.0) - scalar(j))/(scalar(i - j) + VSMALL);
                        }
                    }
                    result[cellI] += termY[cellI];
                }
            }

            forAll(result.boundaryField(), patchi)
            {
                fvPatchScalarField &fbp = result.boundaryField()[patchi];
                fvPatchScalarField &fbM = moment[i].boundaryField()[patchi];
                fvPatchScalarField &fbT = termY.boundaryField()[patchi];
                const fvPatch &patch = fbp.patch();

                forAll(patch, facei){
                    if(fbM[facei] > VSMALL){
                        fbT[facei] = Foam::log(fbM[facei]);
                        for (int j=0; j < 3; j++){
                            if (j != i){
                                fbT[facei] = fbT[facei]*((1.0/2.0) - scalar(j))/(scalar(i - j) + VSMALL);
                            }
                        }
                        fbp[facei] += fbT[facei];
                    }
                }
            }
        }
    }

    else if ((x==1&&y==4)||(x==4&&y==1)){
        volScalarField moment[2] = {f0xy(x,y,rmax), f1xy(x,y,rmax)};

        for (int i=0; i < 2; i++)
        {
             volScalarField termY
            (
                IOobject
                (
                    "termY",
                    mesh_.time().timeName(),
                        mesh_,
                        IOobject::NO_READ,
                        IOobject::NO_WRITE
                ),
                mesh_,
                dimensionedScalar("termY", dimless, 0.0)
            );
        
            forAll(termY, cellI){
                if(moment[i][cellI] > VSMALL){
                    termY[cellI] = Foam::log(moment[i][cellI]);
                    for (int j=0; j < 2; j++){
                        if (j != i){
                            termY[cellI] = termY[cellI]*((1.0/2.0) - scalar(j))/(scalar(i - j) + VSMALL);
                        }
                    }
                    result[cellI] += termY[cellI];
                }
            }

            forAll(result.boundaryField(), patchi){
                fvPatchScalarField &fbp = result.boundaryField()[patchi];
                fvPatchScalarField &fbM = moment[i].boundaryField()[patchi];
                fvPatchScalarField &fbT = termY.boundaryField()[patchi];
                const fvPatch &patch = fbp.patch();

                forAll(patch, facei){
                    if(fbM[facei] > VSMALL){
                        fbT[facei] = Foam::log(fbM[facei]);
                        for (int j=0; j < 2; j++){
                            if (j != i){
                                fbT[facei] = fbT[facei]*((1.0/2.0) - scalar(j))/(scalar(i - j) + VSMALL);
                            }
                        }
                        fbp[facei] += fbT[facei];
                    }
                }
            }
        }
    }

    // volScalarField result2 = volScalarField("result2", (T_/makeTDimless));       // dummy expression
    // result2 *= scalar(0);

     volScalarField result2
    (
        IOobject
        (
            "result2",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result2", dimless, 0.0)
    );


    forAll(result2, cellI)
    {
        if (mag(result[cellI]) > MSMALL_)
            result2[cellI] = Foam::exp(result[cellI]);

        if (result2[cellI] < VSMALL)
            result2[cellI] = 0.0;
    }

    forAll(result2.boundaryField(), patchi)
    {
        fvPatchScalarField &fbp = result2.boundaryField()[patchi];
        fvPatchScalarField &fbR = result.boundaryField()[patchi];
        const fvPatch &patch = fbp.patch();

        forAll(patch, facei)
        {
            if (mag(fbR[facei]) > MSMALL_)
                fbp[facei] = Foam::exp(fbR[facei]);   

            if (fbp[facei] < VSMALL)
            fbp[facei] = 0.0;
        }
    }

    return result2; // but is this being correct ?
}

Foam::volScalarField Foam::sootMoment::f_1by2_(int x, int y)
{
    switch (x*y) {
        case 1:
            return f_1_1;

        case 2:
            return f_1_2;

        case 3:
            return f_1_3;

        case 4:
            if (x == 2 && y == 2){ return f_2_2; }
            else{ return f_1_4; }

        case 6:
            return f_2_3;
    }
}


// Knudesen 数が 10 より大きい時の衝突項
Foam::volScalarField Foam::sootMoment::Gr10(int r)
{

    volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimensionSet(0,0,-1,0,0,0,0), 0.0)
    );

    
    if (r==0)
    {
        return volScalarField(Kf*lng2(0, 0, 6)*Mold_[0]*Mold_[0]/2.0/makeTimeDimless);
    }
    else if (r==1)
    {
        return result;
    }

    else{
      
        for (int k = 1; k < r; k++)
        {
            result += nPr(r,k)*f_1by2_(k, r-k)/makeTimeDimless;
        }
        return volScalarField(result*Kf*Mold_[0]*Mold_[0]/2.0);
    }
}

// knudsen 数が 0.01 と 10 の間の時の衝突項
Foam::volScalarField Foam::sootMoment::Gr1(int r, Foam::volScalarField term1, Foam::volScalarField term2)
{
      volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimensionSet(0,0,-1,0,0,0,0), 0.0)
    );

  
    if (r==1)
    {
        return result;
    }
    else
    {
        //volScalarField term1 = Gr001(r);

//dimensionedScalar test1 = fvc::domainIntegrate(term1);
//Info << "Gr001(" << r << ") = " << test1.value() << endl;

        //volScalarField term2 = Gr10(r);
        forAll(result, cellI)
        {
            if(mag(term1[cellI]) > MSMALL_ && mag(term2[cellI]) > MSMALL_)
                result[cellI] = (term1[cellI]/(term1[cellI]+term2[cellI] + VSMALL))*term2[cellI];

            if(mag(result[cellI]) < MSMALL_)
                result[cellI] = 0.0;
        }

        forAll(result.boundaryField(), patchi)
        {
            fvPatchScalarField &fbp = result.boundaryField()[patchi];
            fvPatchScalarField &fbT1 = term1.boundaryField()[patchi];
            fvPatchScalarField &fbT2 = term2.boundaryField()[patchi];
            const fvPatch &patch = fbp.patch();

            forAll(patch, facei){
                if(mag(fbT1[facei]) > MSMALL_ && mag(fbT2[facei]) > MSMALL_)
                    fbp[facei] = (fbT1[facei]/(fbT1[facei]+fbT2[facei] + VSMALL))*fbT2[facei];

                if(mag(fbp[facei] < MSMALL_))
                    fbp[facei] = 0.0;
            }
        }
        return result;
    }
}

// // 衝突項の組み合わせた計算
Foam::volScalarField Foam::sootMoment:: Gr_(int r)
{
    // volScalarField tempG("tempG", (T_/makeTDimless/makeTimeDimless));       // dummy expression
    // tempG *= scalar(0);


      volScalarField tempG
    (
        IOobject
        (
            "tempG",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("tempG", dimensionSet(0,0,-1,0,0,0,0), 0.0)
    );


    if (r == 1){
        return tempG;
    }

    volScalarField tempG001 = Gr001(r);
    volScalarField tempG10 = Gr10(r);
    volScalarField tempG1 = Gr1(r, tempG001, tempG10);

    // volScalarField Knu("Knu", (calcKn()));
   // calcKn();
    volScalarField& Knu=Kn_;

    forAll(tempG, cellI)
    {
        if (Knu[cellI] > MSMALL_)
        {
            if (Knu[cellI] <= 0.01)
                tempG[cellI] = tempG001[cellI];

            else if (Knu[cellI] >= 10.0)
                tempG[cellI] = tempG10[cellI];

            else
                tempG[cellI] = tempG1[cellI];
        }
    }

    forAll(tempG.boundaryField(), patchi)
    {
        fvPatchScalarField &fbp = tempG.boundaryField()[patchi];
        fvPatchScalarField &fbK = Knu.boundaryField()[patchi];
        fvPatchScalarField &fbG1 = tempG1.boundaryField()[patchi];
        fvPatchScalarField &fbG10 = tempG10.boundaryField()[patchi];
        fvPatchScalarField &fbG001 = tempG001.boundaryField()[patchi];
        const fvPatch &patch = fbp.patch();

        forAll(patch, facei){
            if (fbK[facei] > MSMALL_){
                if (fbK[facei] <= 0.01)
                    fbp[facei] = fbG001[facei];

                else if (fbK[facei] >= 10.0)
                    fbp[facei] = fbG10[facei];

                else
                    fbp[facei] = fbG1[facei];
            }
        }
    }

    if (r == 0)
    {
        return volScalarField(tempG*(-1.0));
    }
    else
    {
        return volScalarField(tempG);
    }
}

// // C2H2 との反応モーメント
Foam::volScalarField Foam::sootMoment::W_C2H2_r(int r)
{
     volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimensionSet(0,0,-1,0,0,0,0), 0.0)
    );

    if (r == 0)
        return result;

    else
    {
        for (int k = 0; k < r; k++)
        {
            result += nPr(r, k)*lng1(scalar(k)+(2.0/3.0), 6)*Foam::pow(2.0, scalar(r-k))/makeTimeDimless;
        }
        return volScalarField(result*kR3*c_C2H2_*f4*Alpha_*Xsoot*M_PI*Cs_*Cs_*Mold_[0]/makeRhoDimless);
    }
}

// // // O2 との反応モーメント
Foam::volScalarField Foam::sootMoment::W_O2_r(int r)
{
    volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimensionSet(0,0,-1,0,0,0,0), 0.0)
    );

    if (r == 0)
        return result;

    else{
        for (int k = 0; k < r; k++){
            result += nPr(r, k)*lng1(scalar(k)+(2.0/3.0), 6)*Foam::pow(-2.0, scalar(r-k))/makeTimeDimless;
        }
        return volScalarField(result*kR5*c_O2_*Alpha_*Xsoot*M_PI*Cs_*Cs_*Mold_[0]/makeRhoDimless);
    }
}

// // OH との反応モーメント
Foam::volScalarField Foam::sootMoment::W_OH_r(int r)
{
   volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimensionSet(0,0,-1,0,0,0,0), 0.0)
    );

    if (r == 0)
        return result;

    else{
        for (int k = 0; k < r; k++){
            result += nPr(r, k)*lng1(scalar(k)+(2.0/3.0), 6)*Foam::pow(-1.0, scalar(r-k))/makeTimeDimless;
        }
        return volScalarField(result*kR8*(kR8/(kR7+kR8))*c_OH_*Alpha_*Xsoot_h*M_PI*Cs_*Cs_*Mold_[0]/makeRhoDimless);
        //return volScalarField(result*yOH*Na_*c_OH_*M_PI*Cs_*Cs_*Mold_[0]*Foam::sqrt(M_PI*kB_*T_/(makeTDimless*2.0*mOH))/makeRhoDimless);	// Frenklach & Wang (1994)
    }
}

// PAH のモーメント場を定義した
Foam::volScalarField Foam::sootMoment::MPAHr(scalar r)
{
    return volScalarField(Na_*Foam::pow(16.0, scalar(r))*c_PAH_/makeRhoDimless);
}

// PAH との反応モーメント
Foam::volScalarField Foam::sootMoment::W_PAH_r(int r)
{
    volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimensionSet(0,0,-1,0,0,0,0), 0.0)
    );

    if (r == 0)
        return result;

    else{
        for (int k = 0; k < r; k++){
            volScalarField term1("term1", (Ch_*Ch_*MPAHr(scalar(r-k)+0.5)*MU_calc(k)/makeTimeDimless));
            volScalarField term2("term2", (2.0*Ch_*Cs_*MPAHr(scalar(r-k))*lng1(scalar(k)+(1.0/3.0),6)/makeTimeDimless));
            volScalarField term3("term3", (Cs_*Cs_*MPAHr(scalar(r-k)-0.5)*lng1(scalar(k)+(2.0/3.0),6)/makeTimeDimless));
            result += nPr(r, k)*(term1+term2+term3);
        }
        return volScalarField(result*2.2*Foam::sqrt(M_PI*kB_*T_/(makeTDimless*2.0*Mc_))*Mold_[0]);
    }
}

// 組み合わせた表面反応項
Foam::volScalarField Foam::sootMoment::Wr_(int r)
{
    return volScalarField(W_C2H2_r(r) + W_O2_r(r) + W_OH_r(r) + W_PAH_r(r));
}

// Cunningham slip 修正係数
Foam::volScalarField Foam::sootMoment::C5_()
{

    // volScalarField pmtr("pmtr", (T_/makeTDimless));     // dummy expression
    // pmtr *= scalar(0);


    volScalarField pmtr
    (
        IOobject
        (
            "pmtr",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("pmtr", dimless, 0.0)
    );


    // volScalarField Knu("Knu", (Kn()));
    volScalarField& Knu(Kn_);

    forAll(pmtr, cellI)
    {
        if (Knu[cellI] > MSMALL_)
            pmtr[cellI] = 1.0 + (Knu[cellI]*(1.257 + (0.4*Foam::exp(-1.1/Knu[cellI]))));
    }

    forAll(pmtr.boundaryField(), patchi)
    {
        fvPatchScalarField &fbp = pmtr.boundaryField()[patchi];
        fvPatchScalarField &fbK = Knu.boundaryField()[patchi];
        const fvPatch &patch = fbp.patch();

        forAll(patch, facei){
            if (fbK[facei] > MSMALL_)
                fbp[facei] = 1.0 + (fbK[facei]*(1.257 + (0.4*Foam::exp(-1.1/fbK[facei]))));
        }
    }

    return pmtr;
}


void Foam::sootMoment::correct()
{
    Info << "momentClass::correct() start" << endl;

    //無次元化
    dimensionedScalar dim_Density( "dim_Density", dimDensity, 1.0  );

    //値を更新
    //Info<<"alpha"<<endl;
    dimensionedScalar CCC( "CCC", dimTemperature, 8168.0  );

    Alpha_= (tanh ( ( CCC/T_ ) -4.57 )  + 1.0 ) * 0.5;


    //Info<<"alpha"<<endl;

    alphaPAH_ =  (2.2*Foam::sqrt((M_PI*kB_*T_/makeTDimless)/mPAH_)*Foam::pow((2.0*dPAH_),2.0) );

    //Info<<"Kc"<<endl;

    //KcはKc Kc_をKc1_に変更
     Kc   =  2.0 * kB_ * T_ / (makeTDimless * 3.0 * nu_ * rho_)*dim_Density;
     //Info<<"Kc1"<<endl;

     Kc1_ = 2.514 *lamda_*Foam::pow((M_PI*rhoS_/6.0/Mc_), 1.0/3.0)/makeLDimless;
     //Info<<"Kf"<<endl;

     Kf   =  2.2 * Foam::sqrt(6.0*kB_*T_/makeTDimless/rhoS_) * Foam::pow(3.0*Mc_/(4.0 * M_PI * rhoS_), 1.0/6.0);

     //Info<<"alpha3"<<endl;


  //***************// 化学種との反応速度  //**********************//
        // Csoot-H  +  H  =  Csoot*  +  H2
        // A = 2.5(+14) cm3mol-1s-1, n = 0, E = 66.9 kJmol-1
         kR1 = 2.5e8*Foam::exp(-66.9*1000.0/R_/(T_/makeTDimless));

        // Csoot*  + H2  =  Csoot-H  +  H
        // A = 3.9(+12) cm3mol-1s-1, n = 0, E = 39.0 kJmol-1
         kR1_ = 3.9e6*Foam::exp(-39.0*1000.0/R_/(T_/makeTDimless));


        // Csoot*  +  C2H2  =  CsootC2H2*
        // A = 2.0(+12) cm3mol-1s-1, n = 0, E = 16.7 kJmol-1
         kR3 = 2.0e6*Foam::exp(-16.7*1000.0/R_/(T_/makeTDimless));

        // CsootC2H2*  =  Csoot*  +  C2H2
        // A = 5.0(+13) cm3mol-1s-1, n = 0.4, E = 159.0 kJmol-1
         kR3_ = 5.0e7*Foam::exp(-159.0*1000.0/R_/(T_/makeTDimless));

        // Csoot*  +  O2  =  Csoot*  +  2CO
        // A = 2.2(+12) cm3mol-1s-1, n = 0, E = 31.3 kJmol-1
         kR5 = 2.2e6*Foam::exp(-31.3*1000.0/R_/(T_/makeTDimless) );

	
	// Csoot-H  +  OH  =  Csoot*  +  H2O
        // A = 1.0(+10) cm3mol-1s-1, n = 0.734, E = 5.98 kJmol-1
         kR7 = 1.0e4*Foam::pow(T_/makeTDimless, 0.734)*Foam::exp(-5.98*1000.0/R_/(T_/makeTDimless));

	// Csoot*  +  H2O  =  Csoot-H  +  OH
        // A = 3.68(+8) cm3mol-1s-1, n = 1.139, E = 71.55 kJmol-1
         kR7_ =  3.68e2*Foam::pow(T_/makeTDimless, 1.139)*Foam::exp(-71.55*1000.0 / R_ /(T_/makeTDimless) ) ;



       dimensionedScalar densityToMoleConcentration( "densityToMoleConcentration", dimMoles/dimMass, 1.0);
       c_PAH_ = (rho_*1000.0*(Y_[PAHI_])/composition_.W(PAHI_) );  

       c_C2H2_ =  (rho_*1000.0*(Y_[C2H2I_])/composition_.W(C2H2I_));      // C2H2のモル濃度 (kg/m3)  

       c_O2_   =  (rho_*1000.0*(Y_[O2I_])/composition_.W(O2I_));         // O2のモル濃度 (kg/m3)

       c_OH_   = (rho_*1000.0*(Y_[OHI_])/composition_.W(OHI_));              // OHのモル濃度 (kg/m3)

       c_H_    = rho_*1000.0*(Y_[HI_])/composition_.W(HI_);                  // Hのモル濃度 (kg/m3)

       c_H2_   = rho_*1000.0*(Y_[H2I_])/composition_.W(H2I_);              // H2のモル濃度 (kg/m3)
   
       c_H2O_  = rho_*1000.0*(Y_[H2OI_])/composition_.W(H2OI_);              // H2Oのモル濃度 (kg/m3)

    f4 = kR4 / (kR4 + kR3_ + (kR5*c_O2_/makeRhoDimless));

    Xsoot =  Xsoot_h * ((kR1*c_H_) + (kR7*c_OH_)) / ((kR1_*c_H2_) + (kR2*c_H_) + (kR7_*c_H2O_) + (kR3*c_C2H2_*f4));

    mu_0 = normM(0);
    mu_1 = normM(1);
    mu_2 = normM(2);
    mu_3 = normM(3);
    mu_4 = normM(4);
    mu_5 = normM(5);

    f_1_1 = lng2(1, 1, 6);
    f_1_2 = lng2(1, 2, 6);
    f_1_3 = lng2(1, 3, 6);
    f_1_4 = lng2(1, 4, 6);
    f_2_2 = lng2(2, 2, 6);
    f_2_3 = lng2(2, 3, 6);


    calcDaveS();

    calcLamda();

    calcKn();

    sootD_ = DaveS_;
    Knudsen_ = Kn_;
    FVSoot_ = volScalarField(M_PI*Foam::pow(DaveS_ /makeLDimless, 3.0)*1.0e6*Mold_[0]/6.0);

    Info << "momentClass::correct() function end" << endl;  
}




// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::sootMoment::sootMoment
(
    // const PtrList<volScalarField>& Mold,
    const fvMesh& mesh,
    const volScalarField& rho,
    const volScalarField& T,
    const volScalarField& p,
    const labelList& c_species,
    const PtrList<volScalarField>& Y, 
    const basicMultiComponentMixture& composition
)
:
mesh_(mesh),
FVSoot_
(
        IOobject
        (
            	"FVSoot_",
             mesh.time().timeName(),
            	mesh,
            	IOobject::NO_READ,
            	IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("FVSoot_", dimless, 0)
),
DYi_
(
        IOobject
        (
            	"DYi",
            	 mesh.time().timeName(),
            	mesh,
            	IOobject::NO_READ,
            	IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("DYi", dimensionSet(1, -3, -1, 0, 0, 0, 0), 0)
),
sootD_
(
        IOobject
        (
            	"sootD_",
            	 mesh.time().timeName(),
            	mesh,
            	IOobject::NO_READ,
            	IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("sootD_", dimensionSet(0, 1, 0, 0, 0, 0, 0), 0)
),
Knudsen_
(
        IOobject
        (
            	"Knudsen_",
            	 mesh.time().timeName(),
            	mesh,
            	IOobject::NO_READ,
            	IOobject::AUTO_WRITE
        ),
        mesh,
        dimensionedScalar("Knudsen_", dimless, 0)
),
Mold_(6),
R_(8.3144598),                       // 一般気体定数 (J/mol.K) 
rhoS_( 1800.0),                       // すすの密度 (kg/m3)
CPAH_(16.0),                         // ピレン分子の炭素原子数
dPAH_(0.789131e-9),              // ピレん分子の直径 (m)
mPAH_(202.2492*1.6605*(1e-27)),      // ピレン分子の質量 (kg)
Mc_(12.0*1.6605*(1e-27)),           // 炭素原子の質量 (kg)
Da_(1.395*Foam::sqrt(3.0)*(1e-10)),          // ベンゼン環の C-C 結合間距離 (m)
kB_(1.38064852e-23),             // ボルツマン定数
Na_(6.02214076e23),             // アボガドロ定数
nu_(4.0e-06),                         // エンジン内動粘度, 定数と想定 (m2/s)
MSMALL_(1e-75),                   // 本計算で VSMALL の代わりに使う数字
rho_(rho),
T_(T),
p_(p),
Y_(Y),
composition_(composition),
C4_(1.17), // empirical constant
C6_(2.18), // empirical constant
C7_(1.14), // empirical constant
kGas_(0.089),     // エンジン内気体の熱伝導率     // W/(m.K)
kSoot_(6.0),     // すすの熱伝導率        // W/(m.K)
PAHI_(c_species[0]),
C2H2I_(c_species[1]),
O2I_(c_species[2]),
OHI_(c_species[3]),
HI_(c_species[4]),
H2I_(c_species[5]),
H2OI_(c_species[6]),

//         // すす粒子の平均自由跡     // 単位を m に設定する
lamda_(
    IOobject
    (
         "sootMoment::lamda",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("lamda", dimLength, 0.0)
),
alphaPAH_(
    IOobject
    (
         "alphaPAH",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("alphaPAH", dimless, 0.0)
),
Kc(
    IOobject
    (
         "Kc",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("Kc", dimless, 0.0)
),
Kc1_(
    IOobject
    (
         "Kc1_",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("Kc1_", dimless, 0.0)
),
Kf(
    IOobject
    (
         "Kf",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("Kf", dimless, 0.0)
),
kR1(
    IOobject
    (
         "kR1",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("kR1", dimless, 0.0)
),
kR1_(
    IOobject
    (
         "kR1_",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("kR1_", dimless, 0.0)
),
kR2(1e8),
kR3(
    IOobject
    (
         "kR3",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("kR3", dimless, 0.0)
),
kR3_(
    IOobject
    (
         "kR3_",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("kR3_", dimless, 0.0)
),
kR4(5.0e4),
kR5(
    IOobject
    (
         "kR5",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("kR5", dimless, 0.0)
),
kR7(
    IOobject
    (
         "kR7",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("kR7", dimless, 0.0)
),
kR7_(
    IOobject
    (
         "kR7_",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("kR7_", dimless, 0.0)
),
kR8(1.5e7),
Xsoot_h( 2.3*(1.0e19)),   // unit m-2
//    // Ch_ = Da * root(2/3)
Ch_ ( 1.139013*(1.0e-10)),  // unit m
// // すすとPAHの衝突係数, 一定値, または式によって算出
Alpha_(
      IOobject
    (
         "Alpha",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("Alpha", dimless, 0.0)
        ),        //0.35;
//     //     // OH 反応係数
//     //     //scalar yOH = 0.13;
//     //     // OH ラジカル質量 (kg)
//     //     //scalar mOH = 17.008*0.001/Na_;        
Cs_ (Foam::pow(6.0*Mc_/(M_PI*rhoS_), 1.0/3.0)),
f4(
        IOobject
        (
         "f4",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("f4", dimless, 0.0)
    ),
Xsoot(
        IOobject
        (
         "Xsoot",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("Xsoot", dimless, 0.0)
    ),
c_PAH_(
        IOobject
        (
         "c_PAH_",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("c_PAH_", dimDensity, 0.0)
    ),
    // c_C2H2_ ( volScalarField("c_C2H2", (rho_*1000.0*(Y_[C2H2I_])/composition_.W(C2H2I_)))),      // C2H2のモル濃度 (mol/m3)
c_C2H2_(
    IOobject
    (
        "c_C2H2_",
        mesh.time().timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("c_C2H2_", dimDensity, 0.0)
),
c_O2_(
    IOobject
    (
        "c_O2_",
        mesh.time().timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("c_O2_", dimDensity, 0.0)
),
c_OH_(
    IOobject
    (
        "c_OH_",
        mesh.time().timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("c_OH_", dimDensity, 0.0)
),
c_H_(
    IOobject
    (
        "c_H_",
        mesh.time().timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("c_H_", dimDensity, 0.0)
),
c_H2_(
    IOobject
    (
        "c_H2_",
        mesh.time().timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("c_H2_", dimDensity, 0.0)
),
c_H2O_(
    IOobject
    (
        "c_H2O_",
        mesh.time().timeName(),
        mesh,
        IOobject::NO_READ,
        IOobject::NO_WRITE
    ),
    mesh,
    dimensionedScalar("c_H2O_", dimDensity, 0.0)
),
DaveS_
(
        IOobject
        (
         "DaveS_",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("DaveS_", dimLength, 0.0)
),
Kn_
(
        IOobject
        (
         "Kn_",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("Kn_", dimless, 0.0)
),
   
mu_0
(
        IOobject
        (
         "mu_0",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("mu_0", dimless, 0.0)
),

mu_1
(
        IOobject
        (
         "mu_1",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("mu_1", dimless, 0.0)
),  

mu_2
(
        IOobject
        (
         "mu_2",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("mu_2", dimless, 0.0)
),  

mu_3
(
        IOobject
        (
         "mu_3",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("mu_3", dimless, 0.0)
),

mu_4
(
        IOobject
        (
         "mu_4",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("mu_4", dimless, 0.0)
),   

mu_5
(
        IOobject
        (
         "mu_5",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("mu_5", dimless, 0.0)
),

f_1_1
(
        IOobject
        (
         "f_1_1",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("f_1_1", dimless, 0.0)
),

f_1_2
(
        IOobject
        (
         "f_1_2",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("f_1_2", dimless, 0.0)
),

f_1_3
(
        IOobject
        (
         "f_1_3",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("f_1_3", dimless, 0.0)
),

f_1_4
(
        IOobject
        (
         "f_1_4",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("f_1_4", dimless, 0.0)
),

f_2_2
(
        IOobject
        (
         "f_2_2",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("f_2_2", dimless, 0.0)
),

f_2_3
(
        IOobject
        (
         "f_2_3",
          mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimensionedScalar("f_2_3", dimless, 0.0)
)


{
    std::cout<<"Class construction OK"<<std::endl;
   
    // 化学種のモル濃度を定義した場   TODO：書き換え
       //モーメントリスト

    wordList moments_(6);

    moments_[0]="M0";
    moments_[1]="M1";
    moments_[2]="M2";
    moments_[3]="M3";
    moments_[4]="M4";
    moments_[5]="M5";

 //std::cout<<"OK!! "<<std::endl;

 forAll(moments_, i)
    {
        //Info<<"i "<<i<<endl;
        IOobject header
        (
            moments_[i],
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ
        );

         //Info<<"i "<<i<<" read "<<endl;

        // check if field exists and can be read
        //if (header.headerOk() || header.is_HDF5())
 //       if (header.headerOk())
        if (header.headerOk() || header.headerHDF5Ok(moments_[i]))
        {
            Mold_.set
            (
                i,
                new volScalarField
                (
                    IOobject
                    (
                        moments_[i],
                        mesh.time().timeName(),
                        mesh,
                        IOobject::MUST_READ,
                        IOobject::AUTO_WRITE
                    ),
                    mesh
                )
            );
        }
        else //修正必要
        {

             FatalErrorIn
            (
                "sootMoment"
            )
               <<"モーメントの初期条件をチェックしてください"
                << abort(FatalError);

            volScalarField Ydefault
            (
                IOobject
                (
                    "Ydefault",
                    mesh.time().timeName(),
                    mesh,
                    IOobject::MUST_READ,
                    IOobject::NO_WRITE
                ),
                mesh
            );

            Mold_.set
            (
                i,
                new volScalarField
                (
                    IOobject
                    (
                        moments_[i],
                        mesh.time().timeName(),
                        mesh,
                        IOobject::NO_READ,
                        IOobject::AUTO_WRITE
                    ),
                    Ydefault
                )
            );
        }
    }

    // correct();
    Info << "MOMENT CLASS INITIATED" << endl;
}


Foam::sootMoment::~sootMoment()
{

}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //



//tmpクラスにする．　モーメント式の生成項

Foam::volScalarField Foam::sootMoment::totalMoment(const label& r)
 {
    volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimensionSet(0,0,-1,0,0,0,0), 0.0)
    );


    
    volScalarField incepM = Rr_(r);
    volScalarField coagM = Gr_(r);
    volScalarField reacM = Wr_(r);

    forAll(result, cellI){
        if (Mold_[0][cellI] < MSMALL_)
            result[cellI] = incepM[cellI];
        else
            result[cellI] = incepM[cellI] + reacM[cellI] + coagM[cellI];
            //result[cellI] = incepM[cellI] + reacM[cellI];
            //result[cellI] = incepM[cellI] + coagM[cellI];
    }

    forAll(result.boundaryField(), patchi){
        fvPatchScalarField &fbp = result.boundaryField()[patchi];
        const fvPatchScalarField &fbM0 = Mold_[0].boundaryField()[patchi];
        fvPatchScalarField &fbI = incepM.boundaryField()[patchi];
        fvPatchScalarField &fbC = coagM.boundaryField()[patchi];
        fvPatchScalarField &fbR = reacM.boundaryField()[patchi];
        const fvPatch &patch = fbp.patch();

        forAll(patch, facei){
            if (fbM0[facei] < MSMALL_)
                fbp[facei] = fbI[facei];
            else
                fbp[facei] = fbI[facei] + fbC[facei] + fbR[facei];
                // fbp[facei] = fbI[facei] + fbR[facei];
                //fbp[facei] = fbI[facei] + fbC[facei];
        }
    }

    return result;
}


Foam::volScalarField Foam::sootMoment::Ddiffunc()
 {
    // dimensionedScalar corrDim = dimensionedScalar("corrDim", dimensionSet(0,2,-1,0,0,0,0), 1.0);
    // volScalarField result("result", (T_*corrDim/makeTDimless));
    // result *= scalar(0);

    volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimensionSet(0,2,-1,0,0,0,0), 0.0)
    );


    volScalarField slipCorr = volScalarField("slipCorr", (C5_()));
    
    // volScalarField Davg = volScalarField("Davg", (DaveS_()));
    volScalarField& Davg = DaveS_;

    forAll(result, cellI){
        if (Davg[cellI] > MSMALL_)
            result[cellI] = kB_*T_[cellI]*slipCorr[cellI]/(3.0*M_PI*nu_*rho_[cellI]*Davg[cellI]);
    }

    forAll(result.boundaryField(), patchi){
        fvPatchScalarField &fbp = result.boundaryField()[patchi];
        const fvPatchScalarField &fbD = Davg.boundaryField()[patchi];
        const fvPatchScalarField &fbF = slipCorr.boundaryField()[patchi];
        const fvPatchScalarField &fbT = T_.boundaryField()[patchi];
        const fvPatchScalarField &fbR = rho_.boundaryField()[patchi];
        const fvPatch &patch = fbp.patch();

        forAll(patch, facei){
            if (fbD[facei] > MSMALL_)
                fbp[facei] = kB_*fbT[facei]*fbF[facei]/(3.0*M_PI*nu_*fbR[facei]*fbD[facei]);
        }
    }

    return result;
}


// //  // すすの熱泳動速度係数を返す関数
Foam::volScalarField  Foam::sootMoment::Dtherfunc()
{
    // dimensionedScalar corrDim = dimensionedScalar("corrDim", dimensionSet(0,2,-1,-1,0,0,0), 1.0);
    // volScalarField result("result", (T_*corrDim/makeTDimless));
    // result *= scalar(0);

     volScalarField result
    (
        IOobject
        (
            "result",
            mesh_.time().timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("result", dimensionSet(0,2,-1,-1,0,0,0), 0.0)
    );



    volScalarField slipCorr = volScalarField("slipCorr", (C5_()));

    const volScalarField& Davg = DaveS_;

    forAll(result, cellI){
        if (mag(Davg[cellI]) > MSMALL_ && lamda_[cellI] > MSMALL_)
            result[cellI] = (-1.0)*2.0*C4_*slipCorr[cellI]*nu_*((kGas_/kSoot_)+(2.0*C6_*lamda_[cellI]/Davg[cellI]))/((1.0+(6.0*C7_*lamda_[cellI]/Davg[cellI]))*(1.0+(2.0*kGas_/kSoot_)+(4.0*C6_*lamda_[cellI]/Davg[cellI]))*T_[cellI]);
    }

    forAll(result.boundaryField(), patchi)
    {
        fvPatchScalarField &fbp = result.boundaryField()[patchi];
        const fvPatchScalarField &fbD = Davg.boundaryField()[patchi];
        const fvPatchScalarField &fbL = lamda_.boundaryField()[patchi];
        const fvPatchScalarField &fbF = slipCorr.boundaryField()[patchi];
        const fvPatchScalarField &fbT = T_.boundaryField()[patchi];
        const fvPatch &patch = fbp.patch();

        forAll(patch, facei)
        {
            if (fbD[facei] > MSMALL_ && fbL[facei] > MSMALL_)
                fbp[facei] = (-1.0)*2.0*C4_*fbF[facei]*nu_*((kGas_/kSoot_)+(2.0*C6_*fbL[facei]/fbD[facei]))/((1.0+(6.0*C7_*fbL[facei]/fbD[facei]))*(1.0+(2.0*kGas_/kSoot_)+(4.0*C6_*fbL[facei]/fbD[facei]))*fbT[facei]);
        }
    }

    return result;
}


// // エラーを発見するために用いる関数
void Foam::sootMoment::errorTestValue()
{
    //Info << "tanh(2.0) = " << Foam::tanh(2.0) << endl; 
    //dimensionedScalar ttt = fvc::domainIntegrate(Gr_(0));
    //Info << "total Gr = " << ttt.value() <<  endl;
    //dimensionedScalar woh = fvc::domainIntegrate(W_OH_r(1));
    //dimensionedScalar woh = fvc::domainIntegrate(Mold_[1]);
    //Info << "Value of M1" << woh.value() <<  endl;
    //dimensionedScalar tt2 = fvc::domainIntegrate(Rr_(5));
    //Info << "total Rr(5) = " << tt2.value() <<  endl;
    //dimensionedScalar tt1 = fvc::domainIntegrate(Rr_(1));
    //dimensionedScalar tt1 = fvc::domainIntegrate(rho_);
    //Info << "Total Rho = " << tt1.value() <<  endl;
    //dimensionedScalar tt3 = fvc::domainIntegrate(Gr1(0));
    //Info << "total Gr1(0) = " << tt3.value() <<  endl;
    //dimensionedScalar ww1 = fvc::domainIntegrate(W_O2_r(1));
    //dimensionedScalar ww1 = fvc::domainIntegrate(p_);
    //Info << "Total p" << ww1.value() <<  endl;
    //dimensionedScalar total2 = fvc::domainIntegrate(W_C2H2_r(1));
    //Info << "W_C2H2 = " << total2.value() << endl;
    //dimensionedScalar total3 = fvc::domainIntegrate(Rr_(5));
    //Info << "value Rr(5) = " << total3.value() << endl;
    //dimensionedScalar total4 = fvc::domainIntegrate(W_PAH_r(1));
    //Info << "W_PAH = " << total4.value() << endl;
    //dimensionedScalar total5 = fvc::domainIntegrate(Ddiffunc());
    //Info << "Diffusion coeff  = " << total5.value() << endl;
    //dimensionedScalar total6 = fvc::domainIntegrate(Dtherfunc());
    //Info << "Dtherfunc coeff  = " << total6.value() << endl;
    //Info << "constE = " << constE << endl; 
    Info << "New f_1/2_ calculation implemented" << endl;
}

// // フィルド値が負になる時0に変える関数
void Foam::sootMoment::positiveCorrect(volScalarField *someField)
{
    forAll(*someField, celli){
        if((*someField)[celli] < 1e-75){(*someField)[celli] = 0.0;}
    }

    forAll((*someField).boundaryField(), patchi){
        fvPatchScalarField &fbp = (*someField).boundaryField()[patchi];
        const fvPatch &patch = fbp.patch();
        forAll(patch, facei){
            if(fbp[facei] < 1e-75){fbp[facei] = 0.0;}
        }
    }
}


const Foam::volScalarField Foam::sootMoment::DPAH()
{
    return volScalarField(makeRhoDimless*((2.0*Rr_(0)/Na_)+(W_PAH_r(1)/CPAH_/Na_)));
}

const  Foam::volScalarField Foam::sootMoment::DC2H2()
{
    return volScalarField(W_C2H2_r(1)*makeRhoDimless/2.0/Na_);
}

const  Foam::volScalarField Foam::sootMoment::DO2()
{
    return volScalarField((-1.0)*W_O2_r(1)*makeRhoDimless/2.0/Na_);
}

const  Foam::volScalarField Foam::sootMoment::DOH()
{
    //return volScalarField((-1.0)*W_OH_r(1)*makeRhoDimless/2.0/Na_);	// Frenklach & Wang (1994)
    return volScalarField((-1.0)*W_OH_r(1)*makeRhoDimless/Na_);
}

const  Foam::volScalarField Foam::sootMoment::D_H()
{
    return volScalarField((W_OH_r(1) - (W_C2H2_r(1)/2.0))*makeRhoDimless/Na_);
}

const  Foam::volScalarField Foam::sootMoment::D_H2()
{
    return volScalarField((-1.0)*W_C2H2_r(1)*makeRhoDimless/Na_);
}

const  Foam::volScalarField Foam::sootMoment::DH2O()
{
    return volScalarField((kR7/(kR7 + kR7_ + kR8))*W_OH_r(1)*makeRhoDimless/Na_);
}

const  Foam::volScalarField Foam::sootMoment::D_CO()
{
    return volScalarField(makeRhoDimless*((W_O2_r(1)/2.0) + W_OH_r(1))/Na_);
}

/*Foam::volScalarField Foam::sootMoment::sootDave()
{
    return volScalarField(DaveS_);
}*/

// ************************************************************************* //
