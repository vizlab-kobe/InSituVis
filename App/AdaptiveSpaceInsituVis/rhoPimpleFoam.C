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
#include <fenv.h>
#include <kvs/UnstructuredVolumeObject>
#include <KVS.py/Lib/Interpreter.h>
#include <KVS.py/Lib/String.h>
#include <KVS.py/Lib/Module.h>
#include <KVS.py/Lib/Dict.h>
#include <KVS.py/Lib/Callable.h>
#include <KVS.py/Lib/Array.h>
#include <KVS.py/Lib/Float.h>
#include <mpi.h>
#include <kvs/Timer>
#include <math.h>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
int main(int argc, char *argv[])
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

  //!!!!!!!!!!!!テスト実行の際の注意!!!!!!!!!!!!!//
  //change the parametersのみを変更してください。//
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!//

  /*******************change the parameters*******/

  /*全ての場合に次の変数を設定してください*/
  const int mode = 0; //実行モード,0:本番実行,1:プレシミュレーション実行

  /*mode = 1のときは次の変数のみを設定すれば良く、他の変数は設定不要で実行可能*/
  const int presimulationinterval = 100;//プレシミュレーション時のKL情報量計算間隔ΔT,mode=0の時はこの変数は使用しない
  
  /*mode = 0のときは、まず次の3つの変数を設定する*/
  const int step = 2; //細かく可視化するときの可視化間隔(Δt')/ボリュームデータ出力間隔
  int estimatethreshold = 0;//閾値を推定するかどうか,0:推定しない(手打ちで決める),1:presimulationにより推定する。
  const int multicamera = 0; //マルチカメラ設定,0:1台のカメラ,1:複数のカメラ。
 
  /*粒子レンダリングを使用する時は次の変数を設定してください*/
  const size_t repetitions = 1;//粒子レンダリング時のアンサンブル数、それ以外の場合は特に使われない変数
  
  /*estimatethreshold=0のときは次の変数を設定してください*/
  float threshold = 0; //閾値、推定する場合は後で更新されるので設定不要
  int vis_skip = 1;  //粗さの粒度R、プレシミュレーションで閾値を推定する場合は自動で決定されるので設定不要
  int noutperKL = 5; //各KL情報量計算までに行うボリュームデータ出力回数、プレシミュレーションで閾値を設定する場合は自動で決定されるので設定不要
  //noutperKL×step=ΔTとなる。
  
  /*multicamera=1のときは次の３つを設定してください。*/
  const int ncamerax = 3; //マルチカメラ時,x方向にカメラの台数を2^ncameras+1台設置する。ただし,0の時は1台設置する。
  const int ncameray = 3; //マルチカメラ時,y方向にカメラの台数を2^ncameras+1台設置する。ただし,0の時は1台設置する。
  const int ncameraz = 3; //マルチカメラ時,z方向にカメラの台数を2^ncameras+1台設置する。ただし,0の時は1台設置する。
  
  /*******************no change******************/
  int volume_size = noutperKL; //ボリュームデータの保存サイズL
  //timer
  kvs::Timer sim_timer;//シミュレーション時間用タイマー
  kvs::Timer vis_timer;//可視化時間用タイマー
  kvs::Timer distribution_timer;//KL情報量計算時間用タイマー*消去もあり
  float sim_time = 0.0;//シミュレーション時間
  float vis_time = 0.0;//可視化時間
  float distribution_time = 0.0;//KL情報量計算時間*消去もあり
  int timeswitch;//計測時間の種類の判定,1:シミュレーションの時間,2:可視化の時間,3:KL情報量計算の時間,自動で設定されるので特にユーザーが設定する必要はない
  //KL情報量計算結果を書き出すファイル
  std::string filename1 = "entropy.csv";
  std::string filename2 = "presimulation/preentropy.csv";
  std::ofstream writing_file;
  if(mode == 0){
    writing_file.open(filename1, std::ios::app);
  }else{
    writing_file.open(filename2, std::ios::app);
  } 
  //シミュレーション時間結果を書き出すファイル
  std::string timefilename = "simulationtime.csv";
  std::ofstream time_writing_file;
  time_writing_file.open(timefilename, std::ios::app);
  //可視化時間結果を書き出すファイル
  std::string timefilename2 = "visualizationtime.csv";
  std::ofstream time_writing_file2;
  time_writing_file2.open(timefilename2, std::ios::app);
  //KL情報量計算時間結果を書き出すファイル*消去もあり
  std::string timefilename3 = "distributiontime.csv";
  std::ofstream time_writing_file3;
  time_writing_file3.open(timefilename3, std::ios::app);
  //プレシミュレーション計算時間結果を書き出すファイル
  std::string timefilename4 = "presimulation/presimulationinterval.txt";
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
  const int nsim=205;//20000ステップでKL情報量計算回数が200回の時に設定している。念のため余分に５個だけ用意している。
  float entropies[nsim];//今までのKL情報量計算結果を保存しておく配列、リスタートの場合については読み込みなど別途プログラムが必要。
  int saveKL = 0;//全体でKL情報量計算結果が何回分保存されているかの変数
  for (int aa = 0; aa < nsim; aa++){//KL情報量保存用配列の初期化
    entropies[aa]=0;
  }
  std::vector<std::vector<float>> data_set;//可視化するボリュームデータの保存用変数
  kvs::ValueArray<float> old_hist; //現在の分布情報
  kvs::ValueArray<float> new_hist; //前の時刻の分布情報
  //volume_sizeの回数分のデータがあるかどうかの判定のためのカウンタ
  int count = 0;
  //multi camera判定用の変数、配列および初期化
  int multicamerax = pow(2,ncamerax)+1;
  int multicameray = pow(2,ncameray)+1;
  int multicameraz = pow(2,ncameraz)+1;
  int judgemulticamera[multicamerax][multicameray][multicameraz];
  for (int i = 0; i < multicamerax; i++){
    for (int j = 0; j < multicameray; j++){
      for (int k = 0; k < multicameraz; k++){
	judgemulticamera[i][j][k] = 1;
      }
    }
  }    
  /****pythonを呼び出すための設定***********/
  kvs::python::Interpreter interpreter;
  const char* script_file_name = "distribution";
  kvs::python::Module module( script_file_name );
  kvs::python::Dict dict = module.dict();
  const char* func_name = "main";
  kvs::python::Callable func( dict.find( func_name ) );
  /****python end***************************/

  /************閾値を手動で決める場合の設定*************************/
  if(estimatethreshold == 0){
#include "estimateThreshold.H"
  }
  /************閾値を手動で決める場合の設定終了*********************/

  /************プレシミュレーションによる初期閾値の推定**************/
  if ( estimatethreshold == 2){
#include "estimateThreshold.H"    
  }
  /************プレシミュレーションによる初期閾値の推定終了**********/

  /*******************no change end**************/




  //ここから処理スタート//
  
    Info<< "\nStarting time loop\n" << endl;
    while (runTime.run())
      {

	/*simulation code*/
	if(mode == 0){
	  sim_timer.start();
	}
#include "simulation.H"//シミュレーションを行うためのコード
	if(mode == 0){
	  sim_timer.stop();
	  sim_time = sim_timer.sec();
	  timeswitch=1;
#include "calculatetime.H"//時間計測の処理、粒子レンダリングを使う場合、粒子レンダリング内の個々の時間計測についてはPBVR_u.cppで行なっている。	  
	}
	/*simulation code end*/
	{
	  fenv_t curr_excepts; //ゼロ割り(floating exception)を無視するための関数,osmesa内でゼロ割が起こっているため
	  feholdexcept( &curr_excepts );
	  
	  /*visualization code*/
#include "vis.H"  //in-situ可視化を行うためのコード
	  /*visualization code end*/
	}
	
        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
      }
    if(mode == 1){//プレシミュレーション結果の出力
       time_writing_file4 << presimulationinterval << std::endl;
       Info<< "cd presimulation\n" << endl;
       Info<< "cat usage.txt\n" << endl;
       Info<< "./usepresimulationI or ./usepresimulationC \n" << endl;
    }
    Info<< "End\n" << endl;
    
    return 0;
}


// ************************************************************************* //
