#pragma once
#include "scene.h"
#include "ui.h"
#include "vec.h"
#include <vector>

/* ==================================================================
   tools.h - El "cerebro" del editor: estado y maquina de estados.
   Define la herramienta activa (Tool), todo el estado de la sesion
   (EditorState) y las funciones que convierten gestos del usuario
   (clicks, arrastres, acciones) en cambios sobre la escena.
   ================================================================== */

/* Herramienta activa en el editor. La herramienta seleccionada decide
   como se interpreta cada gesto:
     - de DIBUJO (POINT/LINE/POLYLINE/POLYGON): se usan con clicks.
     - de TRANSFORMACION (SELECT/TRANSLATE/ROTATE/SCALE): con arrastre. */
enum class Tool {
  POINT,
  LINE,
  POLYLINE,
  POLYGON,
  SELECT,
  TRANSLATE,
  ROTATE,
  SCALE
};

/* Estado completo del editor en un solo lugar (sin variables globales).
   Vive dentro del AppContext y se pasa a las funciones tool*. */
struct EditorState {
  Tool tool;                /* herramienta actualmente activa */
  std::vector<Vec2> pending; /* vertices acumulados de la figura en curso
                                (LINE/POLYLINE/POLYGON antes de finalizar) */
  Color currentOutline;     /* color de contorno para nuevas figuras */
  Color currentFill;        /* color de relleno para nuevas figuras */
  bool filled;              /* relleno activo para nuevas figuras */
  int selectedIndex;        /* indice de la figura seleccionada, -1 = ninguna */
  int colorTarget;          /* a que aplica la paleta: 0 = contorno, 1 = relleno */
  /* Estado del arrastre en curso (para transformaciones) */
  bool dragging;            /* true mientras se arrastra una figura */
  Vec2 dragLast;            /* ultima posicion del mouse, para calcular el delta */
};

/* Crea el estado inicial por defecto (herramienta Punto, contorno blanco,
   sin seleccion, sin relleno). */
EditorState stateInit();

/* Maneja un CLICK simple en el canvas (no hubo arrastre). Segun la
   herramienta: agrega un punto, acumula un vertice, cierra una linea de
   2 puntos, o selecciona la figura bajo el cursor. */
void toolHandleClick(EditorState &state, Scene &sc, Vec2 pos);

/* Inicia un ARRASTRE en el canvas. Para herramientas de transformacion:
   si no hay nada seleccionado, intenta seleccionar bajo el cursor. */
void toolHandleDragStart(EditorState &state, Scene &sc, Vec2 pos);

/* Actualiza el arrastre en cada movimiento del mouse: construye la matriz
   adecuada (traslacion/rotacion/escala) a partir del delta y la aplica a
   la figura seleccionada. */
void toolHandleDragMove(EditorState &state, Scene &sc, Vec2 pos);

/* Finaliza el arrastre (dragging = false). */
void toolHandleDragEnd(EditorState &state);

/* Despachador central de acciones: el unico punto por el que pasan todas
   las ordenes, vengan de un boton o de una tecla. Ver enum Action. */
void toolHandleAction(EditorState &state, Scene &sc, Action a);

/* [Enter] Finaliza la figura en curso: convierte los vertices 'pending'
   en una polilinea o poligono definitivo y lo agrega a la escena. */
void toolFinishShape(EditorState &state, Scene &sc);

/* [Esc] Cancela la figura en curso: descarta los vertices 'pending'. */
void toolCancelShape(EditorState &state);

/* Aplica el color c al contorno o al relleno (segun colorTarget), tanto
   al estado (futuras figuras) como a la figura seleccionada. */
void toolApplyColor(EditorState &state, Scene &sc, const Color &c);

/* Dibuja la PREVIEW de la figura en construccion: los segmentos ya fijados
   mas un segmento "elastico" desde el ultimo vertice hasta el cursor. */
void toolDrawPending(const EditorState &state, Vec2 mousePos);

/* Traduce una Tool a su Action equivalente (sirve para resaltar el boton
   de la herramienta activa en la toolbar). */
Action toolToAction(Tool t);

/* Carga una escena de demostracion con 11 objetos variados (puntos,
   lineas, polilineas y poligonos ya transformados) para mostrar todas las
   capacidades de un vistazo. */
void buildDemoScene(Scene &sc);
