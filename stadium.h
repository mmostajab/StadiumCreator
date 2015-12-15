#ifndef __STADIUM_H__
#define __STADIUM_H__

#include <vector>
#include <stdint.h>

struct float3{
  float v[3];
};

struct AABB{
  float3 min, max;
};

struct Stadium {
  int                                         num_blocks;
  int                                         num_layer_types;
  int                                         num_layers;
  std::vector<int>                            block_sizes;
  std::vector<std::vector<std::vector<int>>>  layer_types;
  std::vector<int>                            layers;
};

void read_stadium(const std::string& stadium_filename, Stadium& stadium){
  std::ifstream infile(stadium_filename);
  if (!infile){
    std::cout << "===> Cannot open the " << stadium_filename << " file.\n";
    exit(0);
  }

  // Read the blocks
  infile >> stadium.num_blocks;
  stadium.block_sizes.resize(3 * stadium.num_blocks);
  for (int i = 0; i < stadium.num_blocks; i++)
    infile >> stadium.block_sizes[3 * i + 0] >> stadium.block_sizes[3 * i + 1] >> stadium.block_sizes[3 * i + 2];
  
  // Read the layer types
  infile >> stadium.num_layer_types;
  stadium.layer_types.resize(stadium.num_layer_types);
  for (auto& layer_type : stadium.layer_types){
    int dims[2];
    infile >> dims[0] >> dims[1];
    layer_type.resize(dims[0]);
    for (auto& row : layer_type){
      row.resize(dims[1]);
      for (auto& col : row)
        infile >> col;
    }
  }

  // Read the layers
  infile >> stadium.num_layers;
  stadium.layers.resize(stadium.num_layers);
  for (auto& layer : stadium.layers){
    infile >> layer;
  }
}

bool write_stadium(const std::string& filename, const Stadium& stadium){

  std::ofstream out(filename.c_str(), std::ios_base::binary);

  if (out)
  {

    std::vector<AABB>     cellBoxes;
    std::vector<float3>   points;
    std::vector<float3>   cellVectors;
    std::vector<uint32_t> cellPoints;
    std::vector<uint32_t> cellPointsBegIndices;
    std::vector<float3>   pointVectors;
    std::vector<float>    cellVolumes;

    for (int l = 0; l < stadium.num_layers; l++){
      float  elem_dim_z    = 1.0f / (stadium.num_layers - 1);
      float  offset_z = static_cast<float>(l) / (stadium.num_layers - 1);
      
      size_t offset_cellBoxes   = cellBoxes.size();
      size_t offset_points      = points.size();
      size_t offset_cellVectors = cellVectors.size();
      
      for (size_t i = 0; i < stadium.layer_types[l].size(); i++){
        for (size_t j = 0; j < stadium.layer_types[l][i].size(); j++){
          
          float elem_dim_x = 1.0f / static_cast<float>(stadium.layer_types[l].size() - 1);
          float elem_dim_y = 1.0f / static_cast<float>(stadium.layer_types[l][i].size() - 1);

          float offset_x = static_cast<float>(i) / static_cast<float>(stadium.layer_types[l].size() - 1);
          float offset_y = static_cast<float>(j) / static_cast<float>(stadium.layer_types[l][i].size() - 1);

          uint32_t block_type = stadium.layer_types[l][i][j];
          int dims[3] = { stadium.block_sizes[3 * block_type + 0], stadium.block_sizes[3 * block_type + 1], stadium.block_sizes[3 * block_type + 2] };

          for (int d0 = 0; d0 < dims[0]; d0++)
            for (int d1 = 0; d1 < dims[1]; d1++)
              for (int d2 = 0; d2 < dims[2]; d2++){

                float x = ;

              }

        }
      }
    }

    std::cout << "saveBinary: saving " << filename << std::endl;

    size_t cellBoxesNumber            = cellBoxes.size();
    size_t pointsNumber               = points.size();
    size_t cellPointsNumber           = cellPoints.size();
    size_t cellPointsBegIndicesNumber = cellPointsBegIndices.size();
    size_t cellVectorsNumber          = cellVectors.size();
    size_t pointVectorsNumber         = pointVectors.size();
    size_t cellVolumesNumber          = cellVolumes.size();


    out << cellBoxesNumber << std::endl;
    out << pointsNumber << std::endl;
    out << cellVectorsNumber << std::endl;
    out << cellPointsNumber << std::endl;
    out << cellPointsBegIndicesNumber << std::endl;
    out << pointVectorsNumber << std::endl;
    out << cellVolumesNumber << std::endl;


    out.write((char*)(&cellBoxes[0]),             sizeof(AABB)*cellBoxesNumber);
    out.write((char*)(&points[0]),                sizeof(float3)*pointsNumber);
    out.write((char*)(&cellVectors[0]),           sizeof(float3)*cellVectorsNumber);
    out.write((char*)(&cellPoints[0]),            sizeof(uint32_t)*cellPointsNumber);
    out.write((char*)(&cellPointsBegIndices[0]),  sizeof(uint32_t)*cellPointsBegIndicesNumber);
    out.write((char*)(&pointVectors[0]),          sizeof(float3)*pointVectorsNumber);
    out.write((char*)(&cellVolumes[0]),           sizeof(float)*cellVolumesNumber);
  }

  return !!out;

}

#endif