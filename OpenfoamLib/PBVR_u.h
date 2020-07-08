#pragma once
#include <iostream>
#include <vector>

//void PBVR_u( const std::vector<float> &values, int ncells, int nnodes, const std::vector<float> &vertex_coords, const std::vector<float> &cell_coords, const std::vector<int> &label, int time, float min_value, float max_value, std::string stlpath, float cameraposx, float cameraposy, float cameraposz, const size_t repetitions, float isothr);
void PBVR_u( const std::vector<float> &values, int ncells, int nnodes, const std::vector<float> &vertex_coords, const std::vector<float> &cell_coords, const std::vector<int> &label, int time, float min_value, float max_value, std::string stlpath, float cameraposx, float cameraposy, float cameraposz, const size_t repetitions);
//void CalculateMinMax( float& min_x, float& min_y, float& min_z, float& max_x, float& max_y, float & max_z );
