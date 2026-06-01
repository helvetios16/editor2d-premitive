#pragma once
#include "mat3.h"
#include "shape.h"
#include <vector>

/* ==================================================================
   scene.h - La escena (coleccion de figuras) y sus operaciones.
   Capa de dominio: agrupa todo lo que se le hace a un conjunto de
   Shapes: dibujarlas, seleccionar la mas cercana (hit-test), borrarlas
   y transformarlas.
   ================================================================== */

/* Contenedor de todos los objetos graficos del editor.
   Es, basicamente, "el documento" sobre el que se trabaja. */
struct Scene {
  std::vector<Shape> shapes;
};

/* Agrega una figura al final de la escena. */
void sceneAdd(Scene &sc, const Shape &s);

/* HIT-TEST: retorna el indice de la figura mas cercana al cursor, siempre
   que este dentro del umbral de seleccion (10 px); si ninguna figura esta
   lo bastante cerca, retorna -1. Es el corazon de la seleccion. */
int scenePick(const Scene &sc, Vec2 cursor);

/* Elimina la primera figura marcada como seleccionada. Tras la llamada,
   el selectedIndex del editor queda invalido y debe resetearse a -1. */
void sceneDeleteSelected(Scene &sc);

/* Quita la marca 'selected' de todas las figuras. Se llama antes de
   seleccionar una nueva para no acumular selecciones. */
void sceneClearSelection(Scene &sc);

/* Dibuja TODA la escena usando OpenGL inmediato: recorre las figuras y
   delega en el dibujante interno de cada una (drawShape). */
void sceneRender(const Scene &sc);

/* Calcula el centroide (promedio de los vertices) de una figura. Es el
   punto que se usa como pivote para rotar y escalar. */
Vec2 shapeCentroid(const Shape &s);

/* Aplica la matriz M a TODOS los vertices de la figura, sobrescribiendolos.
   Asi se "hornean" (bake) las transformaciones: la figura queda ya en su
   nueva posicion, sin guardar la matriz aparte. */
void shapeTransform(Shape &s, const Mat3 &M);
