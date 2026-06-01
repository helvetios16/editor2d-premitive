#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include "fileio.h"
#include "mat3.h"
#include "tools.h"
#include <cmath>

/* ------------------------------------------------------------------ */
/* Helpers internos                                                     */
/* ------------------------------------------------------------------ */

/* Fabrica de figuras: crea un Shape del tipo dado con los vertices
   indicados, copiando el color y relleno actuales del editor. Centraliza
   la creacion para que toda figura nueva herede el estilo vigente. */
static Shape makeShape(ShapeType type, const std::vector<Vec2> &verts,
                       const EditorState &st) {
  Shape s;
  s.type = type;
  s.vertices = verts;
  s.outline = st.currentOutline;
  s.fill = st.currentFill;
  s.filled = st.filled;
  s.selected = false;
  return s;
}

/* ------------------------------------------------------------------ */
/* Inicializacion                                                       */
/* ------------------------------------------------------------------ */

/* Valores iniciales del editor al arrancar: herramienta Punto, contorno
   blanco, relleno azulado desactivado, sin figura seleccionada. */
EditorState stateInit() {
  EditorState st;
  st.tool = Tool::POINT;
  st.currentOutline = {1.0f, 1.0f, 1.0f};
  st.currentFill = {0.3f, 0.5f, 0.8f};
  st.filled = false;
  st.selectedIndex = -1;
  st.colorTarget = 0;
  st.dragging = false;
  st.dragLast = {0, 0};
  return st;
}

/* ------------------------------------------------------------------ */
/* Acciones de herramienta                                              */
/* ------------------------------------------------------------------ */

/* Click simple en el canvas. El comportamiento depende de la herramienta:
   POINT crea la figura al instante; LINE necesita 2 clicks; POLYLINE y
   POLYGON acumulan vertices en 'pending' hasta que el usuario pulse Enter;
   SELECT busca y marca la figura bajo el cursor. */
void toolHandleClick(EditorState &state, Scene &sc, Vec2 pos) {
  switch (state.tool) {
  case Tool::POINT:
    sceneAdd(sc, makeShape(ShapeType::POINT, {pos}, state));
    break;

  case Tool::LINE:
    state.pending.push_back(pos);
    if (state.pending.size() == 2) {
      sceneAdd(sc, makeShape(ShapeType::LINE, state.pending, state));
      state.pending.clear();
    }
    break;

  case Tool::POLYLINE:
  case Tool::POLYGON:
    state.pending.push_back(pos);
    break;

  case Tool::SELECT: {
    sceneClearSelection(sc);
    int idx = scenePick(sc, pos);
    state.selectedIndex = idx;
    if (idx >= 0)
      sc.shapes[idx].selected = true;
    break;
  }

  default:
    /* Las herramientas de transformacion usan drag, no click simple */
    break;
  }
}

/* Comienza un arrastre. Solo tiene efecto con las herramientas de
   seleccion/transformacion: si aun no hay figura seleccionada, intenta
   seleccionar la que este bajo el punto inicial. Guarda dragLast como
   referencia para calcular el delta en cada movimiento. */
void toolHandleDragStart(EditorState &state, Scene &sc, Vec2 pos) {
  /* Para transform tools: si no hay seleccion, intentar seleccionar */
  if (state.tool == Tool::SELECT || state.tool == Tool::TRANSLATE ||
      state.tool == Tool::ROTATE || state.tool == Tool::SCALE) {
    if (state.selectedIndex < 0) {
      sceneClearSelection(sc);
      int idx = scenePick(sc, pos);
      state.selectedIndex = idx;
      if (idx >= 0)
        sc.shapes[idx].selected = true;
    }
    state.dragging = (state.selectedIndex >= 0);
  }
  state.dragLast = pos;
}

/* Cada paso del arrastre transforma la figura seleccionada segun la
   herramienta activa, usando el delta (cuanto se movio el mouse):
     SELECT/TRANSLATE -> traslada por (dx, dy)
     ROTATE           -> el desplazamiento horizontal define el angulo
     SCALE            -> el desplazamiento vertical define el factor
   Rotacion y escala usan el patron T(c)*M*T(-c) para operar respecto al
   centroide de la figura, no respecto al origen. */
void toolHandleDragMove(EditorState &state, Scene &sc, Vec2 pos) {
  if (!state.dragging)
    return;
  if (state.selectedIndex < 0 || state.selectedIndex >= (int)sc.shapes.size())
    return;

  Shape &s = sc.shapes[state.selectedIndex];
  Vec2 delta = {pos.x - state.dragLast.x, pos.y - state.dragLast.y};

  switch (state.tool) {
  case Tool::SELECT:
  case Tool::TRANSLATE: {
    /* Traslacion directa por delta del mouse */
    Mat3 M = mat3Translate(delta.x, delta.y);
    shapeTransform(s, M);
    break;
  }
  case Tool::ROTATE: {
    /* Movimiento horizontal → angulo de rotacion (rad) */
    float angle = delta.x * 0.025f;
    Vec2 c = shapeCentroid(s);
    Mat3 M = mat3Mul(mat3Translate(c.x, c.y),
                     mat3Mul(mat3Rotate(angle), mat3Translate(-c.x, -c.y)));
    shapeTransform(s, M);
    break;
  }
  case Tool::SCALE: {
    /* Movimiento vertical: arriba escala mas, abajo escala menos */
    float factor = 1.0f - delta.y * 0.008f;
    if (factor < 0.01f)
      factor = 0.01f;
    Vec2 c = shapeCentroid(s);
    Mat3 M =
        mat3Mul(mat3Translate(c.x, c.y),
                mat3Mul(mat3Scale(factor, factor), mat3Translate(-c.x, -c.y)));
    shapeTransform(s, M);
    break;
  }
  default:
    break;
  }
  state.dragLast = pos;
}

/* Termina el arrastre actual. */
void toolHandleDragEnd(EditorState &state) { state.dragging = false; }

/* ------------------------------------------------------------------ */
/* Finalizar / cancelar figura en curso                                 */
/* ------------------------------------------------------------------ */

/* [Enter] Cierra la figura en construccion: si hay al menos 2 vertices
   acumulados, los convierte en un POLYGON (si la herramienta es Poligono)
   o en una POLYLINE, y limpia 'pending'. */
void toolFinishShape(EditorState &state, Scene &sc) {
  if (state.pending.size() >= 2) {
    ShapeType type = (state.tool == Tool::POLYGON) ? ShapeType::POLYGON
                                                   : ShapeType::POLYLINE;
    sceneAdd(sc, makeShape(type, state.pending, state));
  }
  state.pending.clear();
}

/* [Esc] Descarta los vertices de la figura en curso. */
void toolCancelShape(EditorState &state) { state.pending.clear(); }

/* ------------------------------------------------------------------ */
/* Aplicar color de paleta                                              */
/* ------------------------------------------------------------------ */

/* Aplica un color de la paleta. colorTarget decide si afecta al contorno
   (0) o al relleno (1). Cambia el color "actual" del editor (para futuras
   figuras) y, si hay una figura seleccionada, tambien la suya. */
void toolApplyColor(EditorState &state, Scene &sc, const Color &c) {
  if (state.colorTarget == 0) {
    state.currentOutline = c;
    if (state.selectedIndex >= 0)
      sc.shapes[state.selectedIndex].outline = c;
  } else {
    state.currentFill = c;
    if (state.selectedIndex >= 0)
      sc.shapes[state.selectedIndex].fill = c;
  }
}

/* ------------------------------------------------------------------ */
/* Manejo de acciones generales                                         */
/* ------------------------------------------------------------------ */

/* Despachador central: ejecuta cualquier Action, venga de un boton de la
   toolbar o de una tecla. Primero detecta si la accion es un color de la
   paleta (rango COLOR_0..COLOR_9) mediante aritmetica de enteros; si no,
   resuelve el resto con un switch (cambio de herramienta, relleno, control
   de figura, archivo, escena...). Es el punto unico de entrada de ordenes. */
void toolHandleAction(EditorState &state, Scene &sc, Action a) {
  /* Acciones de color de paleta */
  static const Color PALETTE[10] = {
      {1.0f, 0.0f, 0.0f}, {0.0f, 0.8f, 0.0f}, {0.2f, 0.4f, 1.0f},
      {1.0f, 1.0f, 0.0f}, {1.0f, 0.5f, 0.0f}, {0.7f, 0.0f, 0.7f},
      {0.0f, 0.9f, 0.9f}, {1.0f, 0.4f, 0.7f}, {0.6f, 0.4f, 0.2f},
      {1.0f, 1.0f, 1.0f},
  };

  int ai = static_cast<int>(a);
  int c0 = static_cast<int>(Action::COLOR_0);
  int c9 = static_cast<int>(Action::COLOR_9);
  if (ai >= c0 && ai <= c9) {
    toolApplyColor(state, sc, PALETTE[ai - c0]);
    return;
  }

  switch (a) {
  /* Cambio de herramienta */
  case Action::TOOL_POINT:
    toolCancelShape(state);
    state.tool = Tool::POINT;
    break;
  case Action::TOOL_LINE:
    toolCancelShape(state);
    state.tool = Tool::LINE;
    break;
  case Action::TOOL_POLYLINE:
    toolCancelShape(state);
    state.tool = Tool::POLYLINE;
    break;
  case Action::TOOL_POLYGON:
    toolCancelShape(state);
    state.tool = Tool::POLYGON;
    break;
  case Action::TOOL_SELECT:
    toolCancelShape(state);
    state.tool = Tool::SELECT;
    break;
  case Action::TOOL_TRANSLATE:
    state.tool = Tool::TRANSLATE;
    break;
  case Action::TOOL_ROTATE:
    state.tool = Tool::ROTATE;
    break;
  case Action::TOOL_SCALE:
    state.tool = Tool::SCALE;
    break;

  /* Propiedades */
  case Action::TOGGLE_FILL:
    state.filled = !state.filled;
    if (state.selectedIndex >= 0)
      sc.shapes[state.selectedIndex].filled = state.filled;
    break;
  case Action::TARGET_OUTLINE:
    state.colorTarget = 0;
    break;
  case Action::TARGET_FILL:
    state.colorTarget = 1;
    break;

  /* Control de figura en curso */
  case Action::FINISH_SHAPE:
    toolFinishShape(state, sc);
    break;
  case Action::CANCEL_SHAPE:
    toolCancelShape(state);
    sceneClearSelection(sc);
    state.selectedIndex = -1;
    break;

  /* Borrar seleccionado */
  case Action::DELETE_SELECTED:
    sceneDeleteSelected(sc);
    state.selectedIndex = -1;
    break;

  /* Archivo */
  case Action::FILE_SAVE:
    saveSceneJson(sc, "escena.json");
    break;
  case Action::FILE_LOAD:
    loadSceneJson(sc, "escena.json");
    state.selectedIndex = -1;
    toolCancelShape(state);
    break;

  /* Escena */
  case Action::SCENE_CLEAR:
    sc.shapes.clear();
    state.selectedIndex = -1;
    toolCancelShape(state);
    break;
  case Action::SCENE_DEMO:
    sc.shapes.clear();
    state.selectedIndex = -1;
    toolCancelShape(state);
    buildDemoScene(sc);
    break;

  default:
    break;
  }
}

/* ------------------------------------------------------------------ */
/* Preview de figura en curso                                           */
/* ------------------------------------------------------------------ */

/* Dibuja la previsualizacion de la figura en construccion: los segmentos
   ya fijados (gris), un segmento "elastico" desde el ultimo vertice hasta
   el cursor, el cierre tentativo si es un poligono, y un punto en cada
   vertice pendiente. No modifica la escena; es solo guia visual. */
void toolDrawPending(const EditorState &state, Vec2 mousePos) {
  if (state.pending.empty())
    return;

  glColor3f(0.6f, 0.6f, 0.6f);
  glLineWidth(1.0f);

  /* Segmentos ya fijados */
  if (state.pending.size() > 1) {
    glBegin(GL_LINE_STRIP);
    for (const auto &v : state.pending)
      glVertex2f(v.x, v.y);
    glEnd();
  }

  /* Segmento dinamico hasta el cursor */
  glBegin(GL_LINES);
  glVertex2f(state.pending.back().x, state.pending.back().y);
  glVertex2f(mousePos.x, mousePos.y);
  glEnd();

  /* Cierre previo si es poligono */
  if (state.tool == Tool::POLYGON && state.pending.size() >= 2) {
    glBegin(GL_LINES);
    glVertex2f(mousePos.x, mousePos.y);
    glVertex2f(state.pending.front().x, state.pending.front().y);
    glEnd();
  }

  /* Marcas de vertices pendientes */
  glPointSize(5.0f);
  glBegin(GL_POINTS);
  for (const auto &v : state.pending)
    glVertex2f(v.x, v.y);
  glEnd();
  glPointSize(1.0f);
}

/* Traduce la herramienta activa a su Action (para resaltar su boton). */
Action toolToAction(Tool t) {
  switch (t) {
  case Tool::POINT:
    return Action::TOOL_POINT;
  case Tool::LINE:
    return Action::TOOL_LINE;
  case Tool::POLYLINE:
    return Action::TOOL_POLYLINE;
  case Tool::POLYGON:
    return Action::TOOL_POLYGON;
  case Tool::SELECT:
    return Action::TOOL_SELECT;
  case Tool::TRANSLATE:
    return Action::TOOL_TRANSLATE;
  case Tool::ROTATE:
    return Action::TOOL_ROTATE;
  case Tool::SCALE:
    return Action::TOOL_SCALE;
  }
  return Action::NONE;
}

/* ------------------------------------------------------------------ */
/* Escena de demostracion (>=10 objetos)                                */
/* ------------------------------------------------------------------ */

/* Escena de demostracion: agrega 11 figuras de ejemplo que cubren todas
   las primitivas y muestran transformaciones ya aplicadas (un triangulo
   escalado, un rectangulo rotado, poligonos generados con trigonometria).
   Util para probar el editor sin tener que dibujar a mano. */
void buildDemoScene(Scene &sc) {
  /* 1. Punto */
  sceneAdd(sc, {ShapeType::POINT, {{500, 100}}, {1, 0, 0}, {}, false, false});

  /* 2. Punto */
  sceneAdd(sc, {ShapeType::POINT, {{560, 100}}, {0, 1, 0}, {}, false, false});

  /* 3. Linea horizontal */
  sceneAdd(sc, {ShapeType::LINE,
                {{200, 150}, {600, 150}},
                {0, 0.8f, 1},
                {},
                false,
                false});

  /* 4. Linea diagonal */
  sceneAdd(sc, {ShapeType::LINE,
                {{200, 200}, {400, 350}},
                {1, 0.5f, 0},
                {},
                false,
                false});

  /* 5. Polilínea en zigzag */
  sceneAdd(sc, {ShapeType::POLYLINE,
                {{250, 400}, {300, 350}, {350, 400}, {400, 350}, {450, 400}},
                {0.8f, 0.2f, 0.8f},
                {},
                false,
                false});

  /* 6. Triangulo rojo relleno */
  {
    Shape s;
    s.type = ShapeType::POLYGON;
    s.vertices = {{500, 180}, {580, 320}, {420, 320}};
    s.outline = {0.8f, 0.1f, 0.1f};
    s.fill = {0.9f, 0.3f, 0.3f};
    s.filled = true;
    s.selected = false;
    /* Escalamos un poco para demostrar transformacion */
    Vec2 c = shapeCentroid(s);
    Mat3 M = mat3Mul(mat3Translate(c.x, c.y),
                     mat3Mul(mat3Scale(1.3f, 1.3f), mat3Translate(-c.x, -c.y)));
    shapeTransform(s, M);
    sceneAdd(sc, s);
  }

  /* 7. Rectangulo azul relleno */
  {
    Shape s;
    s.type = ShapeType::POLYGON;
    s.vertices = {{230, 230}, {390, 230}, {390, 310}, {230, 310}};
    s.outline = {0.2f, 0.3f, 0.9f};
    s.fill = {0.3f, 0.4f, 0.95f};
    s.filled = true;
    s.selected = false;
    /* Rotamos 15 grados para demostrar rotacion */
    Vec2 c = shapeCentroid(s);
    Mat3 M = mat3Mul(mat3Translate(c.x, c.y),
                     mat3Mul(mat3Rotate(0.26f), /* ~15 grados */
                             mat3Translate(-c.x, -c.y)));
    shapeTransform(s, M);
    sceneAdd(sc, s);
  }

  /* 8. Pentagono verde relleno */
  {
    Shape s;
    s.type = ShapeType::POLYGON;
    float cx = 650, cy = 350, r = 70;
    for (int i = 0; i < 5; i++) {
      float ang = -3.14159f / 2.0f + i * 2.0f * 3.14159f / 5.0f;
      s.vertices.push_back({cx + r * cosf(ang), cy + r * sinf(ang)});
    }
    s.outline = {0.1f, 0.7f, 0.1f};
    s.fill = {0.2f, 0.85f, 0.2f};
    s.filled = true;
    s.selected = false;
    sceneAdd(sc, s);
  }

  /* 9. Hexagono naranja sin relleno */
  {
    Shape s;
    s.type = ShapeType::POLYGON;
    float cx = 700, cy = 200, r = 50;
    for (int i = 0; i < 6; i++) {
      float ang = i * 2.0f * 3.14159f / 6.0f;
      s.vertices.push_back({cx + r * cosf(ang), cy + r * sinf(ang)});
    }
    s.outline = {1.0f, 0.55f, 0.0f};
    s.fill = {};
    s.filled = false;
    s.selected = false;
    sceneAdd(sc, s);
  }

  /* 10. Polilínea en forma de W */
  sceneAdd(sc, {ShapeType::POLYLINE,
                {{160, 460}, {200, 520}, {250, 470}, {300, 520}, {340, 460}},
                {0.9f, 0.9f, 0.3f},
                {},
                false,
                false});

  /* 11. Triangulo cyan trasladado */
  {
    Shape s;
    s.type = ShapeType::POLYGON;
    s.vertices = {{600, 420}, {650, 480}, {550, 480}};
    s.outline = {0.0f, 0.9f, 0.9f};
    s.fill = {0.1f, 0.7f, 0.7f};
    s.filled = true;
    s.selected = false;
    Mat3 M = mat3Translate(20.0f, 30.0f);
    shapeTransform(s, M);
    sceneAdd(sc, s);
  }
}
