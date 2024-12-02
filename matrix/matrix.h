#pragma once
#include <vector>
#include <array>
#include <initializer_list>

#define sztUNDEF (size_t) 0xFFFF

struct Slice {

  Slice() : start{ sztUNDEF }, size{ sztUNDEF }, stride{ 1 }
  {}
  explicit Slice(size_t s) : start{ s }, size{ sztUNDEF }, stride{ 1 }
  {}
  Slice(size_t start, size_t size, size_t stride=1) : start{ start }, size{ size }, stride{ stride }
  {}

  size_t start;
  size_t size;
  size_t stride;
};

template <size_t N>
struct MatrixSlice
{
  MatrixSlice() = default;

  MatrixSlice(size_t s, std::initializer_list<size_t> extents);
  MatrixSlice(size_t s, std::initializer_list<size_t> extents, std::initializer_list<size_t> strides);
  
  template <typename... Dims>
  MatrixSlice(Dims... dimensions);

  size_t size;
  size_t start;
  std::array<size_t, N> extents;
  std::array<size_t, N> strides;
};

template <typename T, size_t N>
class MatrixRef
{
public:
  static constexpr size_t Order = N;
  using value_type = T;
  using iterator = typename std::vector<T>::iterator;
  using const_iterator = typename std::vector<T>::const_iterator;

  MatrixRef() = delete;
  MatrixRef(MatrixRef&&) = default;
  MatrixRef& operator=(MatrixRef&&) = default;
  MatrixRef(const MatrixRef&) = default;
  MatrixRef& operator=(const MatrixRef&) = default;
  ~MatrixRef() = default;

  MatrixRef(const MatrixSlice<N>& descriptor, T* pointer);

  template<typename U>
  MatrixRef(std::initializer_list<U>) = delete;
  template<typename U>
  MatrixRef& operator=(std::initializer_list<U>) = delete;

  static constexpr size_t order() { return N; }
  size_t extent(size_t n) const { return _descriptor.extents[n]; }
  size_t size() const { return _pointer->size(); }
  const MatrixSlice<N>& descriptor() const { return _descriptor; }

  T* data() { return _pointer->data(); }
  const T* data() const { return _pointer; }

private:
  T* _pointer;
  MatrixSlice<N> _descriptor;
};

template <size_t C, size_t N>
size_t sliceDimension(const MatrixSlice<N>& oldSlice, MatrixSlice<N>& newSlice, size_t index)
{
  newSlice.extents[C - 1] = 0;
  return oldSlice.start + index * oldSlice.strides[C - 1];
}

template <size_t C, size_t N>
size_t sliceDimension(const MatrixSlice<N>& oldslice, MatrixSlice<N>& newSlice, const Slice& slice)
{
  if (slice.start != sztUNDEF) {
    newSlice.size = slice.start > 0 ? (slice.start - 1) * newSlice.strides[C - 1] : newSlice.size;
    newSlice.start += slice.start * newSlice.strides[C - 1];
  }

  if (slice.size != sztUNDEF) {
    newSlice.size = slice.size < newSlice.extents[C - 1] ? (newSlice.extents[C - 1] - slice.size) * newSlice.strides[C - 1] : newSlice.size;
    newSlice.extents[C - 1] = std::ceil(static_cast<double>((slice.size - slice.start)) / static_cast<double>(slice.stride));
  }

  newSlice.strides[C - 1] = newSlice.strides[C - 1] * slice.stride;

  return 0;
}

template <size_t N, typename T, typename... Args >
size_t sliceMatrix(const MatrixSlice<N>& oldSlice, MatrixSlice<N>& newSlice, const T& arg, const Args&... args)
{
  size_t m = sliceDimension<sizeof...(Args) + 1>(oldSlice, newSlice, arg);
  size_t n = sliceMatrix(oldSlice, newSlice, args...);
  return m + n;
};

template <size_t N>
size_t sliceMatrix(const MatrixSlice<N>& oldSlice, MatrixSlice<N>& newSlice)
{
  return 0;
};


template <typename T, size_t N>
class Matrix
{
public:
  static constexpr size_t Order = N;
  using value_type = T;
  using iterator = typename std::vector<T>::iterator;
  using const_iterator = typename std::vector<T>::const_iterator;

  Matrix() = default;
  Matrix(Matrix&&) = default;
  Matrix& operator=(Matrix&&) = default;
  Matrix(const Matrix&) = default;
  Matrix& operator=(const Matrix&) = default;
  ~Matrix() = default;

  template <typename... Exts>
  Matrix(Exts... extents);

  template<typename U>
  Matrix(std::initializer_list<U>) = delete;
  template<typename U>
  Matrix& operator=(std::initializer_list<U>) = delete;

  static constexpr size_t order() { return N; }
  size_t extent(size_t n) const { return _descriptor.extents[n]; }
  size_t size() const { return _elements.size(); }
  const MatrixSlice<N>& descriptor() const { return _descriptor; }

  T* data() { return _elements.data(); }
  const T* data() const { return _elements.data(); }

  void fill() {
    for (T i = 0; i < _descriptor.size; i++) {
      _elements.push_back(i);
    }
  }

  template <typename... Args >
  MatrixRef<T, N> operator()(const Args&... args) 
  {
      MatrixSlice<N> slice = _descriptor;
      slice.start = sliceMatrix(_descriptor, slice, args...);
      return { slice, _elements.data()};
  }

private:
  std::vector<T> _elements;
  MatrixSlice<N> _descriptor;
};

template<typename T, size_t N>
template<typename ...Exts>
inline Matrix<T, N>::Matrix(Exts ...extents)
  : _descriptor{ extents... }, _elements( _descriptor.size )
{}

template<size_t N>
inline MatrixSlice<N>::MatrixSlice(size_t s, std::initializer_list<size_t>)
  : extents{ extents }
{
  strides[N - 1] = 1;
  for (size_t i = N - 2; i >= 0; i--) {
    strides[i] = strides[i + 1] * extents[i + 1];
  }
}

template<size_t N>
inline MatrixSlice<N>::MatrixSlice(size_t s, std::initializer_list<size_t> extents, std::initializer_list<size_t> strides)
  : extents{ extents }, strides{ strides }
{}

template<size_t N>
template<typename ...Dims>
inline MatrixSlice<N>::MatrixSlice(Dims ...dimensions)
{
  static_assert(sizeof...(dimensions) == N, "ERROR");
  extents = { static_cast<size_t>(dimensions)... };
  strides[N-1] = 1;
  
  for (long int i = N - 2; i >= 0; i--) {
    strides[i] = strides[i + 1] * *(extents.begin() + i + 1);
  }

  size_t _size = 1;
  for (size_t d : extents) _size *= d;
  size = _size;
}

template<typename T, size_t N>
inline MatrixRef<T, N>::MatrixRef(const MatrixSlice<N>& descriptor, T* pointer) : _descriptor{ descriptor }, _pointer{ pointer }
{}