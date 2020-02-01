#include <kvs/ColorImage>
#include <kvs/Vector3>
#include <kvs/RGBColor>
#include <cmath>
#include <iostream>

inline kvs::Real32 MSE( const kvs::ColorImage& image1, const kvs::ColorImage& image2 );
inline kvs::Real32 PSNR( const kvs::ColorImage& image1, const kvs::ColorImage& image2, const kvs::Real32 mse );
int do_PSNR( char* name1, char* name2 );
