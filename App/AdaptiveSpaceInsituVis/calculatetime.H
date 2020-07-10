/*
//時間の計測およびその結果の出力のためのプログラム

if ( timeswitch == 1 )
{
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
