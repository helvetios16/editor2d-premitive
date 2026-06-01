#pragma once
#include "vec.h"

/* ==================================================================
   mat3.h - Matrices homogeneas 3x3 para transformaciones 2D.
   Modulo matematico puro: no sabe nada de OpenGL ni de figuras. Permite
   trasladar, rotar y escalar puntos, y componer (multiplicar) varias
   transformaciones en una sola matriz.
   ================================================================== */

/* Matriz 3x3 almacenada por filas (row-major). El indice de la fila f,
   columna c es: m[f*3 + c]. Un punto 2D (x,y) se trata como el vector
   homogeneo (x, y, 1), lo que permite expresar la traslacion como una
   multiplicacion de matrices. */
struct Mat3 {
  float m[9];
};

/* Matriz identidad: al aplicarla, el punto no cambia. Es el punto de
   partida de todas las demas. */
Mat3 mat3Identity();

/* Matriz de traslacion: desplaza un punto (tx, ty). */
Mat3 mat3Translate(float tx, float ty);

/* Matriz de rotacion alrededor del origen.
   El angulo va en radianes (sentido antihorario). */
Mat3 mat3Rotate(float rad);

/* Matriz de escalado respecto al origen, independiente por eje
   (sx en X, sy en Y). */
Mat3 mat3Scale(float sx, float sy);

/* Producto de matrices: retorna a * b. Sirve para COMPONER
   transformaciones, p. ej. T * R * T^-1 para rotar respecto a un punto
   que no sea el origen. El orden de los factores importa. */
Mat3 mat3Mul(const Mat3 &a, const Mat3 &b);

/* Aplica la transformacion m al punto v (tratado como (x,y,1)) y retorna
   el punto resultante. Es donde la matriz realmente "toca" un vertice. */
Vec2 mat3Apply(const Mat3 &m, Vec2 v);
