#include <iostream>
#include <algorithm>
#include <numeric>

#include "matrix.h"

// 14:00

int main(int argc, char* argv[])
{
  //Matrix<int, 4> mtx(2, 2, 3, 4);

  //Matrix<int, 1> mtx(5);
  //auto info = mtx.descriptor();

  //mtx.fill();
  //MatrixRef<int, 1> subMtx = mtx(2);

  //std::vector<size_t> vec { 0, 1, 0 };

  //std::cout << std::inner_product(vec.begin(), vec.end(), info.strides.begin(), 0) << std::endl;

  Matrix<int, 2> mtx {
    {54, 12, 33, 7},
    {8, 56, 33, 14},
    {8, 8, 7, 7},
  };

}