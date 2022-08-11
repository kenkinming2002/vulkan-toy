#pragma once

#include <stddef.h>
#include <assert.h>

// Believe it or not this dynarray, vector and span template would cover 99% of the use case
template<typename T>
struct vector
{
  T* data;
  size_t size;
  size_t capacity;

  T& operator[](size_t i) const { assert(i<size); return data[i]; }
};

template<typename T> T* begin(vector<T> vector) { return &vector.data[0]; }
template<typename T> T* end(vector<T> vector)   { return &vector.data[vector.size]; }
template<typename T> size_t size(vector<T> span) { return span.size; }
template<typename T> T* data(vector<T> span) { return span.data; }

template<typename T>
vector<T> create_vector(size_t capacity)
{
  return vector<T>{
    .data = new T[capacity],
    .size = 0,
    .capacity = capacity,
  };
}

template<typename T>
void destroy_vector(vector<T>& vector)
{
  delete[] vector.data;
  vector.data     = nullptr;
  vector.size     = 0;
  vector.capacity = 0;
}

template<typename T>
void vector_push(vector<T>& vector, T value)
{
  assert(vector.size != vector.capacity);
  vector.data[vector.size++] = value;
}

template<typename T>
struct dynarray
{
  T* data;
  size_t size;

  T& operator[](size_t i) const { assert(i<size); return data[i]; }
};

template<typename T> T* begin(dynarray<T> dynarray) { return &dynarray.data[0]; }
template<typename T> T* end(dynarray<T> dynarray)   { return &dynarray.data[dynarray.size]; }
template<typename T> size_t size(dynarray<T> span) { return span.size; }
template<typename T> T* data(dynarray<T> span) { return span.data; }

template<typename T>
dynarray<T> create_dynarray(size_t capacity)
{
  return dynarray<T>{
    .data = new T[capacity],
    .size = capacity,
  };
}

template<typename T>
void destroy_dynarray(dynarray<T>& dynarray)
{
  delete[] dynarray.data;
  dynarray.data = nullptr;
  dynarray.size = 0;
}

template<typename T>
dynarray<T> vector_to_dynarray(vector<T> vector)
{
  assert(vector.size == vector.capacity);
  return dynarray<T>{
    .data = vector.data,
    .size = vector.size,
  };
}

template<typename T>
vector<T> dynarray_to_vector(dynarray<T> dynarray)
{
  return vector<T>{
    .data     = dynarray.data,
    .size     = dynarray.size,
    .capacity = dynarray.size,
  };
}

template<typename T>
struct span
{
  T* data;
  size_t size;

  T& operator[](size_t i) const { assert(i<size); return data[i]; }
};

template<typename T> T* begin(span<T> span) { return &span.data[0]; }
template<typename T> T* end(span<T> span)   { return &span.data[span.size]; }
template<typename T> size_t size(span<T> span) { return span.size; }
template<typename T> T* data(span<T> span) { return span.data; }

template<typename T, size_t N>
span<T> create_span(T(&data)[N])
{
  return span<T>{
    .data = data,
    .size = N,
  };
}

template<typename T>
span<T> vector_as_span(vector<T> vector)
{
  assert(vector.size == vector.capacity);
  return span<T>{
    .data = vector.data,
    .size = vector.size,
  };
}

template<typename T>
span<T> dynarray_as_span(dynarray<T> dynarray)
{
  return span<T>{
    .data = dynarray.data,
    .size = dynarray.size,
  };
}

