#include <kvs/ColorImage>
#include <kvs/Vector3>
#include <kvs/RGBColor>
#include <cmath>
#include <iostream>
#include "PSNR.h"


inline kvs::Real32 MSE( const kvs::ColorImage& image1, const kvs::ColorImage& image2 )
{
    kvs::Real32 sum = 0.0f;
    for ( size_t index = 0; index < image1.numberOfPixels(); index++ )
    {
        const kvs::Vec3 p1 = image1.pixel( index ).toVec3();
        const kvs::Vec3 p2 = image2.pixel( index ).toVec3();
        const kvs::Real32 length = ( p2 - p1 ).length();
        sum += length * length;
    }

    return sum / ( 3.0f * image1.numberOfPixels() );
}

inline kvs::Real32 PSNR( const kvs::ColorImage& image1, const kvs::ColorImage& image2, const kvs::Real32 mse )
{
    const kvs::Real32 max_value = 1.0f;
    return 10.0f * std::log10( max_value * max_value / mse );
}

int do_PSNR( char* name1, char* name2 )
{
    kvs::ColorImage image1( name1 );
    kvs::ColorImage image2( name2 );

    const kvs::Real32 mse = MSE( image1, image2 );
    const kvs::Real32 psnr = PSNR( image1, image2, mse );

    if(psnr >= 30)
        return 1;
    else
        return 0;
}
