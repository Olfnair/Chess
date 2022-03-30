#ifndef MATRIX_H
#define MATRIX_H

#define EXC_MATRIX_DIM 1

template<typename T> class Matrix
{
 public:
  Matrix(const unsigned int xDim, const unsigned int yDim);
  Matrix(const unsigned int iDim);

  // copie et affectation
  Matrix(const Matrix<T>& b);
  Matrix<T>& operator=(const Matrix<T>& b);

  ~Matrix();
  unsigned int getSize() const;
  unsigned int getX() const;
  unsigned int getY() const;
  T& operator()(const unsigned int x, const unsigned int y);  // accès en coord x y (nécessite de calculer l'index résultant)
  T operator()(const unsigned int x, const unsigned int y) const;
  T& operator()(const unsigned int i);  // accès direct sans calcul préalable
  T operator()(const unsigned int i) const;

 private:
  void matrixInitData(const unsigned int iDim);

  T* data;  // => copie par défaut ne convient pas !
  unsigned int iDim;
  unsigned int xDim;
  unsigned int yDim;
};

#include "Matrix.inl"  // templates (donc inline)

#endif  // MATRIX_H
