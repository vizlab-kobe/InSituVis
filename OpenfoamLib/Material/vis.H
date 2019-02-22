
const int now_time = static_cast<int>(atof(runTime.timeName().c_str())/runTime.deltaT().value());
const int step = 5; //細かく可視化するときの可視化間隔(Δt')
const int start_time = static_cast<int>( runTime.startTimeIndex() );

if( now_time -1 == start_time ){
  std::vector<float> first_uValues;
  forAll( mesh.cellPoints(), i )
    {
      first_uValues.push_back( static_cast<float>( mag(U[i]) ) );
      old_hist = kvs::ValueArray<float>( first_uValues );
    }
  std::vector<float>().swap(first_uValues);
 }

if( now_time % step == 0 ){
#include <PBVR_u.h>
  std::vector<float> pValues; //圧力
  std::vector<float> pointCoords; //頂点の座標値
  std::vector<float> cellCoords; //要素中心の座標値
  std::vector<int> cellPoints; //中心から頂点への接続情報
  std::vector<float> uValues; //速度の絶対値
  //std::vector<float> uVector; //速度のベクトル値

  vis_timer.start();
  forAll( mesh.cellPoints(), i )
    {
      uValues.push_back( static_cast<float>( mag(U[i]) ) );
      pValues.push_back( static_cast<float>( p[i] ) );
      cellCoords.push_back( static_cast<float>( mesh.C()[i].x() ) );
      cellCoords.push_back( static_cast<float>( mesh.C()[i].y() ) );
      cellCoords.push_back( static_cast<float>( mesh.C()[i].z() ) );
      //uVector.push_back( static_cast<float>( U[i].x() ) );
      //uVector.push_back( static_cast<float>( U[i].y() ) );
      //uVector.push_back( static_cast<float>( U[i].z() ) );
      //六面体のみを抜き出すための処理
      if( mesh.cellPoints()[i].size() == 8 && mesh.cells()[i].size() == 6 )
	{
	  for(auto itr = mesh.cellPoints()[i].begin(); itr != mesh.cellPoints()[i].end(); itr++)
	    {
	      cellPoints.push_back( static_cast<int>(*itr) );
	    }
	}
      else
	{
	  for( int j = 0; j < 8; j++)
	    {
	      cellPoints.push_back( -1 );
	    }
	}
    }
  
  forAll(mesh.points(), k)
    {
      pointCoords.push_back( static_cast<float>( mesh.points()[k].x() ) );
      pointCoords.push_back( static_cast<float>( mesh.points()[k].y() ) );
      pointCoords.push_back( static_cast<float>( mesh.points()[k].z() ) );  
    }

  vis_timer.stop();
  vis_time = vis_timer.sec();
  MPI_Allreduce( &vis_time, &vis_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
  
  data_set.push_back( uValues );
  count++;
  if( count >= volume_size )
    {
      new_hist = kvs::ValueArray<float>( uValues );
      if( old_hist.size() != 0)
	{
	  distribution_timer.start();
	  kvs::python::Tuple args( 2 );
	  args.set( 0, kvs::python::Array( new_hist ) );
	  args.set( 1, kvs::python::Array( old_hist ) );

	  float entropy = 0.0;
	  entropy = kvs::python::Float( func.call( args ) );
	  MPI_Allreduce( &entropy, &entropy, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
	  distribution_timer.stop();  
	  if( entropy >= threshold ) //パターンB(細かい可視化)
	    {
	      for( size_t i = 0; i < data_set.size(); i++)
		{
		  PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time/step - ( volume_size - i -1 ) , 15, 50 );
		}
	    }
	  else
	    {
	      if( pre_entropy >= threshold ) //パターンC(細かい可視化から粗い可視化)
		{
		  for( size_t j = 0; j < int(data_set.size()/2); j++)
		    {
		      PBVR_u( data_set[j], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time/step - (volume_size -j -1),  15, 50);
		    }
		  for( size_t k = int(data_set.size()/2) + vis_skip; k <data_set.size(); k+= vis_skip)
		    {
		      PBVR_u( data_set[k], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time/step - (volume_size -k -1), 15, 50 );
		    }
		}      
	      else //パターンA(粗い可視化)
		{
		  for( size_t i = vis_skip-1; i < data_set.size(); i += vis_skip)
		    {
		      std::cout << "vis num " << now_time/step - ( volume_size - i -1 ) << std::endl;
		      PBVR_u( data_set[i], mesh.nCells(), mesh.nPoints(), pointCoords, cellCoords, cellPoints, now_time/step - (volume_size -i -1), 15, 50 );
		    }
		}
	    }
	  pre_entropy = entropy;
	  if( my_rank == 0 ){
	    std::cout << "entropy : " << entropy << std::endl;
	    writing_file << "," << entropy << std::endl;
	  }
	}
      distribution_time = distribution_timer.sec();
      MPI_Allreduce( &distribution_time, &distribution_time, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );
      count = 0;
      data_set.clear();
      old_hist = new_hist;           
    }
  
  std::vector<float>().swap(uValues);
  std::vector<float>().swap(pValues);
  std::vector<float>().swap(pointCoords);
  std::vector<float>().swap(cellCoords);
  std::vector<int>().swap(cellPoints);
  
 }

