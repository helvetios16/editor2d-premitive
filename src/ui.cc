#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include "ui.h"

/* ------------------------------------------------------------------ */
/* Paleta de colores predefinidos (10 entradas)                        */
/* ------------------------------------------------------------------ */
static const Color PALETTE[10] = {
    {1.0f, 0.0f, 0.0f}, /* 0 rojo      */
    {0.0f, 0.8f, 0.0f}, /* 1 verde     */
    {0.2f, 0.4f, 1.0f}, /* 2 azul      */
    {1.0f, 1.0f, 0.0f}, /* 3 amarillo  */
    {1.0f, 0.5f, 0.0f}, /* 4 naranja   */
    {0.7f, 0.0f, 0.7f}, /* 5 morado    */
    {0.0f, 0.9f, 0.9f}, /* 6 cyan      */
    {1.0f, 0.4f, 0.7f}, /* 7 rosa      */
    {0.6f, 0.4f, 0.2f}, /* 8 marron    */
    {1.0f, 1.0f, 1.0f}, /* 9 blanco    */
};

/* ------------------------------------------------------------------ */
/* Utilidades de dibujo                                                 */
/* ------------------------------------------------------------------ */

/* Dibuja un rectangulo relleno */
static void fillRect(float x, float y, float w, float h) {
  glBegin(GL_QUADS);
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
  glEnd();
}

/* Dibuja solo el borde de un rectangulo */
static void strokeRect(float x, float y, float w, float h) {
  glBegin(GL_LINE_LOOP);
  glVertex2f(x, y);
  glVertex2f(x + w, y);
  glVertex2f(x + w, y + h);
  glVertex2f(x, y + h);
  glEnd();
}

void drawText(float x, float y, const std::string &text) {
  glRasterPos2f(x, y);
  for (char c : text)
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
}

/* ------------------------------------------------------------------ */
/* Boton                                                                */
/* ------------------------------------------------------------------ */

bool buttonContains(const Button &b, double mx, double my) {
  return mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h;
}

void drawButton(const Button &b, bool active) {
  /* Fondo */
  if (active)
    glColor3f(0.30f, 0.55f, 0.85f);
  else
    glColor3f(0.22f, 0.22f, 0.28f);
  fillRect(b.x, b.y, b.w, b.h);

  /* Borde */
  glColor3f(active ? 0.6f : 0.40f, active ? 0.8f : 0.40f,
            active ? 1.0f : 0.50f);
  strokeRect(b.x, b.y, b.w, b.h);

  /* Texto centrado verticalmente */
  glColor3f(1.0f, 1.0f, 1.0f);
  drawText(b.x + 5.0f, b.y + b.h * 0.5f + 4.0f, b.label);
}

/* ------------------------------------------------------------------ */
/* Construccion del toolbar                                             */
/* ------------------------------------------------------------------ */

/* Agrega un boton a la lista con posicion/tamaño y accion */
static void addBtn(std::vector<Button> &btns, float x, float y, float w,
                   float h, const char *label, Action action) {
  btns.push_back({x, y, w, h, label, action});
}

/* Construye, una sola vez, todos los botones del panel con sus posiciones.
   Usa un cursor vertical 'y' y el helper nextY() para ir apilando filas:
   herramientas de dibujo, transformaciones, relleno, selector de objetivo
   de color, la paleta (2x5) y los botones de archivo/escena. */
void buildToolbar(std::vector<Button> &btns) {
  btns.clear();

  /* Ritmo vertical unico: un solo margen (PAD), una sola separacion entre
     botones (GAP) y un hueco fijo (HEADER) reservado encima de cada grupo
     para su etiqueta de seccion (que drawToolbar dibuja anclada al primer
     boton del grupo). Asi la separacion es uniforme y no hay solapes. */
  const float PAD = 8.0f;                 /* margen izq/der del panel       */
  const float X = PAD;                    /* x comun de los botones a ancho  */
  const float W = TOOLBAR_W - 2.0f * PAD; /* ancho util (124)               */
  const float BH = 22.0f;                 /* alto de boton                  */
  const float GAP = 4.0f;                 /* separacion entre botones        */
  const float HEADER = 18.0f;             /* hueco para el titulo de seccion */
  float y = 24.0f;                        /* debajo del titulo "EDITOR 2D"   */

  /* Apila un boton de ancho completo y avanza el cursor una fila. */
  auto row = [&]() -> float {
    float cur = y;
    y += BH + GAP;
    return cur;
  };
  /* Reserva el hueco de la etiqueta antes del primer boton de un grupo. */
  auto section = [&]() { y += HEADER; };

  /* --- Dibujo --- */
  section();
  addBtn(btns, X, row(), W, BH, "Punto    [P]", Action::TOOL_POINT);
  addBtn(btns, X, row(), W, BH, "Linea    [L]", Action::TOOL_LINE);
  addBtn(btns, X, row(), W, BH, "Polilnea [O]", Action::TOOL_POLYLINE);
  addBtn(btns, X, row(), W, BH, "Poligono [G]", Action::TOOL_POLYGON);

  /* --- Transformar --- */
  section();
  addBtn(btns, X, row(), W, BH, "Selec    [S]", Action::TOOL_SELECT);
  addBtn(btns, X, row(), W, BH, "Trasladar[T]", Action::TOOL_TRANSLATE);
  addBtn(btns, X, row(), W, BH, "Rotar    [R]", Action::TOOL_ROTATE);
  addBtn(btns, X, row(), W, BH, "Escalar  [E]", Action::TOOL_SCALE);

  /* --- Color / Relleno --- */
  section();
  addBtn(btns, X, row(), W, BH, "Relleno  [F]", Action::TOGGLE_FILL);

  /* Target de color: dos botones de medio ancho en la misma fila. */
  float hw = (W - GAP) / 2.0f;
  float ty = y;
  y += BH + GAP;
  addBtn(btns, X, ty, hw, BH, "Contorno", Action::TARGET_OUTLINE);
  addBtn(btns, X + hw + GAP, ty, hw, BH, "Relleno", Action::TARGET_FILL);

  /* Paleta de colores (2 filas x 5), con la misma GAP que el resto. */
  float sw = (W - 4.0f * GAP) / 5.0f; /* ancho de cada swatch */
  float sh = 16.0f;
  float py0 = y;
  for (int r = 0; r < 2; r++) {
    for (int col = 0; col < 5; col++) {
      int idx = r * 5 + col;
      float sx = X + col * (sw + GAP);
      float sy = py0 + r * (sh + GAP);
      Action ca = static_cast<Action>(static_cast<int>(Action::COLOR_0) + idx);
      btns.push_back({sx, sy, sw, sh, "", ca});
    }
  }
  y = py0 + 2.0f * (sh + GAP);
  y += 32.0f; /* hueco para el preview de color que dibuja drawToolbar */

  /* --- Archivo / escena --- */
  section();
  addBtn(btns, X, row(), W, BH, "Guardar [^S]", Action::FILE_SAVE);
  addBtn(btns, X, row(), W, BH, "Abrir   [^O]", Action::FILE_LOAD);
  addBtn(btns, X, row(), W, BH, "Limpiar [Del]", Action::SCENE_CLEAR);
  addBtn(btns, X, row(), W, BH, "Demo    [D]", Action::SCENE_DEMO);
}

/* ------------------------------------------------------------------ */
/* Dibujo del panel completo                                            */
/* ------------------------------------------------------------------ */

/* Dibuja el panel completo cada frame: fondo, separador, titulo, los
   botones (resaltando el activo segun la herramienta/estado), los swatches
   de color de la paleta y las muestras del color actual de contorno/relleno. */
void drawToolbar(const std::vector<Button> &btns, Action activeTool,
                 bool filled, int colorTarget, Color oc, Color fc) {
  /* Fondo del panel */
  glColor3f(0.13f, 0.13f, 0.18f);
  fillRect(0, 0, (float)TOOLBAR_W, 10000.0f); /* cubre toda la altura */

  /* Linea separadora derecha */
  glColor3f(0.35f, 0.35f, 0.45f);
  glBegin(GL_LINES);
  glVertex2f((float)TOOLBAR_W, 0);
  glVertex2f((float)TOOLBAR_W, 10000.0f);
  glEnd();

  /* Titulo */
  glColor3f(0.8f, 0.8f, 1.0f);
  drawText(8.0f, 16.0f, "EDITOR 2D");

  /* Botones */
  for (const auto &b : btns) {
    int idx = static_cast<int>(b.action);
    int c0 = static_cast<int>(Action::COLOR_0);
    int c9 = static_cast<int>(Action::COLOR_9);

    if (idx >= c0 && idx <= c9) {
      /* Swatch de color */
      int ci = idx - c0;
      glColor3f(PALETTE[ci].r, PALETTE[ci].g, PALETTE[ci].b);
      fillRect(b.x, b.y, b.w, b.h);
      glColor3f(0.7f, 0.7f, 0.7f);
      strokeRect(b.x, b.y, b.w, b.h);
    } else {
      /* Boton normal: activo si coincide con la herramienta o el estado */
      bool active = (b.action == activeTool);
      if (b.action == Action::TOGGLE_FILL)
        active = filled;
      if (b.action == Action::TARGET_OUTLINE)
        active = (colorTarget == 0);
      if (b.action == Action::TARGET_FILL)
        active = (colorTarget == 1);
      drawButton(b, active);
    }
  }

  /* Etiquetas de secciones: se anclan al primer boton de cada grupo, justo
     en el hueco HEADER que buildToolbar reservo encima de el. Asi quedan
     siempre alineadas con el layout sin coordenadas duplicadas. */
  auto sectionLabel = [&](Action anchor, const char *txt) {
    for (const auto &b : btns) {
      if (b.action == anchor) {
        glColor3f(0.55f, 0.75f, 0.55f);
        drawText(b.x, b.y - 6.0f, txt);
        return;
      }
    }
  };
  sectionLabel(Action::TOOL_POINT, "Dibujo");
  sectionLabel(Action::TOOL_SELECT, "Transformar");
  sectionLabel(Action::TOGGLE_FILL, "Color / Relleno");
  sectionLabel(Action::FILE_SAVE, "Archivo");

  /* Preview de colores actuales bajo la paleta */
  /* Localizar la posicion Y de la paleta buscando el ultimo swatch */
  float paletteBottom = 0;
  for (const auto &b : btns) {
    int idx = static_cast<int>(b.action);
    int c9 = static_cast<int>(Action::COLOR_9);
    if (idx == c9)
      paletteBottom = b.y + b.h + 4.0f;
  }

  if (paletteBottom > 0) {
    float px = 8.0f, py = paletteBottom;
    float pw = (TOOLBAR_W - 16.0f) / 2.0f - 2.0f;
    float ph = 14.0f;

    /* Recuadro contorno */
    glColor3f(oc.r, oc.g, oc.b);
    fillRect(px, py, pw, ph);
    glColor3f(colorTarget == 0 ? 1.0f : 0.5f, colorTarget == 0 ? 1.0f : 0.5f,
              colorTarget == 0 ? 0.0f : 0.5f);
    glLineWidth(colorTarget == 0 ? 2.0f : 1.0f);
    strokeRect(px, py, pw, ph);
    glLineWidth(1.0f);

    /* Recuadro relleno */
    float fx = px + pw + 4.0f;
    glColor3f(fc.r, fc.g, fc.b);
    fillRect(fx, py, pw, ph);
    glColor3f(colorTarget == 1 ? 1.0f : 0.5f, colorTarget == 1 ? 1.0f : 0.5f,
              colorTarget == 1 ? 0.0f : 0.5f);
    glLineWidth(colorTarget == 1 ? 2.0f : 1.0f);
    strokeRect(fx, py, pw, ph);
    glLineWidth(1.0f);

    glColor3f(0.7f, 0.7f, 0.7f);
    drawText(px, py + ph + 12.0f, "Con. / Rell.");
  }
}

/* ------------------------------------------------------------------ */
/* Hit-test del toolbar                                                 */
/* ------------------------------------------------------------------ */

Action toolbarHandleClick(const std::vector<Button> &btns, double mx,
                          double my) {
  for (const auto &b : btns) {
    if (buttonContains(b, mx, my))
      return b.action;
  }
  return Action::NONE;
}
