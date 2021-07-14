#include <kvs/ColorImage>
#include <kvs/Vector3>
#include <kvs/RGBColor>
#include <cmath>
#include <iostream>
#include <string>

inline kvs::Real32 MSE( const kvs::ColorImage& image1, const kvs::ColorImage& image2 );
inline kvs::Real32 PSNR( const kvs::ColorImage& image1, const kvs::ColorImage& image2, const kvs::Real32 mse );
int do_PSNR( const std::string name1, const std::string name2 );
