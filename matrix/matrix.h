#pragma once
#include <vector>
#include <array>
#include <initializer_list>

#define sztUNDEF (size_t) 0xFFFF

template <bool B, typename T>
using EnableIf = typename std::enable_if<B, T>::type;

struct Slice {

  Slice() : start{ sztUNDEF }, size{ sztUNDEF }, stride{ 1 }
  {}
  explicit Slice(size_t s) : start{ s }, size{ sztUNDEF }, stride{ 1 }
  {}
  Slice(size_t start, size_t size, size_t stride = 1) : start{ start }, size{ size }, stride{ stride }
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
size_t sliceDimension(const MatrixSlice<N>& oldSlice, MatrixSlice<N>& newSlice, size_t offset, size_t index)
{
  size_t current_dim = C + offset - 1;
  newSlice.extents[current_dim] = 0;
  newSlice.size = newSlice.strides[current_dim];
  return oldSlice.start + index * oldSlice.strides[current_dim];
}

template <size_t C, size_t N>
size_t sliceDimension(const MatrixSlice<N>& oldslice, MatrixSlice<N>& newSlice, size_t offset, const Slice& slice)
{
  size_t current_dim = C + offset - 1;

  if (slice.start != sztUNDEF) {
    newSlice.size = slice.start > 0 ? (slice.start - 1) * newSlice.strides[current_dim] : newSlice.size;
    newSlice.start += slice.start * newSlice.strides[current_dim];
  }

  if (slice.size != sztUNDEF) {
    newSlice.size = slice.size < newSlice.extents[current_dim] ? newSlice.size - (newSlice.extents[current_dim] - slice.size) * newSlice.strides[current_dim] : newSlice.size;
    newSlice.extents[current_dim] = std::ceil(static_cast<double>((slice.size - slice.start)) / static_cast<double>(slice.stride));
  }
  newSlice.strides[current_dim] = newSlice.strides[current_dim] * slice.stride;

  return 0;
}

template <size_t N, typename T, typename... Args >
size_t sliceMatrix(const MatrixSlice<N>& oldSlice, MatrixSlice<N>& newSlice, size_t offset, const T& arg, const Args&... args)
{
  size_t m = sliceDimension<sizeof...(Args) + 1>(oldSlice, newSlice, offset, arg);
  size_t n = sliceMatrix(oldSlice, newSlice, offset, args...);
  return m + n;
};

template <size_t N>
size_t sliceMatrix(const MatrixSlice<N>& oldSlice, MatrixSlice<N>& newSlice, size_t offset)
{
  return 0;
};

template <typename T, size_t N>
struct MatrixInitializerRec {
  using type = std::initializer_list<typename MatrixInitializerRec<T, N - 1>::type>;
};

template <typename T>
struct MatrixInitializerRec<T, 1> {
  using type = std::initializer_list<T>;
};

template <typename T>
struct MatrixInitializerRec <T, 0>;

template <typename T, size_t N>
using MatrixInitializer = typename MatrixInitializerRec<T, N>::type;

template <size_t N, typename E, typename L>
EnableIf< (N == 1), void> addExtents(E& first, L& list)
{
  *first++ = list.size();
}

template <size_t N, typename E, typename L>
EnableIf< (N > 1), void> addExtents(E& first, L& list)
{
  auto start = list.begin();
  for (auto i = start + 1; i != list.end(); ++i)
    if (i->size() != start->size()) throw "error not all lists are the same size";

  *first = list.size();
  addExtents<N-1>(++first, *list.begin());
}

template <typename T>
void copyFromInitList(const T* s_start, const T* s_end, std::vector<T>& destination)
{
  destination.insert(destination.end(), s_start, s_end);
}

template <typename L, typename T>
void copyFromInitList(const std::initializer_list<L>* s_start, const std::initializer_list<L>* s_end, std::vector<T>& destination) {
  for (; s_start != s_end; s_start++)
    copyFromInitList(s_start->begin(), s_start->end(), destination);
}


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
  explicit Matrix(Exts... extents);

  Matrix(MatrixInitializer<T, N> list);
  Matrix& operator=(MatrixInitializer<T, N>list);

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
    static_assert(sizeof...(Args) == N, "");
    constexpr size_t offset = N - (sizeof...(Args));
    MatrixSlice<N> slice = _descriptor;
    slice.start = sliceMatrix(_descriptor, slice, offset, args...);
    return { slice, _elements.data() };
  }

private:
  std::vector<T> _elements;
  MatrixSlice<N> _descriptor;
};

template<typename T, size_t N>
template<typename ...Exts>
inline Matrix<T, N>::Matrix(Exts ...extents)
  : _descriptor{ extents... }, _elements(_descriptor.size)
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
  strides[0] = 1;

  for (long int i = 1; i < N; i++) {
    strides[i] = strides[i - 1] * *(extents.begin() + i - 1);
  }

  size_t _size = 1;
  for (size_t d : extents) _size *= d;
  size = _size;
}

template<typename T, size_t N>
inline MatrixRef<T, N>::MatrixRef(const MatrixSlice<N>& descriptor, T* pointer) : _descriptor{ descriptor }, _pointer{ pointer }
{}

template<typename T, size_t N>
inline Matrix<T, N>::Matrix(MatrixInitializer<T, N> list)
{
  auto extents_begin = _descriptor.extents.begin();
  addExtents<N>(extents_begin, list);
  _descriptor.size = 1;
  for (size_t e : _descriptor.extents)
    _descriptor.size *= e;
  _elements.reserve(_descriptor.size);
  copyFromInitList(list.begin(), list.end(), _elements);
}

template<typename T, size_t N>
inline Matrix<T, N>& Matrix<T, N>::operator=(MatrixInitializer<T, N> list)
{
  auto extents_begin = _descriptor.extents.begin();
  addExtents<N>(extents_begin, list);
  _descriptor.size = 1;
  for (size_t e : _descriptor.extents)
    _descriptor.size *= e;
  _elements.reserve(_descriptor.size);
  copyFromInitList(list.begin(), list.end(), _elements);
}
