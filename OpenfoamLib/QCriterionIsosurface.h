#pragma once
#include <iostream>
#include <vector>

void QCriterionIsosurface( const std::vector<float> &values, int ncells, int nnodes, const std::vector<float> &vertex_coords, const std::vector<float> &cell_coords, const std::vector<int> &label, int time, float min_value, float max_value);
//void CalculateMinMax( float& min_x, float& min_y, float& min_z, float& max_x, float& max_y, float & max_z );
//void CalculateValues( float& min_value, float& max_value );

