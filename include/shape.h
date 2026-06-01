#pragma once
#include "vec.h"
#include <vector>

/* ==================================================================
   shape.h - Define una figura (Shape) y los tipos de primitiva.
   Una Shape es el objeto que se dibuja, selecciona y transforma; la
   escena no es mas que una lista de Shapes.
   ================================================================== */

/* Tipos de primitivas que el editor sabe dibujar. El tipo determina como
   se renderiza la figura y como se mide la distancia del cursor a ella:
     POINT    -> un solo vertice (un punto)
     LINE     -> exactamente 2 vertices (un segmento)
     POLYLINE -> cadena abierta de vertices (linea quebrada)
     POLYGON  -> cadena cerrada de vertices (poligono, puede rellenarse) */
enum class ShapeType { POINT, LINE, POLYLINE, POLYGON };

/* Representa un objeto grafico concreto dentro de la escena.

   IMPORTANTE: los vertices se guardan YA TRANSFORMADOS, es decir, en sus
   coordenadas finales de pantalla. No se almacena una matriz de modelo
   por figura; al trasladar/rotar/escalar, la transformacion se aplica de
   inmediato a 'vertices' (ver shapeTransform). */
struct Shape {
  ShapeType type;            /* que clase de primitiva es */
  std::vector<Vec2> vertices; /* puntos que la componen (coords de pantalla) */
  Color outline;             /* color del contorno / linea */
  Color fill;                /* color de relleno (solo POLYGON) */
  bool filled;               /* true = se pinta el interior del poligono */
  bool selected;             /* true = esta seleccionada (se resalta) */
};
