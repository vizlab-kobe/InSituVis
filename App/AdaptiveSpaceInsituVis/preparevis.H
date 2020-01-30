const int now_time = static_cast<int>(atof(runTime.timeName().c_str())/runTime.deltaT().value());
const int start_time = static_cast<int>( runTime.startTimeIndex() );
//可視化初期処理
if( now_time -1 == start_time ){
  std::vector<float> first_pValues;
  forAll( mesh.cellPoints(), i )
    {
      first_pValues.push_back( static_cast<float>( mag(U[i]) ) );
      old_hist = kvs::ValueArray<float>( first_pValues );
    }
  std::vector<float>().swap(first_pValues);
 }
//ボリュームデータ出力判定
if(( mode == 0 && now_time % step == 0 )||(mode == 1 && now_time % presimulationinterval == 0)){

#include <PBVR_u.h>
  #include <Isosurface_five.h>
  std::vector<float> pValues; //圧力
  std::vector<float> pointCoords; //頂点の座標値
  std::vector<float> cellCoords; //要素中心の座標値
  std::vector<int> cellPoints; //中心から頂点への接続情報
  std::vector<float> uValues; //速度の絶対値
  //std::vector<float> uVector; //速度のベクトル値
  //ボリュームデータ出力処理
#include "outputvolumedata.H"
  //KL情報量計算の実行判定および処理


  Isosurface_five( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time, 99998, 100002, "./constant/triSurface/realistic-cfd3.stl",0,0,10,100,99998,99999,100000,100001,100002 );

  /*  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time, 15, 50, "./constant/triSurface/realistic-cfd3.stl",0,0,10,repetitions,15 );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time, 15, 50, "./constant/triSurface/realistic-cfd3.stl",0,0,10,repetitions,25 );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time, 15, 50, "./constant/triSurface/realistic-cfd3.stl",0,0,10,repetitions,35 );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time, 15, 50, "./constant/triSurface/realistic-cfd3.stl",0,0,10,repetitions,45 );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time, 15, 50, "./constant/triSurface/realistic-cfd3.stl",0,0,10,repetitions,50 );
  */
  /*  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-9,12,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-6,12,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-3,12,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",0,12,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",3,12,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",6,12,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",9,12,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",12,12,12,repetitions );
  */
  /*  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-12,9,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-9,9,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-6,9,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-3,9,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",0,9,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",3,9,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",6,9,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",9,9,12,repetitions );
  PBVR_u( data_set[0], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 20000, 15, 50, "./constant/triSurface/realistic-cfd3.stl",12,9,12,repetitions );
  
  */

  
  //  return 0;
  

   
    std::vector<float>().swap(uValues);
    std::vector<float>().swap(pValues);
    std::vector<float>().swap(pointCoords);
    std::vector<float>().swap(cellCoords);
    std::vector<int>().swap(cellPoints);  

 } 




	      //PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 62, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-3,-3,10,100 );
	      		
	      /*		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 83, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-2,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 84, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-1,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 85, 15, 50, "./constant/triSurface/realistic-cfd3.stl",0,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 86, 15, 50, "./constant/triSurface/realistic-cfd3.stl",1,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 87, 15, 50, "./constant/triSurface/realistic-cfd3.stl",2,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 88, 15, 50, "./constant/triSurface/realistic-cfd3.stl",3,-1,10,repetitions );*/
	      //	      PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time/step - (noutperKL -i -1), 15, 50, "./constant/triSurface/realistic-cfd3.stl",4,-1,10,1 );

