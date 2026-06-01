#pragma once
#include "vec.h"
#include <string>
#include <vector>

/* ==================================================================
   ui.h - Interfaz del panel lateral (toolbar) y el enum Action.
   Define los botones, como dibujarlos y detectar clicks, y sobre todo el
   enum Action, que es el "idioma comun" entre la entrada (botones y
   teclado) y la logica (toolHandleAction).
   ================================================================== */

/* Ancho del panel lateral izquierdo, en pixeles de ventana. Marca la
   frontera: x < TOOLBAR_W es toolbar; x >= TOOLBAR_W es canvas. */
static const int TOOLBAR_W = 140;

/* ------------------------------------------------------------------ */
/* Acciones disponibles: el puente unico entre botones, teclas y logica */
/* ------------------------------------------------------------------ */
/* Un boton pulsado, una tecla o el cambio de herramienta producen todos
   un valor de Action, y toolHandleAction los ejecuta en un solo switch.
   Los valores COLOR_0..COLOR_9 son CONSECUTIVOS a proposito: permiten
   detectar "es un color de la paleta" con aritmetica de enteros e indexar
   directamente el arreglo de colores. */
enum class Action {
  NONE = -1,
  /* Herramientas de dibujo */
  TOOL_POINT = 0,
  TOOL_LINE,
  TOOL_POLYLINE,
  TOOL_POLYGON,
  /* Herramientas de seleccion y transformacion */
  TOOL_SELECT,
  TOOL_TRANSLATE,
  TOOL_ROTATE,
  TOOL_SCALE,
  /* Propiedades */
  TOGGLE_FILL,
  TARGET_OUTLINE, /* el siguiente color elegido afecta al contorno */
  TARGET_FILL,    /* el siguiente color elegido afecta al relleno  */
  /* Paleta de colores (10 colores predefinidos, valores consecutivos) */
  COLOR_0,
  COLOR_1,
  COLOR_2,
  COLOR_3,
  COLOR_4,
  COLOR_5,
  COLOR_6,
  COLOR_7,
  COLOR_8,
  COLOR_9,
  /* Archivo y escena */
  FILE_SAVE,
  FILE_LOAD,
  SCENE_CLEAR,
  SCENE_DEMO,
  /* Control de dibujo */
  FINISH_SHAPE,    /* Enter: finaliza polilinea/poligono */
  CANCEL_SHAPE,    /* Esc:   cancela figura en curso     */
  DELETE_SELECTED, /* Del:   elimina la figura seleccionada */
};

/* ------------------------------------------------------------------ */
/* Boton rectangular del toolbar, con su region clicable               */
/* ------------------------------------------------------------------ */
struct Button {
  float x, y, w, h;  /* rectangulo en coordenadas de ventana */
  std::string label; /* texto que se muestra encima */
  Action action;     /* que Action dispara al pulsarlo */
};

/* Retorna true si el punto (mx, my) cae dentro del rectangulo del boton. */
bool buttonContains(const Button &b, double mx, double my);

/* Dibuja un boton (fondo + borde + etiqueta). Si active=true, lo resalta
   para indicar que es la herramienta/opcion activa. */
void drawButton(const Button &b, bool active);

/* Dibuja texto en (x, y) con la fuente GLUT_BITMAP_HELVETICA_12.
   Es el unico uso de GLUT en tiempo de ejecucion. */
void drawText(float x, float y, const std::string &text);

/* Construye la lista de botones del toolbar con sus posiciones.
   Se llama UNA sola vez al iniciar la aplicacion. */
void buildToolbar(std::vector<Button> &btns);

/* Dibuja el panel lateral completo en cada frame.
   activeTool : Action de la herramienta activa (para resaltar su boton)
   filled     : si el relleno esta activado
   colorTarget: 0 = contorno, 1 = relleno (para marcar el objetivo)
   oc, fc     : colores actuales de contorno y relleno (muestras) */
void drawToolbar(const std::vector<Button> &btns, Action activeTool,
                 bool filled, int colorTarget, Color oc, Color fc);

/* Comprueba si algun boton fue clickeado en (mx, my) y retorna su Action;
   si no se pulso ninguno, retorna Action::NONE. */
Action toolbarHandleClick(const std::vector<Button> &btns, double mx,
                          double my);
