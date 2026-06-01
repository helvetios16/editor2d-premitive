#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include "scene.h"
#include <cfloat>
#include <cmath>

/* ------------------------------------------------------------------ */
/* Utilidades internas                                                  */
/* ------------------------------------------------------------------ */

/* Distancia minima de un punto P al segmento AB.
   Proyecta P sobre la recta AB y recorta el parametro t a [0,1] para
   quedarse dentro del segmento (no de la recta infinita). Si A==B,
   devuelve la distancia a ese punto. Base del hit-test de lineas. */
static float distToSegment(Vec2 p, Vec2 a, Vec2 b) {
  float abx = b.x - a.x, aby = b.y - a.y;
  float len2 = abx * abx + aby * aby;
  if (len2 < 1e-6f) {
    float dx = p.x - a.x, dy = p.y - a.y;
    return sqrtf(dx * dx + dy * dy);
  }
  float t = ((p.x - a.x) * abx + (p.y - a.y) * aby) / len2;
  if (t < 0.0f)
    t = 0.0f;
  if (t > 1.0f)
    t = 1.0f;
  float cx = a.x + t * abx, cy = a.y + t * aby;
  float dx = p.x - cx, dy = p.y - cy;
  return sqrtf(dx * dx + dy * dy);
}

/* Prueba "punto dentro de poligono" por ray casting.
   Lanza un rayo horizontal desde P y cuenta cuantas aristas cruza; un
   numero IMPAR de cruces significa que el punto esta dentro. Permite
   seleccionar un poligono relleno haciendo click en su interior. */
static bool pointInPolygon(Vec2 p, const std::vector<Vec2> &verts) {
  int n = (int)verts.size();
  int crossings = 0;
  for (int i = 0; i < n; i++) {
    Vec2 a = verts[i], b = verts[(i + 1) % n];
    if ((a.y <= p.y && b.y > p.y) || (b.y <= p.y && a.y > p.y)) {
      float xi = a.x + (p.y - a.y) / (b.y - a.y) * (b.x - a.x);
      if (p.x < xi)
        crossings++;
    }
  }
  return (crossings & 1) != 0;
}

/* Distancia minima del cursor a una figura, segun su tipo (para hit-test):
     POINT    -> distancia euclidiana al unico vertice
     LINE     -> distancia al segmento
     POLYLINE -> minima distancia a cualquiera de sus segmentos
     POLYGON  -> 0 si esta relleno y el cursor cae dentro; si no, distancia
                 a la arista mas cercana.
   scenePick usa esta funcion para elegir la figura mas cercana. */
static float shapeDistance(const Shape &s, Vec2 cursor) {
  if (s.vertices.empty())
    return FLT_MAX;

  switch (s.type) {
  case ShapeType::POINT: {
    float dx = cursor.x - s.vertices[0].x;
    float dy = cursor.y - s.vertices[0].y;
    return sqrtf(dx * dx + dy * dy);
  }
  case ShapeType::LINE: {
    if (s.vertices.size() < 2)
      return FLT_MAX;
    return distToSegment(cursor, s.vertices[0], s.vertices[1]);
  }
  case ShapeType::POLYLINE: {
    float best = FLT_MAX;
    for (int i = 0; i + 1 < (int)s.vertices.size(); i++) {
      float d = distToSegment(cursor, s.vertices[i], s.vertices[i + 1]);
      if (d < best)
        best = d;
    }
    return best;
  }
  case ShapeType::POLYGON: {
    int n = (int)s.vertices.size();
    if (n < 2)
      return FLT_MAX;
    /* Si tiene relleno y el cursor esta dentro, distancia cero */
    if (s.filled && pointInPolygon(cursor, s.vertices))
      return 0.0f;
    float best = FLT_MAX;
    for (int i = 0; i < n; i++) {
      float d = distToSegment(cursor, s.vertices[i], s.vertices[(i + 1) % n]);
      if (d < best)
        best = d;
    }
    return best;
  }
  }
  return FLT_MAX;
}

/* ------------------------------------------------------------------ */
/* Renderizado de una figura individual                                 */
/* ------------------------------------------------------------------ */

/* Dibuja UNA figura con la primitiva OpenGL adecuada a su tipo.
   Si esta seleccionada, usa contorno amarillo, linea mas gruesa y marca
   cada vertice con un punto. Para POLYGON relleno, primero pinta el
   interior (GL_POLYGON) y luego el contorno (GL_LINE_LOOP). */
static void drawShape(const Shape &s) {
  if (s.vertices.empty())
    return;

  /* Las figuras seleccionadas se resaltan en amarillo y mas gruesas */
  Color oc = s.selected ? Color{1.0f, 1.0f, 0.0f} : s.outline;
  float lw = s.selected ? 3.0f : 1.5f;

  switch (s.type) {
  case ShapeType::POINT:
    glPointSize(s.selected ? 12.0f : 7.0f);
    glColor3f(oc.r, oc.g, oc.b);
    glBegin(GL_POINTS);
    glVertex2f(s.vertices[0].x, s.vertices[0].y);
    glEnd();
    glPointSize(1.0f);
    break;

  case ShapeType::LINE:
    glLineWidth(lw);
    glColor3f(oc.r, oc.g, oc.b);
    glBegin(GL_LINES);
    glVertex2f(s.vertices[0].x, s.vertices[0].y);
    glVertex2f(s.vertices[1].x, s.vertices[1].y);
    glEnd();
    glLineWidth(1.0f);
    break;

  case ShapeType::POLYLINE:
    glLineWidth(lw);
    glColor3f(oc.r, oc.g, oc.b);
    glBegin(GL_LINE_STRIP);
    for (const auto &v : s.vertices)
      glVertex2f(v.x, v.y);
    glEnd();
    glLineWidth(1.0f);
    break;

  case ShapeType::POLYGON:
    if (s.filled && s.vertices.size() >= 3) {
      glColor3f(s.fill.r, s.fill.g, s.fill.b);
      glBegin(GL_POLYGON);
      for (const auto &v : s.vertices)
        glVertex2f(v.x, v.y);
      glEnd();
    }
    glLineWidth(lw);
    glColor3f(oc.r, oc.g, oc.b);
    glBegin(GL_LINE_LOOP);
    for (const auto &v : s.vertices)
      glVertex2f(v.x, v.y);
    glEnd();
    glLineWidth(1.0f);
    break;
  }

  /* Marcas de vertices para figura seleccionada */
  if (s.selected) {
    glPointSize(6.0f);
    glColor3f(1.0f, 0.8f, 0.0f);
    glBegin(GL_POINTS);
    for (const auto &v : s.vertices)
      glVertex2f(v.x, v.y);
    glEnd();
    glPointSize(1.0f);
  }
}

/* ------------------------------------------------------------------ */
/* API publica de Scene                                                 */
/* ------------------------------------------------------------------ */

void sceneAdd(Scene &sc, const Shape &s) { sc.shapes.push_back(s); }

/* Recorre todas las figuras y devuelve el indice de la mas cercana al
   cursor, siempre que su distancia sea menor que el umbral (10 px).
   Devuelve -1 si ninguna esta lo bastante cerca. */
int scenePick(const Scene &sc, Vec2 cursor) {
  const float THRESHOLD = 10.0f; /* radio de tolerancia en pixeles */
  int best = -1;
  float bestDist = THRESHOLD;
  for (int i = 0; i < (int)sc.shapes.size(); i++) {
    float d = shapeDistance(sc.shapes[i], cursor);
    if (d < bestDist) {
      bestDist = d;
      best = i;
    }
  }
  return best;
}

/* Elimina la primera figura marcada como seleccionada y termina. */
void sceneDeleteSelected(Scene &sc) {
  auto &v = sc.shapes;
  for (int i = (int)v.size() - 1; i >= 0; i--) {
    if (v[i].selected) {
      v.erase(v.begin() + i);
      return; /* elimina solo la primera encontrada */
    }
  }
}

void sceneClearSelection(Scene &sc) {
  for (auto &s : sc.shapes)
    s.selected = false;
}

/* Dibuja toda la escena: una pasada por cada figura. */
void sceneRender(const Scene &sc) {
  for (const auto &s : sc.shapes)
    drawShape(s);
}

/* Centroide = promedio de los vertices. Es el pivote sobre el que rotan y
   escalan las herramientas (ver toolHandleDragMove). */
Vec2 shapeCentroid(const Shape &s) {
  if (s.vertices.empty())
    return {0, 0};
  float sx = 0, sy = 0;
  for (const auto &v : s.vertices) {
    sx += v.x;
    sy += v.y;
  }
  float n = (float)s.vertices.size();
  return {sx / n, sy / n};
}

/* "Hornea" la transformacion: aplica M a cada vertice y sobrescribe el
   resultado. La figura no recuerda M; queda ya en su posicion final. */
void shapeTransform(Shape &s, const Mat3 &M) {
  for (auto &v : s.vertices)
    v = mat3Apply(M, v);
}
