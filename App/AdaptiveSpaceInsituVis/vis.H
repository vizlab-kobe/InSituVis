
const int now_time = static_cast<int>(atof(runTime.timeName().c_str())/runTime.deltaT().value());
const int start_time = static_cast<int>( runTime.startTimeIndex() );

//可視化初期処理
if( now_time -1 == start_time ){
  std::vector<float> first_uValues;
  forAll( mesh.cellPoints(), i )
    {
      first_uValues.push_back( static_cast<float>( mag(U[i]) ) );
      old_hist = kvs::ValueArray<float>( first_uValues );
    }
  std::vector<float>().swap(first_uValues);
 }


//ボリュームデータ出力判定
if(( mode == 0 && now_time % step == 0 )||(mode == 1 && now_time % presimulationinterval == 0)){

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
  if((count >= noutperKL)||(mode == 1)){
    new_hist = kvs::ValueArray<float>( uValues );
    if( old_hist.size() != 0)
      {
	if(mode == 0){
	  distribution_timer.start();
	}
#include "calculateKL.H"
	if(mode == 0){
	  distribution_timer.stop();
	  distribution_time = distribution_timer.sec();	  
	  timeswitch=3;
#include "calculatetime.H"	  
	  count=0;
	}
      }
  }
 
  
  //可視化の実行判定および処理
  if(mode == 0){
      vis_timer.start();
	if( KLpattern==2 ) //パターンB(細かい可視化)
	  {
	    for( size_t i = 0; i < data_set.size(); i++)
	      {
		
		//PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time/step - ( volume_size - i -1 ) , 15, 50, "./constant/triSurface/realistic-cfd3.stl",1,1,1,repetitions );
		//PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 62, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-3,-3,10,100 );
		
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 83, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-2,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 84, 15, 50, "./constant/triSurface/realistic-cfd3.stl",-1,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 85, 15, 50, "./constant/triSurface/realistic-cfd3.stl",0,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 86, 15, 50, "./constant/triSurface/realistic-cfd3.stl",1,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 87, 15, 50, "./constant/triSurface/realistic-cfd3.stl",2,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 88, 15, 50, "./constant/triSurface/realistic-cfd3.stl",3,-1,10,repetitions );
		PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, 89, 15, 50, "./constant/triSurface/realistic-cfd3.stl",4,-1,10,repetitions );
	      }
	  }
	else
	  {
	    if( KLpattern==3 ) //パターンC(細かい可視化から粗い可視化)
	      {		
		for( size_t j = 0; j < int(data_set.size()/2); j++)
		  {		    
		    PBVR_u( data_set[j], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time/step - (volume_size -j -1),  15, 50,"./constant/triSurface/realistic-cfd3.stl",1,1,1,1 );
		  }
		for( size_t k = int(data_set.size()/2) + vis_skip; k <data_set.size(); k+= vis_skip)
		  {
		    PBVR_u( data_set[k], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time/step - (volume_size -k -1), 15, 50,"./constant/triSurface/realistic-cfd3.stl",1,1,1,1 );
		  }
	      }
	    else //パターンA(粗い可視化)
	      {
		for( size_t i = data_set.size()*vis_skip-1; i < data_set.size(); i += vis_skip)
		  {    
		    PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time/step - (volume_size -i -1), 15, 50,"./constant/triSurface/realistic-cfd3.stl",1,1,1,1 );
		  }
	      }
	  }
  
      
      
      

      data_set.clear();
      
      vis_timer.stop();
      vis_time = vis_timer.sec();
      
      timeswitch=2;
#include "calculatetime.H"
    

    
    
  }else{//(mode==1)
    data_set.clear();
  }
  std::vector<float>().swap(uValues);
  std::vector<float>().swap(pValues);
  std::vector<float>().swap(pointCoords);
  std::vector<float>().swap(cellCoords);
  std::vector<int>().swap(cellPoints);
  
  
  

 }
