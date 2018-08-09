program main
  use vis_m

  implicit none
  integer, parameter :: dimx = 3
  integer, parameter :: dimy = 3
  integer, parameter :: dimz = 3
  integer, parameter :: size = dimx * dimy * dimz
  real, dimension(:), allocatable :: values

  allocate( values( dimx * dimy * dimz ) )
  values = (/0, 10, 0, 50, 255, 200, 0, 100, 0, 50, 0, 150, 0, 0, 0, 150, 0, 50, 0, 255, 0, 50, 10, 50, 0, 255, 0/)

  call vis_ExternalFaces( values, size, dimx, dimy, dimz )

  deallocate( values )

end program main
