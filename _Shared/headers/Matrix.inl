#ifndef MATRIX_INL
#define MATRIX_INL

#include "Matrix.h"

#include <new>
#include <cstring>

template<typename T> void Matrix<T>::matrixInitData(const unsigned int iDim)
{
  try
  {
    data = new T[iDim];
  }
  catch (std::bad_alloc& e)
  {
    // TODO : plus assez d'espace en mémoire : trouver une solution pour gérer ça.
    // pour l'instant, on va se contenter de relayer l'exception
    throw e;
  }
  std::memset(data, 0, iDim * sizeof(T));  // on init la zone mémoire allouée à 0
}

template<typename T> Matrix<T>::Matrix(const unsigned int xDim, const unsigned int yDim)
    : iDim(xDim * yDim), xDim(xDim), yDim(yDim)
{
  matrixInitData(iDim);
}

template<typename T> Matrix<T>::Matrix(const unsigned int iDim)
    : Matrix(iDim, 1)
{
}

template<typename T> Matrix<T>::Matrix(const Matrix<T>& b)
    : iDim(b.iDim), xDim(b.xDim), yDim(b.yDim)
{
  matrixInitData(iDim);
  for (unsigned int i = 0; i < iDim; ++i)
    data[i] = b.data[i];
}

template<typename T> Matrix<T>& Matrix<T>::operator=(const Matrix<T>& b)
{
  iDim = b.iDim;
  xDim = b.xDim;
  yDim = b.yDim;
  for (unsigned int i = 0; i < iDim; ++i)
    data[i] = b.data[i];
  return *this;
}

template<typename T> Matrix<T>::~Matrix()
{
  delete[] data;
}

template<typename T> unsigned int Matrix<T>::getSize() const
{
  return iDim;
}

template<typename T> unsigned int Matrix<T>::getX() const
{
  return xDim;
}

template<typename T> unsigned int Matrix<T>::getY() const
{
  return yDim;
}

template<typename T> T& Matrix<T>::operator()(const unsigned int x, const unsigned int y)
{
  if (x >= xDim || y >= yDim)
  {
    // TODO : lancer exception
    throw EXC_MATRIX_DIM;
  }
  return data[y * xDim + x];
}

template<typename T> T Matrix<T>::operator()(const unsigned int x, const unsigned int y) const
{
  if (x >= xDim || y >= yDim)
  {
    // TODO : lancer exception
    throw EXC_MATRIX_DIM;
  }
  return data[y * xDim + x];
}

template<typename T> T& Matrix<T>::operator()(const unsigned int i)
{  // accès direct sans calcul préalable
  // on ne vérifie rien : on veut que ce soit rapide
  return data[i];
}

template<typename T> T Matrix<T>::operator()(const unsigned int i) const
{
  // on ne vérifie rien : on veut que ce soit rapide
  return data[i];
}

#endif  // MATRIX_INL