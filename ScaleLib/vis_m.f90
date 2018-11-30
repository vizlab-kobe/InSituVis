module vis_m
  use iso_c_binding
  implicit none
  interface
     subroutine vis_ExternalFaces( values, size, dimx, dimy, dimz, x_coords, y_coords, z_coords, time )&
       bind( c, name="ExternalFaces" )
       import
       integer(c_int), value :: size
       real(c_double)        :: values(size)
       integer(c_int), value :: dimx
       integer(c_int), value :: dimy
       integer(c_int), value :: dimz
       real(c_double)        :: x_coords(dimx)
       real(c_double)        :: y_coords(dimy)
       real(c_double)        :: z_coords(dimz)
       integer(c_int), value :: time
     end subroutine vis_ExternalFaces
     subroutine vis_Isosurface( values, size, dimx, dimy, dimz, x_coords, y_coords, z_coords, time)&
       bind( c, name="Isosurface" )
       import
       integer(c_int), value :: size
       real(c_double)        :: values(size)
       integer(c_int), value :: dimx
       integer(c_int), value :: dimy
       integer(c_int), value :: dimz       
       real(c_double)        :: x_coords(dimx)
       real(c_double)        :: y_coords(dimy)
       real(c_double)        :: z_coords(dimz)
       integer(c_int), value :: time
     end subroutine vis_Isosurface
     subroutine vis_OrthoSlice( values, size, dimx, dimy, dimz, minx, miny, minz, maxx, maxy, maxz, time)&
       bind( c, name="OrthoSlice" )
       import
       integer(c_int), value :: size
       real(c_double)        :: values(size)
       integer(c_int), value :: dimx
       integer(c_int), value :: dimy
       integer(c_int), value :: dimz
       real(c_double), value :: minx
       real(c_double), value :: miny
       real(c_double), value :: minz
       real(c_double), value :: maxx
       real(c_double), value :: maxy
       real(c_double), value :: maxz
       integer(c_int), value :: time
     end subroutine vis_OrthoSlice
     subroutine vis_ParticleBasedRendering( values, size, dimx, dimy, dimz, x_coords, y_coords, z_coords, time)&
       bind( c, name="ParticleBasedRendering" )
       import
       integer(c_int), value :: size
       real(c_double)        :: values(size)
       integer(c_int), value :: dimx
       integer(c_int), value :: dimy
       integer(c_int), value :: dimz       
       real(c_double)        :: x_coords(dimx)
       real(c_double)        :: y_coords(dimy)
       real(c_double)        :: z_coords(dimz)
       integer(c_int), value :: time
     end subroutine vis_ParticleBasedRendering
     subroutine vis_RayCasting( values, size, dimx, dimy, dimz, minx, miny, minz, maxx, maxy, maxz, time)&
       bind( c, name="RayCasting" )
       import
       integer(c_int), value :: size
       real(c_double)         :: values(size)
       integer(c_int), value :: dimx
       integer(c_int), value :: dimy
       integer(c_int), value :: dimz
       real(c_double), value :: minx
       real(c_double), value :: miny
       real(c_double), value :: minz
       real(c_double), value :: maxx
       real(c_double), value :: maxy
       real(c_double), value :: maxz
       integer(c_int), value :: time
     end subroutine vis_RayCasting
     subroutine vis_Test( values, size, dimx, dimy, dimz )&
       bind( c, name="VisTest" )
       import 
       integer(c_int), value :: size
       real(c_double)         :: values(size)
       integer(c_int), value :: dimx
       integer(c_int), value :: dimy
       integer(c_int), value :: dimz

     end subroutine vis_Test
  end interface

end module vis_m
