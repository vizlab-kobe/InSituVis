#pragma once
#include <kvs/osmesa/Screen>
#include <kvs/ColorImage>

kvs::ColorImage Composition( kvs::osmesa::Screen& screen, int rank, int nnodes );
void CalculateMinMax( double& min_x, double& min_y, double& min_z, double& max_x, double& max_y, double& max_z, double& min_value, double& max_value );
