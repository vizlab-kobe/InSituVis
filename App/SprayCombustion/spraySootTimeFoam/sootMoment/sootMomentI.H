// モーメントの計算に必要なクラス及びモーメントの修正を行う関数を定義したヘッダーファイル
// sprayEngineKIVAFoam.C において #include "wallFvPatch.H" の次に、main 関数が始まる前に入れておく
// #include "sootMoment.H"

// // * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //
inline Foam::scalar Foam::sootMoment::ftl(int n)
{
    // if(n == 0)
    // {
    //     return 1.0;
    // }
    // else
    // {
    //     return scalar(n)*ftl(n-1);
    // }

    int sum = 1;
    for (int i = 1; i <= n; ++i)
    {
        sum *= i;
    }
    return sum;
}

// // 順列 nPr を求める関数
inline Foam::scalar Foam::sootMoment:: nPr(int n, int r)
{
    return ftl(n)/(ftl(n-r)*ftl(r));
}


// ラグランジュ補間を行うための写像関数
inline Foam::scalar Foam::sootMoment:: mapS(scalar x)
{
    // 補間はa=0からb=5の間に行うと仮定
    // x_new = (a+b)/2 + (a-b)/2*cos(pi*(x-a)/(b-a))
    return 2.5*(1 - Foam::cos(degToRad(36 * x)));
}




// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //




// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


inline Foam::PtrList<Foam::volScalarField>& Foam::sootMoment::Moments()
{
    return Mold_;
}


inline Foam::volScalarField& Foam::sootMoment::FVSoot()
{
    return FVSoot_;
}

inline Foam::volScalarField& Foam::sootMoment::DYi()
{
    return DYi_;
}

inline Foam::volScalarField& Foam::sootMoment::sootDAve()
{
    return sootD_;
    //return volScalarField(DaveS_);
}

inline Foam::volScalarField& Foam::sootMoment::Knudsen()
{
    return Knudsen_;
    //return Kn_;
}

/*Foam::volScalarField& Foam::sootMoment::dummySD()
{
    return sootD_;
}*/

// ************************************************************************* //

