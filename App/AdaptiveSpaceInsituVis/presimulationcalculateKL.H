new_hist = kvs::ValueArray<float>( uValues );

kvs::python::Tuple args( 2 );
for(int numpre = 0; numpre < 20; numpre++){
  if((minpresimulation>numpre)&&(now_time%(numpre+10)==0)){
    args.set( 0, kvs::python::Array( new_hist ) );
    args.set( 1, kvs::python::Array( pre_hist[numpre] ) );
    entropy = kvs::python::Float( func.call( args ) );
    MPI_Allreduce( &entropy, &entropy, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );    
    if( my_rank == 0 ){
      std::cout << "entropy : " << entropy << std::endl;
      writing_file1[numpre] << "," << entropy << std::endl;
    }
  }
  
 }

minpresimulation++;
pre_hist[presimulationcount]=new_hist;
presimulationcount = (presimulationcount + 1)%20;
