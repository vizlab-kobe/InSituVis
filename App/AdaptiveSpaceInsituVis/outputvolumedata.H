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
data_set.push_back( pValues );
