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



#include "StampTimer.h"


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

    /**********************mpi***************/
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    /**********************mpi end***********/

    #include "config.H" //テスト実行時に変更するパラメータが書かれているソースコード

    /*******************no change******************/
    int presimulationcount = 0;
    float cameraposx = 0;//default:0
    float cameraposy = 0;//default:0
    float cameraposz = 5;//default:5

    //timer
//  kvs::Timer sim_timer;
    kvs::Timer vis_timer;//可視化時間用タイマー
    kvs::Timer distribution_timer;//KL情報量計算時間用タイマー*消去もあり
//    float sim_time = 0.0;//シミュレーション時間
    float vis_time = 0.0;//可視化時間
    float distribution_time = 0.0;//KL情報量計算時間*消去もあり
    int timeswitch;//計測時間の種類の判定,1:シミュレーションの時間,2:可視化の時間,3:KL情報量計算の時間,自動で設定されるので特にユーザーが設定する必要はない

    //KL情報量計算結果を書き出すファイル
    std::string filename1 = "entropy.csv";
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
    std::ofstream writing_file;
    std::ofstream writing_file1[20];
    if ( mode == 0 )
    {
        writing_file.open(filename1, std::ios::app);
    }
    else
    {
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

    //シミュレーション時間結果を書き出すファイル
//    std::string timefilename = "simulationtime.csv";
//    std::ofstream time_writing_file;
//    time_writing_file.open(timefilename, std::ios::app);

    //可視化時間結果を書き出すファイル
    std::string timefilename2 = "visualizationtime.csv";
    std::ofstream time_writing_file2;
    time_writing_file2.open(timefilename2, std::ios::app);

    //KL情報量計算時間結果を書き出すファイル*消去もあり
    std::string timefilename3 = "distributiontime.csv";
    std::ofstream time_writing_file3;
    time_writing_file3.open(timefilename3, std::ios::app);

    //プレシミュレーション計算時間結果を書き出すファイル
    std::string timefilename4 = "presimulation/presimulationinterval.csv";
    std::ofstream time_writing_file4;
    time_writing_file4.open(timefilename4, std::ios::app);

    //閾値推定結果を書き出すファイル
    std::string timefilename5 = "threshold.csv";
    std::ofstream time_writing_file5;
    time_writing_file5.open(timefilename5, std::ios::app);

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
    const int nmulticamera_high = 1 + 2^depthmulti_high; //マルチカメラ時、KL情報量が閾値以上の場合、x,y,z方向にそれぞれ何台カメラを設置するか
    const int nmulticamera_low = 1 + 2^depthmulti_low;//閾値未満の場合、同様
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
    if ( multicamera == 1 )
    {
        //マルチカメラ時の初期設定
        camerainterval = multicamera_posABS*2/(nmulticamera_high-1);
    }

    const int presimulationinterval = 10;//プレシミュレーション時の最小KL情報量計算間隔ΔT,mode=0の時はこの変数は使用しない

    /****pythonを呼び出すための設定***********/
    std::string module_name( "distribution" );
    kvs::python::Interpreter interpreter;
    kvs::python::Module module( module_name );
    kvs::python::Callable func( module.dict().find( "main" ) );
    /****python end***************************/

    /************プレシミュレーションによる初期閾値の推定**************/
    double inputentropy = 0;
    double sorted_entropies[20000/presim_number];  //segmentation fault が出た場合、この配列のサイズを変えること。デフォルトでは、シミュレーションを20000step行った場合について作っている。
    double ordered_entropies[20000/presim_number];
    int presim_judgevis[20000/presim_number];
    if ( estimatethreshold == 1 )
    {
        for( int shokika = 0; shokika < 20000/presim_number; shokika++ )
        {
            sorted_entropies[shokika] = 0;
            ordered_entropies[shokika] = 0;
            presim_judgevis[shokika] = 0;
        }

        #include "estimateThreshold.H"
    }
    /************プレシミュレーションによる初期閾値の推定終了**********/

    /*******************no change end**************/


    //ここから処理スタート//

//    kvs::Timer sim_timer; // timer for simulation
//    float sim_time = 0.0;//シミュレーション時間
//    std::vector<float> sim_times;

    local::StampTimer sim_times;

    Info<< "\nStarting time loop\n" << endl;
    while ( runTime.run() )
    {
        /* simulation code */
//        if ( mode == 0 ) { sim_timer.start(); sim_times.start(); }
        if ( mode == 0 ) { sim_times.start(); }

        #include "simulation.H" // シミュレーションを行うためのコード
        if ( mode == 0 )
        {
//            sim_timer.stop();
//            sim_time = sim_timer.sec();
            sim_times.stamp();
            Info << "Simulation Solver time: " << sim_times.last() << endl;

            timeswitch = 1;

            // 時間計測の処理、粒子レンダリングを使う場合、粒子レンダリング内の個々の時間計測についてはPBVR_u.cppで行なっている。
//            #include "calculatetime.H"
//            {
//                if ( timeswitch == 1 )
//                {
//                    sim_times.push_back( sim_time );
//                    MPI_Allreduce( &sim_time, &sim_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
//                    if (my_rank == 0 )
//                    {
//                        time_writing_file << sim_time << std::endl;
//                    }
//                    Info << "Simulation Solver time : " << sim_time << endl;
//                    Info << "Simulation Solver time stamp: " << sim_times.last() << endl;
//                }
/*
                else if ( timeswitch == 2 )
                {
                    MPI_Allreduce( &vis_time, &vis_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
                    if ( my_rank == 0 )
                    {
                        time_writing_file2 << vis_time << std::endl;
                    }
                    Info << "Conversion vis time : " << vis_time << endl;
                }

                else if ( timeswitch == 3 )
                {
                    MPI_Allreduce( &distribution_time, &distribution_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
                    if ( my_rank == 0 )
                    {
                        time_writing_file3 << distribution_time << std::endl;
                    }
                    Info << "Distribution time : " << distribution_time << endl;
                }
*/
//            }
        }
        /* simulation code end */

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
                if ( ( mode == 0 && now_time % step == 0 )||(mode == 1 && now_time % presimulationinterval == 0))
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
                    if ( ( count >= noutperKL ) || ( mode == 1 ) )
                    {
                        new_hist = kvs::ValueArray<float>( uValues );
                        if ( old_hist.size() != 0)
                        {
                            if ( ( mode == 0 ) && ( estimatethreshold == 0 ) )
                            {
                                distribution_timer.start();
                                #include "calculateKL.H"
                                distribution_timer.stop();
                                distribution_time = distribution_timer.sec();
                                timeswitch=3;
//                                #include "calculatetime.H"
                                {
                                    /*
                                    if ( timeswitch == 1 )
                                    {
//                                        sim_times.push_back( sim_time );
                                        MPI_Allreduce( &sim_time, &sim_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
                                        if (my_rank == 0 )
                                        {
                                            time_writing_file << sim_time << std::endl;
                                        }
                                        Info << "Simulation Solver time : " << sim_time << endl;
                                    }

                                    else if ( timeswitch == 2 )
                                    {
                                        MPI_Allreduce( &vis_time, &vis_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
                                        if ( my_rank == 0 )
                                        {
                                            time_writing_file2 << vis_time << std::endl;
                                        }
                                        Info << "Conversion vis time : " << vis_time << endl;
                                    }
                                    */
//                                    else if ( timeswitch == 3 )
                                    {
                                        MPI_Allreduce( &distribution_time, &distribution_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
                                        if ( my_rank == 0 )
                                        {
                                            time_writing_file3 << distribution_time << std::endl;
                                        }
                                        Info << "Distribution time : " << distribution_time << endl;
                                    }
                                }

                                count=0;
                            }
                            else if ( ( mode == 0 ) && ( estimatethreshold == 1 ) )
                            {
                                KLpattern==presim_judgevis[countpreKL];
                                countpreKL++;
                            }
                            else
                            {
                                //mode == 1
                                #include "presimulationcalculateKL.H"
                            }
                        }

                        //可視化の実行判定および処理
                        if ( mode == 0 )
                        {
                            vis_timer.start();
                            if( KLpattern==2 ) //パターンB(細かい可視化)
                            {
                                for( size_t i = 0; i < data_set.size(); i++)
                                {
                                    if ( multicamera == 1 )
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
                                        if ( multicamera == 1 )
                                        {
                                            #include "multicamera.H"
                                        }
                                        else
                                        {
                                            #include "vis.H"
                                        }
                                    }
                                    KLpattern = 1;
                                    for ( size_t i = int(data_set.size()/2) + vis_skip; i < data_set.size(); i += vis_skip )
                                    {
                                        if ( multicamera == 1 )
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
                                    for( size_t i = data_set.size()*vis_skip-1; i < data_set.size(); i += vis_skip)
                                    {
                                        if ( multicamera == 1 )
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
                            vis_timer.stop();
                            vis_time = vis_timer.sec();
                            timeswitch = 2;
//                            #include "calculatetime.H"
                            {
                                /*
                                if ( timeswitch == 1 )
                                {
//                                    sim_times.push_back( sim_time );
                                    MPI_Allreduce( &sim_time, &sim_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
                                    if (my_rank == 0 )
                                    {
                                        time_writing_file << sim_time << std::endl;
                                    }
                                    Info << "Simulation Solver time : " << sim_time << endl;
                                }
                                */
//                                else if ( timeswitch == 2 )
                                {
                                    MPI_Allreduce( &vis_time, &vis_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
                                    if ( my_rank == 0 )
                                    {
                                        time_writing_file2 << vis_time << std::endl;
                                    }
                                    Info << "Conversion vis time : " << vis_time << endl;
                                }
/*
                                else if ( timeswitch == 3 )
                                {
                                    MPI_Allreduce( &distribution_time, &distribution_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
                                    if ( my_rank == 0 )
                                    {
                                        time_writing_file3 << distribution_time << std::endl;
                                    }
                                    Info << "Distribution time : " << distribution_time << endl;
                                }
*/
                            }
                        }
                        else
                        {
                            //(mode==1)
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

    if ( mode == 1 )
    {
        //プレシミュレーション結果の出力
        time_writing_file4 << presimulationinterval << std::endl;
        Info<< "Rewrite the config.H (at least, mode = 0, estimatethreshold = 1) and do your simulation with visualization.\n" << endl;
    }

    {
        sim_times.allreduce( MPI_MAX, MPI_COMM_WORLD );
        if ( my_rank == 0 ) { sim_times.write( "simulationtime.csv" ); }
        /*
        std::vector<float> temp( sim_times.size() );
        MPI_Allreduce( &sim_times[0], &temp[0], sim_times.size(), MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
        temp.swap( sim_times );

        if ( my_rank == 0 )
        {
            std::ofstream ofs( "sim.csv", std::ios::out | std::ios::app );
            for ( auto& t : sim_times ) { ofs << t << std::endl; }
        }
        */
    }

    Info << "End\n" << endl;

    return 0;
}

// ************************************************************************* //
