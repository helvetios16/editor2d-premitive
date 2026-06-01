#pragma once

/* ==================================================================
   vec.h - Tipos primitivos compartidos por todo el editor.
   No contiene logica: solo define las dos "unidades minimas" de datos
   que usan el resto de modulos (figuras, mouse, colores, matrices).
   ================================================================== */

/* Punto o vector en 2D, en coordenadas de pantalla (pixeles).
   Se reutiliza para: cada vertice de una figura, la posicion del cursor
   y los desplazamientos (delta) durante un arrastre.
   El origen (0,0) esta arriba-izquierda y la Y crece hacia abajo. */
struct Vec2 {
  float x, y;
};

/* Color RGB con componentes normalizados en el rango [0.0, 1.0], que es
   el formato que espera OpenGL en glColor3f. Cada figura guarda un Color
   de contorno y otro de relleno. */
struct Color {
  float r, g, b;
};
