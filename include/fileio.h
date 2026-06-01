#pragma once
#include "scene.h"
#include <string>

/* ==================================================================
   fileio.h - Guardado y carga de la escena en JSON.
   Usa un writer y un parser propios (sin librerias externas) para un
   formato fijo y conocido. El parser esta dirigido por claves: no es un
   JSON de proposito general, pero basta porque el archivo lo genera el
   propio programa.
   ================================================================== */

/* Serializa la escena completa a un archivo JSON en la ruta indicada
   (clave "objetos" con tipo, vertices, colores y relleno de cada figura).
   Retorna true si la escritura fue exitosa. */
bool saveSceneJson(const Scene &sc, const std::string &path);

/* Deserializa la escena desde un archivo JSON. VACIA el contenido actual
   de sc y lo reemplaza por las figuras leidas del archivo.
   Retorna true si la carga fue exitosa. */
bool loadSceneJson(Scene &sc, const std::string &path);
