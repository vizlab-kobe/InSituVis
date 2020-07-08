#pragma once
#include <iostream>
#include <vector>

void MultiplePBVR( const std::vector<float> &p_values, const std::vector<float> &u_values,int ncells, int nnodes, const std::vector<float> &vertex_coords, const std::vector<float> &cell_coords, const std::vector<int> &label, int time, float p_min_value, float p_max_value, float u_min_value, float u_max_value );
//void CalculateMinMax( float& min_x, float& min_y, float& min_z, float& max_x, float& max_y, float & max_z );

