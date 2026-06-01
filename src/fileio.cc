#include "fileio.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

/* ================================================================== */
/* ESCRITURA JSON                                                       */
/* ================================================================== */

static const char *shapeTypeName(ShapeType t) {
  switch (t) {
  case ShapeType::POINT:
    return "POINT";
  case ShapeType::LINE:
    return "LINE";
  case ShapeType::POLYLINE:
    return "POLYLINE";
  case ShapeType::POLYGON:
    return "POLYGON";
  }
  return "POINT";
}

/* Escribe la escena como JSON a mano (sin libreria). Recorre cada figura
   y emite su tipo, lista de vertices, colores y bandera de relleno,
   cuidando las comas entre elementos. Retorna true si el flujo quedo OK. */
bool saveSceneJson(const Scene &sc, const std::string &path) {
  std::ofstream f(path);
  if (!f.is_open())
    return false;

  f << "{\n  \"objetos\": [\n";
  for (int i = 0; i < (int)sc.shapes.size(); i++) {
    const Shape &s = sc.shapes[i];
    f << "    {\n";
    f << "      \"tipo\": \"" << shapeTypeName(s.type) << "\",\n";

    /* Vertices */
    f << "      \"vertices\": [";
    for (int j = 0; j < (int)s.vertices.size(); j++) {
      if (j)
        f << ", ";
      f << "[" << s.vertices[j].x << ", " << s.vertices[j].y << "]";
    }
    f << "],\n";

    /* Colores */
    f << "      \"outline\": [" << s.outline.r << ", " << s.outline.g << ", "
      << s.outline.b << "],\n";
    f << "      \"fill\": [" << s.fill.r << ", " << s.fill.g << ", " << s.fill.b
      << "],\n";
    f << "      \"relleno\": " << (s.filled ? "true" : "false") << "\n";

    f << "    }";
    if (i + 1 < (int)sc.shapes.size())
      f << ",";
    f << "\n";
  }
  f << "  ]\n}\n";
  return f.good();
}

/* ================================================================== */
/* PARSER JSON MINIMO                                                   */
/* ================================================================== */

/* ---- Tokenizer ---- */

static void skipWs(const std::string &s, size_t &p) {
  while (p < s.size() &&
         (s[p] == ' ' || s[p] == '\n' || s[p] == '\r' || s[p] == '\t'))
    p++;
}

static std::string parseString(const std::string &s, size_t &p) {
  /* p apunta a la comilla inicial */
  p++; /* salta " */
  std::string result;
  while (p < s.size() && s[p] != '"') {
    if (s[p] == '\\')
      p++; /* escapes simples */
    if (p < s.size())
      result += s[p++];
  }
  if (p < s.size())
    p++; /* salta " final */
  return result;
}

static double parseNumber(const std::string &s, size_t &p) {
  size_t start = p;
  if (p < s.size() && (s[p] == '-' || s[p] == '+'))
    p++;
  while (p < s.size() && (isdigit(s[p]) || s[p] == '.' || s[p] == 'e' ||
                          s[p] == 'E' || s[p] == '+' || s[p] == '-'))
    p++;
  return std::stod(s.substr(start, p - start));
}

static bool parseBool(const std::string &s, size_t &p) {
  if (s.compare(p, 4, "true") == 0) {
    p += 4;
    return true;
  }
  if (s.compare(p, 5, "false") == 0) {
    p += 5;
    return false;
  }
  return false;
}

/* Nucleo del parser "dirigido por claves": busca la clave "key" a partir
   de la posicion p, y deja p justo despues del ':' que la sigue, listo
   para leer su valor. Retorna false si la clave no aparece. */
static bool seekKey(const std::string &s, size_t &p, const std::string &key) {
  std::string pattern = "\"" + key + "\"";
  size_t found = s.find(pattern, p);
  if (found == std::string::npos)
    return false;
  p = found + pattern.size();
  skipWs(s, p);
  if (p < s.size() && s[p] == ':')
    p++;
  skipWs(s, p);
  return true;
}

/* Lee un array de floats: [ f, f, ... ] */
static std::vector<float> parseFloatArray(const std::string &s, size_t &p) {
  std::vector<float> out;
  if (p >= s.size() || s[p] != '[')
    return out;
  p++; /* [ */
  while (p < s.size()) {
    skipWs(s, p);
    if (s[p] == ']') {
      p++;
      break;
    }
    if (s[p] == ',') {
      p++;
      continue;
    }
    out.push_back((float)parseNumber(s, p));
  }
  return out;
}

/* Lee un array de pares [x,y]: [ [x,y], [x,y], ... ] */
static std::vector<Vec2> parseVec2Array(const std::string &s, size_t &p) {
  std::vector<Vec2> out;
  if (p >= s.size() || s[p] != '[')
    return out;
  p++; /* [ outer */
  while (p < s.size()) {
    skipWs(s, p);
    if (s[p] == ']') {
      p++;
      break;
    }
    if (s[p] == ',') {
      p++;
      continue;
    }
    if (s[p] == '[') {
      auto xy = parseFloatArray(s, p);
      if (xy.size() >= 2)
        out.push_back({xy[0], xy[1]});
    } else {
      p++;
    }
  }
  return out;
}

/* Convierte string a ShapeType */
static ShapeType parseShapeType(const std::string &name) {
  if (name == "LINE")
    return ShapeType::LINE;
  if (name == "POLYLINE")
    return ShapeType::POLYLINE;
  if (name == "POLYGON")
    return ShapeType::POLYGON;
  return ShapeType::POINT;
}

/* Carga la escena desde JSON. Estrategia: leer todo el archivo a memoria,
   localizar el array "objetos", y recorrer cada bloque { ... } aislandolo
   con un contador de llaves balanceadas. De cada bloque extrae tipo,
   vertices, colores y relleno con seekKey + los parsers de arrays.
   Reemplaza por completo el contenido previo de la escena. */
bool loadSceneJson(Scene &sc, const std::string &path) {
  std::ifstream f(path);
  if (!f.is_open())
    return false;

  std::string text((std::istreambuf_iterator<char>(f)),
                   std::istreambuf_iterator<char>());
  f.close();

  sc.shapes.clear();

  /* Localizar el array de objetos */
  size_t pos = 0;
  if (!seekKey(text, pos, "objetos"))
    return false;
  if (pos >= text.size() || text[pos] != '[')
    return false;
  pos++; /* [ del array */

  /* Iterar sobre cada objeto { ... } */
  while (pos < text.size()) {
    skipWs(text, pos);
    if (text[pos] == ']')
      break;
    if (text[pos] == ',') {
      pos++;
      continue;
    }
    if (text[pos] != '{') {
      pos++;
      continue;
    }

    /* Aislar el objeto actual: avanzar hasta su '}' de cierre contando la
       profundidad de llaves, para soportar posibles llaves anidadas. */
    size_t objStart = pos;
    int depth = 0;
    size_t objEnd = pos;
    for (size_t i = pos; i < text.size(); i++) {
      if (text[i] == '{')
        depth++;
      else if (text[i] == '}') {
        depth--;
        if (depth == 0) {
          objEnd = i + 1;
          break;
        }
      }
    }
    std::string block = text.substr(objStart, objEnd - objStart);
    pos = objEnd;

    Shape s;
    s.selected = false;

    /* Tipo */
    size_t bp = 0;
    if (seekKey(block, bp, "tipo")) {
      if (bp < block.size() && block[bp] == '"')
        s.type = parseShapeType(parseString(block, bp));
    }

    /* Vertices */
    bp = 0;
    if (seekKey(block, bp, "vertices"))
      s.vertices = parseVec2Array(block, bp);

    /* Outline */
    bp = 0;
    if (seekKey(block, bp, "outline")) {
      auto v = parseFloatArray(block, bp);
      if (v.size() >= 3)
        s.outline = {v[0], v[1], v[2]};
    }

    /* Fill */
    bp = 0;
    if (seekKey(block, bp, "fill")) {
      auto v = parseFloatArray(block, bp);
      if (v.size() >= 3)
        s.fill = {v[0], v[1], v[2]};
    }

    /* Relleno */
    bp = 0;
    if (seekKey(block, bp, "relleno"))
      s.filled = parseBool(block, bp);

    if (!s.vertices.empty())
      sc.shapes.push_back(s);
  }
  return true;
}
