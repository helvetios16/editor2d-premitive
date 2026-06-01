#include "mat3.h"
#include <cmath>
#include <cstring>

/* ==================================================================
   mat3.cc - Implementacion de las matrices 3x3 (ver mat3.h).
   Recordatorio: row-major, indice m[fila*3 + col]; un punto es (x,y,1).
   ================================================================== */

/* --- Constructores de matrices elementales --- */

/* Identidad:  1 0 0 / 0 1 0 / 0 0 1.  No modifica al punto. */
Mat3 mat3Identity() {
  Mat3 r;
  memset(r.m, 0, sizeof(r.m));
  r.m[0] = r.m[4] = r.m[8] = 1.0f;
  return r;
}

/* Traslacion: la identidad con tx,ty en la ultima columna.
   Al multiplicar por (x,y,1) suma (tx,ty) al punto. */
Mat3 mat3Translate(float tx, float ty) {
  Mat3 r = mat3Identity();
  r.m[2] = tx;
  r.m[5] = ty;
  return r;
}

/* Rotacion alrededor del origen, bloque 2x2 con cos/sin:
     cos -sin
     sin  cos    (la ultima fila/columna queda como identidad). */
Mat3 mat3Rotate(float rad) {
  Mat3 r = mat3Identity();
  float c = cosf(rad), s = sinf(rad);
  r.m[0] = c;
  r.m[1] = -s;
  r.m[3] = s;
  r.m[4] = c;
  return r;
}

/* Escala respecto al origen: sx y sy en la diagonal principal. */
Mat3 mat3Scale(float sx, float sy) {
  Mat3 r = mat3Identity();
  r.m[0] = sx;
  r.m[4] = sy;
  return r;
}

/* Producto de dos matrices 3x3: retorna a * b.
   Triple bucle clasico: cada celda del resultado es el producto punto de
   una fila de 'a' por una columna de 'b'. Componer asi varias matrices
   (p. ej. T*R*T^-1) permite aplicar transformaciones combinadas en una
   sola pasada. El orden importa: a*b != b*a. */
Mat3 mat3Mul(const Mat3 &a, const Mat3 &b) {
  Mat3 r;
  memset(r.m, 0, sizeof(r.m));
  for (int row = 0; row < 3; row++)
    for (int col = 0; col < 3; col++)
      for (int k = 0; k < 3; k++)
        r.m[row * 3 + col] += a.m[row * 3 + k] * b.m[k * 3 + col];
  return r;
}

/* Transforma el punto v usando la matriz (coordenada homogenea w=1).
   Solo se calculan x e y; la tercera fila (0 0 1) daria w=1 y se omite. */
Vec2 mat3Apply(const Mat3 &m, Vec2 v) {
  float x = m.m[0] * v.x + m.m[1] * v.y + m.m[2];
  float y = m.m[3] * v.x + m.m[4] * v.y + m.m[5];
  return {x, y};
}
