module vis_m
  use iso_c_binding
  implicit none

  interface
     subroutine vis_ExternalFaces( values, size, dimx, dimy, dimz )&
       bind( c, name="ExternalFaces" )
       import
       integer(c_int), value :: size
       real(c_float)         :: values(size)
       integer(c_int), value :: dimx
       integer(c_int), value :: dimy
       integer(c_int), value :: dimz
     end subroutine vis_ExternalFaces
  end interface

end module vis_m
