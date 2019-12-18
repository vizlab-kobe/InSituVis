kvs::python::Tuple args( 2 );
args.set( 0, kvs::python::Array( new_hist ) );
args.set( 1, kvs::python::Array( old_hist ) );

entropy = kvs::python::Float( func.call( args ) );
MPI_Allreduce( &entropy, &entropy, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD );


if(entropy >= threshold){//パターンB(細かい可視化)
  KLpattern=2;  
 }else if( pre_entropy >= threshold ){ //パターンC(細かい可視化から粗い可視化)
  KLpattern=3;
 }else{//パターンA(粗い可視化)
  KLpattern=1;
 }

pre_entropy = entropy;
if( my_rank == 0 ){
  std::cout << "entropy : " << entropy << std::endl;
  writing_file << "," << entropy << std::endl;
 }
old_hist = new_hist;
