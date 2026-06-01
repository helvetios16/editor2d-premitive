#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif
#include <GLFW/glfw3.h>
#include <cstdio>

#include "scene.h"
#include "tools.h"
#include "ui.h"

/* ================================================================== */
/* Contexto global de la aplicacion                                     */
/* (unico struct, pasado a callbacks via glfwSetWindowUserPointer)     */
/* ================================================================== */
/* Estado UNICO de toda la aplicacion. En lugar de variables globales, se
   crea una instancia en main() y se asocia a la ventana con
   glfwSetWindowUserPointer; cada callback la recupera. Asi todo el estado
   (documento, herramienta, toolbar y datos del mouse) vive en un solo
   sitio y es facil de pasar entre funciones. */
struct AppContext {
  Scene scene;                 /* el documento: lista de figuras */
  EditorState state;           /* herramienta activa, seleccion, colores... */
  std::vector<Button> toolbar; /* botones del panel lateral */
  int winW, winH;              /* tamaño de la ventana (coords logicas) */
  int fbW, fbH;                /* tamaño del framebuffer (px reales, Retina) */
  double mouseX, mouseY;       /* posicion actual del cursor */
  bool mouseDown;              /* boton izquierdo pulsado */
  bool wasDragging;            /* true si el gesto ya supero el umbral de drag */
  double pressX, pressY;       /* punto donde se presiono (para medir el drag) */
};

/* ------------------------------------------------------------------ */
/* Proyeccion 2D: origen arriba-izquierda, Y hacia abajo               */
/* ------------------------------------------------------------------ */
/* Configura una proyeccion ortografica 2D con el origen ARRIBA-IZQUIERDA y
   la Y hacia abajo (gluOrtho2D(0,w,h,0)). Asi las coordenadas que da el
   mouse coinciden 1:1 con las del mundo, sin conversiones. */
static void setOrtho2D(int w, int h) {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, h, 0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

/* ------------------------------------------------------------------ */
/* Texto de estado en el canvas (esquina inferior)                     */
/* ------------------------------------------------------------------ */
/* Nombre legible de la herramienta, para mostrarlo en la barra de estado. */
static const char *toolName(Tool t) {
  switch (t) {
  case Tool::POINT:
    return "Punto";
  case Tool::LINE:
    return "Linea";
  case Tool::POLYLINE:
    return "Polilinea";
  case Tool::POLYGON:
    return "Poligono";
  case Tool::SELECT:
    return "Seleccion";
  case Tool::TRANSLATE:
    return "Trasladar";
  case Tool::ROTATE:
    return "Rotar (drag horizontal)";
  case Tool::SCALE:
    return "Escalar (drag vertical)";
  }
  return "";
}

/* Dibuja la barra inferior: herramienta activa, estado del relleno y
   numero de figuras; ademas, recuerda los atajos cuando hay una figura
   en construccion. */
static void drawStatusBar(const AppContext &app) {
  char buf[256];
  snprintf(
      buf, sizeof(buf), "Herramienta: %s | %s | Figuras: %d%s",
      toolName(app.state.tool), app.state.filled ? "Relleno ON" : "Relleno OFF",
      (int)app.scene.shapes.size(),
      app.state.pending.empty() ? "" : " | [Enter]=finalizar [Esc]=cancelar");

  glColor3f(0.7f, 0.7f, 0.7f);
  drawText((float)TOOLBAR_W + 8.0f, (float)app.winH - 8.0f, buf);
}

/* ------------------------------------------------------------------ */
/* Render                                                               */
/* ------------------------------------------------------------------ */
/* Dibuja un frame completo, de atras hacia adelante: fondo, cuadricula de
   referencia, figuras de la escena, preview de la figura en curso, panel
   de toolbar y barra de estado. Se llama una vez por iteracion del bucle. */
static void render(AppContext &app) {
  glViewport(0, 0, app.fbW, app.fbH);
  setOrtho2D(app.winW, app.winH);

  glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  /* Canvas: cuadricula de referencia */
  glColor3f(0.14f, 0.14f, 0.18f);
  glBegin(GL_LINES);
  for (int x = TOOLBAR_W; x < app.winW; x += 40) {
    glVertex2f((float)x, 0);
    glVertex2f((float)x, (float)app.winH);
  }
  for (int y = 0; y < app.winH; y += 40) {
    glVertex2f((float)TOOLBAR_W, (float)y);
    glVertex2f((float)app.winW, (float)y);
  }
  glEnd();

  /* Figuras de la escena */
  sceneRender(app.scene);

  /* Preview de figura en curso */
  toolDrawPending(app.state, {(float)app.mouseX, (float)app.mouseY});

  /* Toolbar lateral */
  drawToolbar(app.toolbar, toolToAction(app.state.tool), app.state.filled,
              app.state.colorTarget, app.state.currentOutline,
              app.state.currentFill);

  /* Barra de estado */
  drawStatusBar(app);
}

/* ================================================================== */
/* Callbacks GLFW                                                       */
/* ================================================================== */

/* Mouse movido: actualiza la posicion y, si el boton esta pulsado, decide
   si el gesto se convierte en arrastre. El arrastre solo empieza cuando el
   cursor se aleja mas de 3 px del punto de presion (umbral dx^2+dy^2 > 9),
   evitando que un click con micro-movimiento se interprete como drag. */
static void cursorPosCallback(GLFWwindow *win, double mx, double my) {
  AppContext *app = static_cast<AppContext *>(glfwGetWindowUserPointer(win));
  app->mouseX = mx;
  app->mouseY = my;

  if (app->mouseDown) {
    double dx = mx - app->pressX, dy = my - app->pressY;
    if (!app->wasDragging && dx * dx + dy * dy > 9.0) {
      /* Umbral superado: iniciar arrastre */
      app->wasDragging = true;
      if (mx >= TOOLBAR_W)
        toolHandleDragStart(app->state, app->scene,
                            {(float)app->pressX, (float)app->pressY});
    }
    if (app->wasDragging && mx >= TOOLBAR_W)
      toolHandleDragMove(app->state, app->scene, {(float)mx, (float)my});
  }
}

/* Boton del mouse. Al soltar distingue click de arrastre: si hubo drag,
   cierra la transformacion; si fue un click limpio, enruta segun donde
   ocurrio: en el panel (x < TOOLBAR_W) ejecuta la accion del boton, en el
   canvas delega en la herramienta activa. */
static void mouseButtonCallback(GLFWwindow *win, int button, int action,
                                int /*mods*/) {
  AppContext *app = static_cast<AppContext *>(glfwGetWindowUserPointer(win));

  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      app->mouseDown = true;
      app->wasDragging = false;
      app->pressX = app->mouseX;
      app->pressY = app->mouseY;
    } else if (action == GLFW_RELEASE) {
      if (app->wasDragging) {
        toolHandleDragEnd(app->state);
      } else {
        /* Click sin arrastre */
        double mx = app->mouseX, my = app->mouseY;
        if (mx < TOOLBAR_W) {
          /* Click en toolbar */
          Action a = toolbarHandleClick(app->toolbar, mx, my);
          if (a != Action::NONE)
            toolHandleAction(app->state, app->scene, a);
        } else {
          /* Click en canvas */
          toolHandleClick(app->state, app->scene, {(float)mx, (float)my});
        }
      }
      app->mouseDown = false;
      app->wasDragging = false;
    }
  }
}

/* Teclado: traduce cada tecla (considerando Ctrl) a una Action y la pasa al
   despachador toolHandleAction. Las flechas son la excepcion: mueven
   directamente la figura seleccionada 5 px aplicando una traslacion. */
static void keyCallback(GLFWwindow *win, int key, int /*sc*/, int action,
                        int mods) {
  if (action != GLFW_PRESS && action != GLFW_REPEAT)
    return;
  AppContext *app = static_cast<AppContext *>(glfwGetWindowUserPointer(win));

  bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;

  switch (key) {
  /* Herramientas de dibujo */
  case GLFW_KEY_P:
    toolHandleAction(app->state, app->scene, Action::TOOL_POINT);
    break;
  case GLFW_KEY_L:
    toolHandleAction(app->state, app->scene, Action::TOOL_LINE);
    break;
  case GLFW_KEY_O:
    if (ctrl)
      toolHandleAction(app->state, app->scene, Action::FILE_LOAD);
    else
      toolHandleAction(app->state, app->scene, Action::TOOL_POLYLINE);
    break;
  case GLFW_KEY_G:
    toolHandleAction(app->state, app->scene, Action::TOOL_POLYGON);
    break;
  /* Seleccion y transformacion */
  case GLFW_KEY_S:
    if (ctrl)
      toolHandleAction(app->state, app->scene, Action::FILE_SAVE);
    else
      toolHandleAction(app->state, app->scene, Action::TOOL_SELECT);
    break;
  case GLFW_KEY_T:
    toolHandleAction(app->state, app->scene, Action::TOOL_TRANSLATE);
    break;
  case GLFW_KEY_R:
    toolHandleAction(app->state, app->scene, Action::TOOL_ROTATE);
    break;
  case GLFW_KEY_E:
    toolHandleAction(app->state, app->scene, Action::TOOL_SCALE);
    break;
  /* Propiedades */
  case GLFW_KEY_F:
    toolHandleAction(app->state, app->scene, Action::TOGGLE_FILL);
    break;
  case GLFW_KEY_D:
    toolHandleAction(app->state, app->scene, Action::SCENE_DEMO);
    break;
  /* Control de figura en curso */
  case GLFW_KEY_ENTER:
    toolHandleAction(app->state, app->scene, Action::FINISH_SHAPE);
    break;
  case GLFW_KEY_ESCAPE:
    toolHandleAction(app->state, app->scene, Action::CANCEL_SHAPE);
    break;
  case GLFW_KEY_DELETE:
  case GLFW_KEY_BACKSPACE:
    toolHandleAction(app->state, app->scene, Action::DELETE_SELECTED);
    break;
  /* Transformacion por teclado (requiere objeto seleccionado) */
  case GLFW_KEY_LEFT: {
    int idx = app->state.selectedIndex;
    if (idx >= 0 && idx < (int)app->scene.shapes.size())
      shapeTransform(app->scene.shapes[idx], mat3Translate(-5, 0));
    break;
  }
  case GLFW_KEY_RIGHT: {
    int idx = app->state.selectedIndex;
    if (idx >= 0 && idx < (int)app->scene.shapes.size())
      shapeTransform(app->scene.shapes[idx], mat3Translate(5, 0));
    break;
  }
  case GLFW_KEY_UP: {
    int idx = app->state.selectedIndex;
    if (idx >= 0 && idx < (int)app->scene.shapes.size())
      shapeTransform(app->scene.shapes[idx], mat3Translate(0, -5));
    break;
  }
  case GLFW_KEY_DOWN: {
    int idx = app->state.selectedIndex;
    if (idx >= 0 && idx < (int)app->scene.shapes.size())
      shapeTransform(app->scene.shapes[idx], mat3Translate(0, 5));
    break;
  }
  default:
    break;
  }
}

/* La ventana cambio de tamaño: guarda el nuevo tamaño de framebuffer (px
   reales) y de ventana (coords logicas). Pueden diferir en pantallas
   Retina, por eso se guardan ambos. */
static void framebufferSizeCallback(GLFWwindow *win, int fbW, int fbH) {
  AppContext *app = static_cast<AppContext *>(glfwGetWindowUserPointer(win));
  app->fbW = fbW;
  app->fbH = fbH;
  glfwGetWindowSize(win, &app->winW, &app->winH);
}

/* ================================================================== */
/* Main                                                                 */
/* ================================================================== */
/* Punto de entrada: inicializa GLUT (solo para fuentes) y GLFW, crea la
   ventana, prepara el AppContext, registra los callbacks y corre el bucle
   principal render -> swap -> poll hasta que se cierre la ventana. */
int main(int argc, char **argv) {
  /* Inicializar GLUT solo para texto bitmap (no crea ventana) */
  glutInit(&argc, argv);

  if (!glfwInit())
    return -1;

  GLFWwindow *window = glfwCreateWindow(
      950, 620, "Laboratorio 6 - Editor de Primitivas 2D", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1); /* vsync */

  /* Contexto sin globales */
  AppContext app;
  app.state = stateInit();
  app.mouseX = app.mouseY = 0;
  app.mouseDown = app.wasDragging = false;
  app.pressX = app.pressY = 0;
  glfwGetWindowSize(window, &app.winW, &app.winH);
  glfwGetFramebufferSize(window, &app.fbW, &app.fbH);
  buildToolbar(app.toolbar);

  glfwSetWindowUserPointer(window, &app);
  glfwSetCursorPosCallback(window, cursorPosCallback);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

  printf("Editor de Primitivas 2D\n");
  printf("Teclas: P=Punto L=Linea O=Polilinea G=Poligono\n");
  printf("        S=Selec T=Trasladar R=Rotar E=Escalar\n");
  printf("        F=Relleno D=Demo Ctrl+S=Guardar Ctrl+O=Abrir\n");
  printf("        Enter=Finalizar Esc=Cancelar Del=Borrar\n");
  printf("        Flechas=Mover objeto seleccionado\n");

  /* Bucle principal: redibuja todo cada frame, presenta el buffer y
     procesa los eventos pendientes (que invocan a los callbacks). */
  while (!glfwWindowShouldClose(window)) {
    render(app);
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
