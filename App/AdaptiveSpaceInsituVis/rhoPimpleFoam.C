/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    rhoPimpleFoam

Description
    Transient solver for laminar or turbulent flow of compressible fluids
    for HVAC and similar applications.

    Uses the flexible PIMPLE (PISO-SIMPLE) solution for time-resolved and
    pseudo-transient simulations.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "psiThermo.H"
#include "turbulenceModel.H"
#include "bound.H"
#include "pimpleControl.H"
#include "fvIOoptionList.H"

#include <vector>
#include <sstream>
#include <string>
#include <fenv.h>
#include <kvs/UnstructuredVolumeObject>
#include <kvs/python/Interpreter>
#include <kvs/python/String>
#include <kvs/python/Module>
#include <kvs/python/Dict>
#include <kvs/python/Callable>
#include <kvs/python/Array>
#include <kvs/python/Float>
#include <mpi.h>
#include <math.h>
#include <kvs/ColorImage> // modified
#include <kvs/RGBColor> // modified

#include <kvs/Indent>


#include "StampTimer.h"

/*
int EstimateThreshold( kvs::ValueArray<int>& presim_judgevis )
{
//    double sorted_entropies[20000/config::presim_number];  //segmentation fault が出た場合、この配列のサイズを変えること。デフォルトでは、シミュレーションを20000step行った場合について作っている。
//    double ordered_entropies[20000/config::presim_number];
    kvs::ValueArray<double> sorted_entropies( presim_judgevis.size() );
    kvs::ValueArray<double> ordered_entropies( presim_judgevis.size() );

    sorted_entropies.fill( 0.0 );
    ordered_entropies.fill( 0.0 );
    presim_judgevis.fill( 0 );
//    for ( int shokika = 0; shokika < 20000/config::presim_number; shokika++ )
//    {
//        sorted_entropies[shokika] = 0;
//        ordered_entropies[shokika] = 0;
//        presim_judgevis[shokika] = 0;
//    }

//        #include "estimateThreshold.H"
//    int ret;
//    int prei = 0;

     //int interval = 1;
     //double threshold = 0.0;
    //プレシミュレーションの時のデータの読み込みをして、thresholdを決定する。
    Info << "Reading the data of presimulation...\n" << endl;

    config::noutperKL = config::presim_number / config::step;

    FILE *fp;
    if ( config::presim_number == 10 )       { fp = fopen("./presimulation/preentropy10.csv","r"); }
    else if ( config::presim_number == 20 )  { fp = fopen("./presimulation/preentropy20.csv","r"); }
    else if ( config::presim_number == 30 )  { fp = fopen("./presimulation/preentropy30.csv","r"); }
    else if ( config::presim_number == 40 )  { fp = fopen("./presimulation/preentropy40.csv","r"); }
    else if ( config::presim_number == 50 )  { fp = fopen("./presimulation/preentropy50.csv","r"); }
    else if ( config::presim_number == 60 )  { fp = fopen("./presimulation/preentropy60.csv","r"); }
    else if ( config::presim_number == 70 )  { fp = fopen("./presimulation/preentropy70.csv","r"); }
    else if ( config::presim_number == 80 )  { fp = fopen("./presimulation/preentropy80.csv","r"); }
    else if ( config::presim_number == 90 )  { fp = fopen("./presimulation/preentropy90.csv","r"); }
    else if ( config::presim_number == 100 ) { fp = fopen("./presimulation/preentropy100.csv","r"); }
    else if ( config::presim_number == 110 ) { fp = fopen("./presimulation/preentropy110.csv","r"); }
    else if ( config::presim_number == 120 ) { fp = fopen("./presimulation/preentropy120.csv","r"); }
    else if ( config::presim_number == 130 ) { fp = fopen("./presimulation/preentropy130.csv","r"); }
    else if ( config::presim_number == 140 ) { fp = fopen("./presimulation/preentropy140.csv","r"); }
    else if ( config::presim_number == 150 ) { fp = fopen("./presimulation/preentropy150.csv","r"); }
    else if ( config::presim_number == 160 ) { fp = fopen("./presimulation/preentropy160.csv","r"); }
    else if ( config::presim_number == 170 ) { fp = fopen("./presimulation/preentropy170.csv","r"); }
    else if ( config::presim_number == 180 ) { fp = fopen("./presimulation/preentropy180.csv","r"); }
    else if ( config::presim_number == 190 ) { fp = fopen("./presimulation/preentropy190.csv","r"); }
    else if ( config::presim_number == 200 ) { fp = fopen("./presimulation/preentropy200.csv","r"); }
    else
    {
        Info << "presim_number in config.H is wrong!!!!!\nPlease check the config.H" << endl;
        return 1;
    }

    if ( config::I_R < 0.0 ) config::I_R = 0;
    if ( config::I_R > 1.0 ) config::I_R = 1.0;
    if ( config::C_R < 0.0 ) config::C_R = 0;
    if ( config::C_R > 1.0 ) config::C_R = 1.0;
    if ( ( config::I_R * config::C_R == 0 ) && ( config::I_R + config::C_R == 0 ) )
    {
        Info << "I_R or C_R in config.H is wrong!!!!!\nPlease check the config.H" << endl;
        return 1;
    }

    double inputentropy = 0;
    int ret = 0;
    int prei = 0;
    while ( ret = fscanf( fp, "%lf,", &inputentropy ) != EOF )
    {
        ordered_entropies[prei]=inputentropy;
        for ( int ethr = prei; ethr >=0; ethr-- )
        {
            if ( sorted_entropies[ethr] > inputentropy )
            {
                sorted_entropies[ethr+1]=sorted_entropies[ethr];
            }
            else
            {
                sorted_entropies[ethr+1]=inputentropy;
            }
        }
        prei++;
        if ( prei == 20000 / config::presim_number ) //エラー回避
            break;
    }

    int rating;
    if ( config::I_R == 0 )
    {
        //コスト削減割合から閾値を決定
        if ( config::vis_skip > config::noutperKL )
            rating=prei*(config::C_R);
        else
            rating=prei*(config::vis_skip/(config::vis_skip-1))*config::C_R;
    }
    else
    {
        //重要区間率から閾値を決定
        rating=prei*(1-config::I_R);
    }

    if ( rating == 0 ) rating++; //0のときは0を取るようにする
    config::threshold = sorted_entropies[rating-1];

    for ( int judgeKL = 0; judgeKL < prei; judgeKL++ )
    {
        if ( ordered_entropies[judgeKL] > config::threshold )
            presim_judgevis[judgeKL]=2;
        else if ( judgeKL == 0 )//エラー回避
            presim_judgevis[judgeKL]=1;
        else if ( ordered_entropies[judgeKL-1] > config::threshold )
            presim_judgevis[judgeKL]=3;
        else
            presim_judgevis[judgeKL]=1;
    }

    Info<< "Finishing Reading the data of presimulation" << endl;

    return 0;
}
*/

#include "config.H" //テスト実行時に変更するパラメータが書かれているソースコード

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
int main( int argc, char** argv )
{
    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"

    pimpleControl pimple(mesh);

    #include "createFields.H"
    #include "createFvOptions.H"
    #include "initContinuityErrs.H"

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Foam::messageStream::level = 0; // Disable Foam::Info

    /**********************mpi***************/
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    /**********************mpi end***********/

//    #include "config.H" //テスト実行時に変更するパラメータが書かれているソースコード

    /*******************no change******************/
    int presimulationcount = 0;
    float cameraposx = 0;//default:0
    float cameraposy = 0;//default:0
    float cameraposz = 5;//default:5

    //KL情報量計算結果を書き出すファイル
    std::ofstream writing_file;
    std::ofstream writing_file1[20];
//    if ( mode == 0 )
    if ( config::mode == 0 )
    {
        std::string filename1 = "entropy.csv";
        writing_file.open(filename1, std::ios::app);
    }
    else
    {
        std::string filename2 = "presimulation/preentropy10.csv";
        std::string filename3 = "presimulation/preentropy20.csv";
        std::string filename4 = "presimulation/preentropy30.csv";
        std::string filename5 = "presimulation/preentropy40.csv";
        std::string filename6 = "presimulation/preentropy50.csv";
        std::string filename7 = "presimulation/preentropy60.csv";
        std::string filename8 = "presimulation/preentropy70.csv";
        std::string filename9 = "presimulation/preentropy80.csv";
        std::string filename10 = "presimulation/preentropy90.csv";
        std::string filename11 = "presimulation/preentropy100.csv";
        std::string filename12 = "presimulation/preentropy110.csv";
        std::string filename13 = "presimulation/preentropy120.csv";
        std::string filename14 = "presimulation/preentropy130.csv";
        std::string filename15 = "presimulation/preentropy140.csv";
        std::string filename16 = "presimulation/preentropy150.csv";
        std::string filename17 = "presimulation/preentropy160.csv";
        std::string filename18 = "presimulation/preentropy170.csv";
        std::string filename19 = "presimulation/preentropy180.csv";
        std::string filename20 = "presimulation/preentropy190.csv";
        std::string filename21 = "presimulation/preentropy200.csv";
        writing_file1[0].open(filename2, std::ios::app);
        writing_file1[1].open(filename3, std::ios::app);
        writing_file1[2].open(filename4, std::ios::app);
        writing_file1[3].open(filename5, std::ios::app);
        writing_file1[4].open(filename6, std::ios::app);
        writing_file1[5].open(filename7, std::ios::app);
        writing_file1[6].open(filename8, std::ios::app);
        writing_file1[7].open(filename9, std::ios::app);
        writing_file1[8].open(filename10, std::ios::app);
        writing_file1[9].open(filename11, std::ios::app);
        writing_file1[10].open(filename12, std::ios::app);
        writing_file1[11].open(filename13, std::ios::app);
        writing_file1[12].open(filename14, std::ios::app);
        writing_file1[13].open(filename15, std::ios::app);
        writing_file1[14].open(filename16, std::ios::app);
        writing_file1[15].open(filename17, std::ios::app);
        writing_file1[16].open(filename18, std::ios::app);
        writing_file1[17].open(filename19, std::ios::app);
        writing_file1[18].open(filename20, std::ios::app);
        writing_file1[19].open(filename21, std::ios::app);
    }

    //KL情報量計算に必要な変数
    float pre_entropy  = 0.0;//前の時刻のKL情報量
    float entropy = 0.0;//現時刻のKL情報量
    int KLpattern = 2;//KL情報量計算区間のregionパターン(A,B,C)を保存する変数
    std::vector<std::vector<float>> data_set;//可視化するボリュームデータの保存用変数
    kvs::ValueArray<float> old_hist; //現在の分布情報
    kvs::ValueArray<float> new_hist; //前の時刻の分布情報
    kvs::ValueArray<float> pre_hist[20];//プレシミュレーション用分布情報
    int minpresimulation = 0;

    //volume_sizeの回数分のデータがあるかどうかの判定のためのカウンタ
    int count = 0;
    //presimulationの結果のデータのうちどれを読み込むかのためのカウンタ
    int countpreKL = 0;

    //multi camera判定用の変数、配列および初期化
    float image1_x;
    float image1_y;
    float image1_z;
    float image2_x;
    float image2_y;
    float image2_z;
    const int nmulticamera_high = 1 + 2^config::depthmulti_high; //マルチカメラ時、KL情報量が閾値以上の場合、x,y,z方向にそれぞれ何台カメラを設置するか
    const int nmulticamera_low = 1 + 2^config::depthmulti_low;//閾値未満の場合、同様
    float camerainterval, maxcamerapos, mincamerapos;
    int nmulticamera;
    int judgevis;
    int initialmulti;
    float mthreefourminx;
    float mthreefourminy;
    float mthreefourminz;
    float mthreefourmaxx;
    float mthreefourmaxy;
    float mthreefourmaxz;
    if ( config::multicamera == 1 )
    {
        //マルチカメラ時の初期設定
        camerainterval = config::multicamera_posABS * 2/(nmulticamera_high-1);
    }

    const int presimulationinterval = 10;//プレシミュレーション時の最小KL情報量計算間隔ΔT,mode=0の時はこの変数は使用しない

    /****pythonを呼び出すための設定***********/
    std::string module_name( "distribution" );
    kvs::python::Interpreter interpreter;
    kvs::python::Module module( module_name );
    kvs::python::Callable func( module.dict().find( "main" ) );
    /****python end***************************/

    /************プレシミュレーションによる初期閾値の推定**************/
//    double inputentropy = 0;
//    double sorted_entropies[20000/config::presim_number];  //segmentation fault が出た場合、この配列のサイズを変えること。デフォルトでは、シミュレーションを20000step行った場合について作っている。
//    double ordered_entropies[20000/config::presim_number];
    int presim_judgevis[20000/config::presim_number];
    if ( config::estimatethreshold == 1 )
    {
        double sorted_entropies[20000/config::presim_number];  //segmentation fault が出た場合、この配列のサイズを変えること。デフォルトでは、シミュレーションを20000step行った場合について作っている。
        double ordered_entropies[20000/config::presim_number];

        for( int shokika = 0; shokika < 20000/config::presim_number; shokika++ )
        {
            sorted_entropies[shokika] = 0;
            ordered_entropies[shokika] = 0;
            presim_judgevis[shokika] = 0;
        }

//        #include "estimateThreshold.H"
        int ret;
        int prei = 0;
        /*
           int interval = 1;
           double threshold = 0.0;*/
        //プレシミュレーションの時のデータの読み込みをして、thresholdを決定する。
        Info << "Reading the data of presimulation...\n" << endl;

        config::noutperKL = config::presim_number / config::step;

        FILE *fp;
        if ( config::presim_number == 10 )       { fp = fopen("./presimulation/preentropy10.csv","r"); }
        else if ( config::presim_number == 20 )  { fp = fopen("./presimulation/preentropy20.csv","r"); }
        else if ( config::presim_number == 30 )  { fp = fopen("./presimulation/preentropy30.csv","r"); }
        else if ( config::presim_number == 40 )  { fp = fopen("./presimulation/preentropy40.csv","r"); }
        else if ( config::presim_number == 50 )  { fp = fopen("./presimulation/preentropy50.csv","r"); }
        else if ( config::presim_number == 60 )  { fp = fopen("./presimulation/preentropy60.csv","r"); }
        else if ( config::presim_number == 70 )  { fp = fopen("./presimulation/preentropy70.csv","r"); }
        else if ( config::presim_number == 80 )  { fp = fopen("./presimulation/preentropy80.csv","r"); }
        else if ( config::presim_number == 90 )  { fp = fopen("./presimulation/preentropy90.csv","r"); }
        else if ( config::presim_number == 100 ) { fp = fopen("./presimulation/preentropy100.csv","r"); }
        else if ( config::presim_number == 110 ) { fp = fopen("./presimulation/preentropy110.csv","r"); }
        else if ( config::presim_number == 120 ) { fp = fopen("./presimulation/preentropy120.csv","r"); }
        else if ( config::presim_number == 130 ) { fp = fopen("./presimulation/preentropy130.csv","r"); }
        else if ( config::presim_number == 140 ) { fp = fopen("./presimulation/preentropy140.csv","r"); }
        else if ( config::presim_number == 150 ) { fp = fopen("./presimulation/preentropy150.csv","r"); }
        else if ( config::presim_number == 160 ) { fp = fopen("./presimulation/preentropy160.csv","r"); }
        else if ( config::presim_number == 170 ) { fp = fopen("./presimulation/preentropy170.csv","r"); }
        else if ( config::presim_number == 180 ) { fp = fopen("./presimulation/preentropy180.csv","r"); }
        else if ( config::presim_number == 190 ) { fp = fopen("./presimulation/preentropy190.csv","r"); }
        else if ( config::presim_number == 200 ) { fp = fopen("./presimulation/preentropy200.csv","r"); }
        else
        {
            Info << "presim_number in config.H is wrong!!!!!\nPlease check the config.H" << endl;
            return 1;
        }

        if ( config::I_R < 0.0 ) config::I_R = 0;
        if ( config::I_R > 1.0 ) config::I_R = 1.0;
        if ( config::C_R < 0.0 ) config::C_R = 0;
        if ( config::C_R > 1.0 ) config::C_R = 1.0;
        if ( ( config::I_R * config::C_R == 0 ) && ( config::I_R + config::C_R == 0 ) )
        {
            Info << "I_R or C_R in config.H is wrong!!!!!\nPlease check the config.H" << endl;
            return 1;
        }

        double inputentropy = 0;
        while ( ret = fscanf( fp, "%lf,", &inputentropy ) != EOF )
        {
            ordered_entropies[prei]=inputentropy;
            for ( int ethr = prei; ethr >=0; ethr-- )
            {
                if ( sorted_entropies[ethr] > inputentropy )
                {
                    sorted_entropies[ethr+1]=sorted_entropies[ethr];
                }
                else
                {
                    sorted_entropies[ethr+1]=inputentropy;
                }
            }
            prei++;
            if ( prei == 20000 / config::presim_number ) //エラー回避
                break;
        }

        int rating;
        if ( config::I_R == 0 )
        {
            //コスト削減割合から閾値を決定
            if ( config::vis_skip > config::noutperKL )
                rating=prei*(config::C_R);
            else
                rating=prei*(config::vis_skip/(config::vis_skip-1))*config::C_R;
        }
        else
        {
            //重要区間率から閾値を決定
            rating=prei*(1-config::I_R);
        }

        if ( rating == 0 ) rating++; //0のときは0を取るようにする
        config::threshold = sorted_entropies[rating-1];

        for ( int judgeKL = 0; judgeKL < prei; judgeKL++ )
        {
            if ( ordered_entropies[judgeKL] > config::threshold )
                presim_judgevis[judgeKL]=2;
            else if ( judgeKL == 0 )//エラー回避
                presim_judgevis[judgeKL]=1;
            else if ( ordered_entropies[judgeKL-1] > config::threshold )
                presim_judgevis[judgeKL]=3;
            else
                presim_judgevis[judgeKL]=1;
        }

        Info<< "Finishing Reading the data of presimulation" << endl;
    }
    /************プレシミュレーションによる初期閾値の推定終了**********/

    /*******************no change end**************/




    //ここから処理スタート//

    const kvs::Indent indent( 4 );
    const auto start_time = runTime.startTime().value();
    const auto start_time_index = runTime.startTimeIndex();
    const auto end_time = runTime.endTime().value();
    const auto end_time_index = static_cast<int>( end_time / runTime.deltaT().value() );
    if ( my_rank == 0 ) std::cout << std::endl;
    if ( my_rank == 0 ) std::cout << "STARTING TIME LOOP" << std::endl;
    if ( my_rank == 0 ) std::cout << indent << "Start time and index: " << start_time << ", " << start_time_index << std::endl;
    if ( my_rank == 0 ) std::cout << indent << "End time and index: " << end_time << ", " << end_time_index << std::endl;
    if ( my_rank == 0 ) std::cout << std::endl;

    // Timers for processing time measurement.
    local::StampTimer sim_times; // simulation processing times
    local::StampTimer vis_times; // visualization processing times
    local::StampTimer kld_times; // KL divergence calculation processing times

    Info<< "\nStarting time loop\n" << endl;
    while ( runTime.run() )
    {
        /* simulation code */
        if ( config::mode == 0 ) { sim_times.start(); }
        #include "simulation.H" // シミュレーションを行うためのコード
        if ( config::mode == 0 )
        {
            sim_times.stamp();
            Info << "Simulation Solver time: " << sim_times.last() << endl;
            // 時間計測の処理、粒子レンダリングを使う場合、粒子レンダリング内の個々の時間計測についてはPBVR_u.cppで行なっている。
        }
        /* simulation code end */

        const auto current_time = runTime.timeName();
        const auto current_time_index = runTime.timeIndex();
        if ( my_rank == 0 ) std::cout << "LOOP[" << current_time_index << "/" << end_time_index << "]: " << std::endl;
        if ( my_rank == 0 ) std::cout << indent << "T: " << current_time << std::endl;
        if ( my_rank == 0 ) std::cout << indent << "End T: " << end_time << std::endl;
        if ( my_rank == 0 ) std::cout << indent << "Delta T: " << runTime.deltaT().value() << std::endl << std::endl;

        {
            // ゼロ割り(floating exception)を無視するための関数,osmesa内でゼロ割が起こっているため
            fenv_t curr_excepts;
            feholdexcept( &curr_excepts );


            /* visualization code */
            //in-situ可視化を行うためのコード
//            #include "preparevis.H"
            {
                const int now_time = static_cast<int>(atof(runTime.timeName().c_str())/runTime.deltaT().value());
                const int start_time = static_cast<int>( runTime.startTimeIndex() );

                //可視化初期処理
                if ( now_time -1 == start_time )
                {
                    std::vector<float> first_uValues;
                    forAll( mesh.cellPoints(), i )
                    {
                        first_uValues.push_back( static_cast<float>( mag(U[i]) ) );
                        old_hist = kvs::ValueArray<float>( first_uValues );
                    }
                    std::vector<float>().swap(first_uValues);
                }

                //ボリュームデータ出力判定
                if ( ( config::mode == 0 && now_time % config::step == 0 )||( config::mode == 1 && now_time % presimulationinterval == 0))
                {
                    #include <PBVR_u.h>
                    std::vector<float> pValues; //圧力
                    std::vector<float> pointCoords; //頂点の座標値
                    std::vector<float> cellCoords; //要素中心の座標値
                    std::vector<int> cellPoints; //中心から頂点への接続情報
                    std::vector<float> uValues; //速度の絶対値
                    //std::vector<float> uVector; //速度のベクトル値

                    //ボリュームデータ出力処理
                    #include "outputvolumedata.H"
                    //KL情報量計算の実行判定および処理
                    count++;
                    if ( ( count >= config::noutperKL ) || ( config::mode == 1 ) )
                    {
                        new_hist = kvs::ValueArray<float>( uValues );
                        if ( old_hist.size() != 0)
                        {
                            if ( ( config::mode == 0 ) && ( config::estimatethreshold == 0 ) )
                            {
                                kld_times.start();
//                                #include "calculateKL.H"
                                {
                                    kvs::python::Tuple args( 2 );
                                    args.set( 0, kvs::python::Array( new_hist ) );
                                    args.set( 1, kvs::python::Array( old_hist ) );

                                    entropy = kvs::python::Float( func.call( args ) );
                                    MPI_Allreduce( &entropy, &entropy, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );

                                    if ( entropy >= config::threshold )
                                    {
                                        //パターンB(細かい可視化)
                                        KLpattern = 2;
                                    }
                                    else if ( pre_entropy >= config::threshold )
                                    {
                                        //パターンC(細かい可視化から粗い可視化)
                                        KLpattern = 3;
                                    }
                                    else
                                    {
                                        //パターンA(粗い可視化)
                                        KLpattern = 1;
                                    }

                                    pre_entropy = entropy;
                                    if ( my_rank == 0 )
                                    {
                                        std::cout << "entropy : " << entropy << std::endl;
                                        writing_file << "," << entropy << std::endl;
                                    }
                                    old_hist = new_hist;
                                }
                                kld_times.stamp();
                                Info << "Distribution time : " << kld_times.last() << endl;

                                count=0;
                            }
                            else if ( ( config::mode == 0 ) && ( config::estimatethreshold == 1 ) )
                            {
                                KLpattern==presim_judgevis[countpreKL];
                                countpreKL++;
                            }
                            else
                            {
                                //config::mode == 1
//                                #include "presimulationcalculateKL.H"
                                {
                                    new_hist = kvs::ValueArray<float>( uValues );

                                    kvs::python::Tuple args( 2 );
                                    for ( int numpre = 0; numpre < 20; numpre++ )
                                    {
                                        if ( ( minpresimulation > numpre ) && ( now_time % (numpre+10) == 0 ) )
                                        {
                                            args.set( 0, kvs::python::Array( new_hist ) );
                                            args.set( 1, kvs::python::Array( pre_hist[numpre] ) );
                                            entropy = kvs::python::Float( func.call( args ) );
                                            MPI_Allreduce( &entropy, &entropy, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
                                            if ( my_rank == 0 )
                                            {
                                                std::cout << "entropy : " << entropy << std::endl;
                                                writing_file1[numpre] << "," << entropy << std::endl;
                                            }
                                        }
                                    }

                                    minpresimulation++;
                                    pre_hist[presimulationcount]=new_hist;
                                    presimulationcount = (presimulationcount + 1)%20;
                                }
                            }
                        }

                        //可視化の実行判定および処理
                        if ( config::mode == 0 )
                        {
                            vis_times.start();
                            if( KLpattern==2 ) //パターンB(細かい可視化)
                            {
                                for( size_t i = 0; i < data_set.size(); i++)
                                {
                                    if ( config::multicamera == 1 )
                                    {
                                        #include "multicamera.H"
                                    }
                                    else
                                    {
                                        #include "vis.H"
                                    }
                                }
                            }
                            else
                            {
                                if ( KLpattern==3 ) //パターンC(細かい可視化から粗い可視化)
                                {
                                    KLpattern = 2;
                                    for( size_t i = 0; i < int(data_set.size()/2); i++)
                                    {
                                        if ( config::multicamera == 1 )
                                        {
                                            #include "multicamera.H"
                                        }
                                        else
                                        {
                                            #include "vis.H"
                                        }
                                    }
                                    KLpattern = 1;
                                    for ( size_t i = int(data_set.size()/2) + config::vis_skip; i < data_set.size(); i += config::vis_skip )
                                    {
                                        if ( config::multicamera == 1 )
                                        {
                                            #include "multicamera.H"
                                        }
                                        else
                                        {
                                            #include "vis.H"
                                        }
                                    }
                                }
                                else //パターンA(粗い可視化)
                                {
                                    for( size_t i = data_set.size()*config::vis_skip-1; i < data_set.size(); i += config::vis_skip)
                                    {
                                        if ( config::multicamera == 1 )
                                        {
                                            #include "multicamera.H"
                                        }
                                        else
                                        {
                                            #include "vis.H"
                                        }
                                    }
                                }
                            }

                            data_set.clear();

                            vis_times.stamp();
                            Info << "Conversion vis time : " << vis_times.last() << endl;
                        }
                        else
                        {
                            //(config::mode==1)
                            data_set.clear();
                        }
                        std::vector<float>().swap(uValues);
                        std::vector<float>().swap(pValues);
                        std::vector<float>().swap(pointCoords);
                        std::vector<float>().swap(cellCoords);
                        std::vector<int>().swap(cellPoints);
                    }
                }
            }
            /* visualization code end */
        }
        Info << "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
             << "  ClockTime = " << runTime.elapsedClockTime() << " s"
             << nl << endl;
    }

    if ( config::mode == 1 )
    {
        std::string timefilename4 = "presimulation/presimulationinterval.csv";
        std::ofstream time_writing_file4;
        time_writing_file4.open(timefilename4, std::ios::app);

        //プレシミュレーション結果の出力
        time_writing_file4 << presimulationinterval << std::endl;
        Info<< "Rewrite the config.H (at least, mode = 0, estimatethreshold = 1) and do your simulation with visualization.\n" << endl;
    }

    // Output the processing times to the files at the rank0 node.
    sim_times.allreduce( MPI_MAX, MPI_COMM_WORLD );
    vis_times.allreduce( MPI_MAX, MPI_COMM_WORLD );
    kld_times.allreduce( MPI_MAX, MPI_COMM_WORLD );
    if ( my_rank == 0 )
    {
        sim_times.write( "simulationtime.csv" );
        vis_times.write( "visualizationtime.csv" );
        kld_times.write( "distributiontime.csv" );
    }

    Info << "End\n" << endl;

    return 0;
}

// ************************************************************************* //
