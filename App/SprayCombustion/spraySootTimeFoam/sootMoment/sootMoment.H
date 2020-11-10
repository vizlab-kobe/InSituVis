// モーメントの計算に必要なクラス及びモーメントの修正を行う関数を定義したヘッダーファイル
// sprayEngineKIVAFoam.C において #include "wallFvPatch.H" の次に、main 関数が始まる前に入れておく

#ifndef sootMoment_H
#define sootMoment_H

#include "fvCFD.H"
#include "SLGThermo.H"
#include "PtrList.H"
#include "volFields.H"
// #include "volScalarField.H"


#include<cmath>


namespace Foam
{

// クラスの定義
class sootMoment
{
    protected:

      PtrList<volScalarField> Mold_;                 // 前計算ステップのモーメント

        // すすの体積分率
        volScalarField FVSoot_;

        // 化学種の消費速度
        volScalarField DYi_;

        // すす粒子の平均直径
        volScalarField sootD_;

        // すす粒子に対するKnudsen数(気体の平均自由行程と粒子直径の比)
        volScalarField Knudsen_;

    private:

        const fvMesh& mesh_;

      


        ////////////////////////////// 変数などの定義 /////////////////////////
        scalar  R_ ;                       // 一般気体定数 (J/mol.K) 
        scalar  rhoS_ ;                       // すすの密度 (kg/m3)
        scalar  CPAH_ ;                         // ピレン分子の炭素原子数
        scalar  dPAH_ ;              // ピレん分子の直径 (m)
        scalar  mPAH_ ;      // ピレン分子の質量 (kg)
        scalar  Mc_ ;           // 炭素原子の質量 (kg)
        scalar  Da_ ;          // ベンゼン環の C-C 結合間距離 (m)
        scalar  kB_ ;             // ボルツマン定数
        scalar  Na_;             // アボガドロ定数
        scalar  nu_;                         // エンジン内動粘度, 定数と想定 (m2/s)
        scalar  MSMALL_ ;                   // 本計算で VSMALL の代わりに使う数字


        // PAH 衝突項のパラメータなど
        scalar  A_ ;               // g/mol.A3
        scalar  B_ ;             // g/mol
        scalar  C_ ;
        scalar  gunma_;        // まだ検討中

        // モーメント拡散係数及びすすの熱泳動速度の計算に必要なパラメータ
        const scalar C4_; // empirical constant
        const scalar C6_; // empirical constant
        const scalar C7_; // empirical constant
        const scalar kGas_;     // エンジン内気体の熱伝導率     // W/(m.K)
        const scalar kSoot_;     // すすの熱伝導率        // W/(m.K)



        // // クラスオブジェクトが作られたとき取得する変数
 

        const volScalarField& rho_;                        // 密度場
        const volScalarField& T_;                          // 温度場
        const volScalarField& p_;                              // 圧力場


        //化学種のラベル
        label PAHI_;
        label C2H2I_;
        label O2I_;
        label OHI_;
        label HI_;
        label H2I_;
        label H2OI_;    

        // 衝突係数
        scalar constE_;
        // PAH 衝突頻度     // dimensionless
        

        const PtrList<volScalarField>& Y_;                    // 化学種質量分率場
        const basicMultiComponentMixture& composition_;       // 化学種の構成についての情報



        //モーメントの無次元化

        // void calcMomentNormalize(const label& imoment);
        
        volScalarField c_PAH_ ;           // Pyreneのモル濃度 (mol/m3)
        volScalarField c_C2H2_;    // C2H2のモル濃度 (mol/m3)
        volScalarField c_O2_ ;        // O2のモル濃度 (mol/m3)
        volScalarField c_OH_ ;             // OHのモル濃度 (mol/m3)
        volScalarField c_H_  ;               // Hのモル濃度 (mol/m3)
        volScalarField c_H2_ ;            // H2のモル濃度 (mol/m3)
        volScalarField c_H2O_;            // H2Oのモル濃度 (mol/m3)


        // 無次元化するために用意した変数；これで割る　書き換え
        const dimensionedScalar makeTDimless = dimensionedScalar("makeTDimless", dimTemperature, 1.0);
        const dimensionedScalar makeLDimless = dimensionedScalar("makeLDimless", dimLength, 1.0);
        const dimensionedScalar makeTimeDimless = dimensionedScalar("makeTDimless", dimTime, 1.0);
        const dimensionedScalar makeRhoDimless = dimensionedScalar("makeRhoDimless", dimensionSet(1,-3,0,0,0,0,0), 1.0);     

        volScalarField alphaPAH_;

        // すす粒子の平均自由距離     // 単位を m に設定する
        // dimensionedScalar corrLamdaDim ;
        volScalarField lamda_;

        // 計算に必要な項       // 無単位
        volScalarField Kc ;
        volScalarField Kc1_ ;
        volScalarField Kf ;

        //***************// 化学種との反応速度  //**********************//  書き換え
        // Csoot-H  +  H  =  Csoot*  +  H2
        // A = 2.5(+14) cm3mol-1s-1, n = 0, E = 66.9 kJmol-1
        volScalarField kR1;

        // Csoot*  + H2  =  Csoot-H  +  H
        // A = 3.9(+12) cm3mol-1s-1, n = 0, E = 39.0 kJmol-1
        volScalarField kR1_;

	// Csoot*  +  H  =  Csoot-H
        // A = 1.0(+14) cm3mol-1s-1, n = 0, E = 0 kJmol-1
        const scalar kR2 ;

        // Csoot*  +  C2H2  =  CsootC2H2*
        // A = 2.0(+12) cm3mol-1s-1, n = 0, E = 16.7 kJmol-1
        volScalarField kR3;

        // CsootC2H2*  =  Csoot*  +  C2H2
        // A = 5.0(+13) cm3mol-1s-1, n = 0.4, E = 159.0 kJmol-1
        volScalarField kR3_;

        // CsootC2H2*  =  Csoot-H  + H
        // A = 5.0(+10) cm3mol-1s-1, n = 0, E = 0 kJmol-1
        const scalar kR4;

        // Csoot*  +  O2  =  Csoot*  +  2CO
        // A = 2.2(+12) cm3mol-1s-1, n = 0, E = 31.3 kJmol-1
        volScalarField kR5;

        // CsootC2H2*  +  O2  =  Csoot*  +  2CHO
        // kR6 is same as kR5
	
        // Csoot-H  +  OH  =  Csoot*  +  H2O
        // A = 1.0(+10) cm3mol-1s-1, n = 0.734, E = 5.98 kJmol-1
        volScalarField kR7 ;

	// Csoot*  +  H2O  =  Csoot-H  +  OH
        // A = 3.68(+8) cm3mol-1s-1, n = 1.139, E = 71.55 kJmol-1
        volScalarField kR7_ ;

        // Csoot-H  +  OH  =  Csoot*  +  CO  +  H
       const scalar kR8 ;	//1.1e7  //1.2e7   //2.2e7   //1.0e8

    //     // 表面成長項の計算に必要な変数など
    //     // すす粒子面の活性化した場所の密度
        const scalar Xsoot_h ;   // unit m-2
    //     // Ch_ = Da * root(2/3)
        const scalar Ch_ ;  // unit m
    //     // すすとPAHの衝突係数, 一定値, または式によって算出
        volScalarField Alpha_ ;        //0.35;
    // //     // OH 反応係数
    // //     //scalar yOH = 0.13;
    // //     // OH ラジカル質量 (kg)
    // //     //scalar mOH = 17.008*0.001/Na_;        

    scalar Cs_ ;      

    volScalarField f4;

    volScalarField Xsoot ;
     

    volScalarField mu_0;
    volScalarField mu_1;
    volScalarField mu_2;
    volScalarField mu_3;
    volScalarField mu_4;
    volScalarField mu_5;

   
    //volScalarField f_0_0;
    volScalarField f_1_1;
    volScalarField f_1_2;
    volScalarField f_1_3;
    volScalarField f_1_4;
    volScalarField f_2_2;
    volScalarField f_2_3; 
    

    //     /////////////////////////// 関数などの定義 ////////////////////////////
    //     // 正数 n の階乗を求める関数
        inline scalar ftl(int n);        

    //     // 順列 nPr を求める関数
        inline scalar nPr(int n, int r);
        

    //     // ラグランジュ補間を行うための写像関数
        inline scalar mapS(scalar x);
      

        // // 正規化したモーメント u_i = M[i]/M[0] を求める関数
        volScalarField normM(int i);
       
        // // モーメント生成項
        volScalarField Rr_(int r);
          

        // // 平均モル質量の算出           // = 1 / sum(mass fraction / molecular mass)
        volScalarField Mmix();
        

        // // すす粒子の平均直径計算       // すすがない場において 0 に設定        // 単位を m に設定
        volScalarField  DaveS_;
     
        // // Knudsen 数
        volScalarField Kn_;
    
        volScalarField MU_calc(int r);
        
        volScalarField f_1by2_(int x, int y);  
 
        // ///// 一項ラグランジュ補間を求める関数 ///////////////
        volScalarField lng1(scalar x, int rmax);

        // // Knudsen 数が 0.01 より小さい時の衝突項
	volScalarField Gr001(int r);
       
        // // 二項補間を行うための補助関数など
        volScalarField f0xy(int x, int y, int rmax);
        
        volScalarField f1xy(int x, int y, int rmax);

        volScalarField f2xy(int x, int y, int rmax);

        volScalarField f3xy(int x, int y, int rmax);

        // //////// グリード関数 f_l(x,y) のための二項ラグランジュ補間を求める関数 ///////////
        // /// 常に l = 1/2 
        volScalarField lng2(int x, int y, int rmax);

        // // Knudesen 数が 10 より大きい時の衝突項
        volScalarField Gr10(int r);

        // // knudsen 数が 0.01 と 10 の間の時の衝突項
        volScalarField Gr1(int r, volScalarField term1, volScalarField term2);

        // // 衝突項の組み合わせた計算
        volScalarField Gr_(int r);

        // // C2H2 との反応モーメント
        volScalarField W_C2H2_r(int r);

        // // O2 との反応モーメント
        volScalarField W_O2_r(int r);

        // OH との反応モーメント
        volScalarField W_OH_r(int r);

        // PAH のモーメント場を定義した
        volScalarField MPAHr(scalar r);

        // PAH との反応モーメント
        volScalarField W_PAH_r(int r);

        // 組み合わせた表面反応項
        volScalarField Wr_(int r);

        // Cunningham slip 修正係数
        volScalarField C5_();

    public:

    //- Runtime type information
    // TypeName("sootMoment");

        // constructor      // 初期化のときに引数などの入力順番に要注意
        sootMoment
        (
            // const PtrList<volScalarField>& Mold,
            const fvMesh& mesh,
            const volScalarField& rho,
            const volScalarField& T,
            const volScalarField& p,
            const labelList& c_species,
            const PtrList<volScalarField>& Y, 
            const basicMultiComponentMixture& composition
        );

        // sootMoment
        // (
        //     // const PtrList<volScalarField>& Mold,
        //     const fvMesh& mesh,
        //     const volScalarField& rho,
        //     const volScalarField& T,
        //     const volScalarField& p,
        //     const PtrList<volScalarField>& Y, 
        //     const basicMultiComponentMixture& composition
        // );

        ~sootMoment();
     

        // 全てのモーメント項を組み合わして返す関数
        volScalarField totalMoment(const label& r);

       
        // モーメント拡散係数を返す関数
        volScalarField Ddiffunc();
       

        // すすの熱泳動速度係数を返す関数
        volScalarField Dtherfunc();
      

        // 化学種の濃度変化速度を返す関数など
         const volScalarField DPAH();
       
         const volScalarField DC2H2();

         const volScalarField DO2();

         const volScalarField DOH();

         const volScalarField D_H();

	 const volScalarField D_H2();

         const volScalarField DH2O();

	 const volScalarField D_CO();
    
        // エラーを発見するために用いる関数
        void errorTestValue();



        void correct();


        void calcDaveS();


        void calcLamda();

        void calcKn();

    //access

        volScalarField M0()
        {
            return Mold_[0];
        }
        volScalarField M1()
        {
            return Mold_[1];
        }
         volScalarField M2()
        {
            return Mold_[2];
        }
         volScalarField M3()
        {
            return Mold_[3];
        }
         volScalarField M4()
        {
            return Mold_[4];
        }
         volScalarField M5()
        {
            return Mold_[5];
        }
        /* volScalarField sootD()
        {
	    std::cout << "Hello World \n";
            return volScalarField(1.0*DaveS_);
        }*/

         // // クラスオブジェクトが作られたとき取得する変数
        inline PtrList<volScalarField>& Moments();
        inline volScalarField& FVSoot();
        inline volScalarField& DYi();
        inline volScalarField& sootDAve();
        inline volScalarField& Knudsen();
    
        // volScalarField& dummySD();

        // フィルド値が負になる時0に変える関数
        void positiveCorrect(volScalarField *someField);

};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "sootMomentI.H"

#endif

// ************************************************************************* //
