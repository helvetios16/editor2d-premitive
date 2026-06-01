#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>

static float angleX = 20.0f;
static float angleY = 0.0f;
static bool usePerspective = false;

/* Camara para exercise3 */
static float camX = 0.0f;
static float camY = 1.5f;
static float camZ = 8.0f;
static bool followEarth = true;

/* Trackball para exercise5 */
static float tbRotX = 20.0f;
static float tbRotY = 30.0f;
static float tbZoom = 10.0f;
static bool tbDragging = false;
static double tbLastX = 0.0;
static double tbLastY = 0.0;

/* ------------------------------------------------------------------ */
static void drawGrid() {
  glDisable(GL_LIGHTING);
  glColor3f(0.10f, 0.25f, 0.70f);
  glBegin(GL_LINES);
  for (int i = -10; i <= 10; i++) {
    glVertex3f((float)i, -1.5f, -10.0f);
    glVertex3f((float)i, -1.5f, 10.0f);
    glVertex3f(-10.0f, -1.5f, (float)i);
    glVertex3f(10.0f, -1.5f, (float)i);
  }
  glEnd();
}

/* ------------------------------------------------------------------ */
/* Ejercicio 1 - Comparacion ortografica vs perspectiva               */
/* ------------------------------------------------------------------ */
void exercise1() {
  glRotatef(25.0f, 1.0f, 0.0f, 0.0f);

  glDisable(GL_LIGHTING);
  drawGrid();

  glPushMatrix();
  glTranslatef(-2.5f, 0.0f, 5.0f);
  glColor3f(0.85f, 0.25f, 0.20f);
  glutWireCube(1.2);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0.0f, 0.0f, 0.0f);
  glColor3f(0.20f, 0.80f, 0.30f);
  glutWireSphere(0.75, 16, 16);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(2.5f, 0.0f, -5.0f);
  glColor3f(0.20f, 0.40f, 0.90f);
  glutWireTorus(0.25, 0.60, 16, 32);
  glPopMatrix();
}

/* ------------------------------------------------------------------ */
/* Ejercicio 2 - Escena wireframe con proyeccion ortografica/persp    */
/* ------------------------------------------------------------------ */
void exercise2() {
  angleY += 0.4f;
  if (angleY >= 360.0f)
    angleY -= 360.0f;
  glRotatef(angleX, 1.0f, 0.0f, 0.0f);
  glRotatef(angleY, 0.0f, 1.0f, 0.0f);

  drawGrid();

  glColor3f(1.0f, 1.0f, 1.0f);

  glPushMatrix();
  glTranslatef(-2.2f, 0.0f, 0.0f);
  glutWireCube(1.1);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(0.0f, 0.0f, 0.0f);
  glutWireSphere(0.75, 16, 16);
  glPopMatrix();

  glPushMatrix();
  glTranslatef(2.2f, 0.0f, 0.0f);
  glutWireTorus(0.25, 0.60, 16, 32);
  glPopMatrix();
}

/* ------------------------------------------------------------------ */
/* Ejercicio 3 - Camara con gluLookAt controlada por teclado          */
/* Controles:                                                          */
/*   <- ->   mover camara horizontal                                  */
/*   ^  v    mover camara vertical                                     */
/*   W / S   acercar / alejar                                         */
/* ------------------------------------------------------------------ */
void exercise3() {
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);

  /* Cubo - izquierda, rojo */
  glPushMatrix();
  glTranslatef(-2.5f, 0.0f, 0.0f);
  glColor3f(0.85f, 0.25f, 0.20f);
  glutSolidCube(1.2);
  glPopMatrix();

  /* Esfera - centro, verde */
  glPushMatrix();
  glTranslatef(0.0f, 0.0f, 0.0f);
  glColor3f(0.20f, 0.80f, 0.30f);
  glutSolidSphere(0.80, 32, 32);
  glPopMatrix();

  /* Toro - derecha, azul */
  glPushMatrix();
  glTranslatef(2.5f, 0.0f, 0.0f);
  glColor3f(0.20f, 0.40f, 0.90f);
  glutSolidTorus(0.25, 0.65, 16, 32);
  glPopMatrix();

  drawGrid();
}

/* ------------------------------------------------------------------ */
/* Ejercicio 4 - Sistema Solar con camara que sigue a la Tierra        */
/* ------------------------------------------------------------------ */

static void drawOrbit(float radius) {
  glDisable(GL_LIGHTING);
  glColor3f(0.22f, 0.22f, 0.30f);
  glBegin(GL_LINE_LOOP);
  for (int i = 0; i < 128; i++) {
    float a = i * 2.0f * 3.14159f / 128.0f;
    glVertex3f(radius * cosf(a), 0.0f, radius * sinf(a));
  }
  glEnd();
}

void exercise4() {
  double t = glfwGetTime();

  /* Velocidades angulares (rad/s) proporcionales a periodos reales */
  float mercuryA = (float)t * 2.61f;
  float venusA = (float)t * 1.01f;
  float earthA = (float)t * 0.63f;
  float moonA = (float)t * 5.00f;
  float marsA = (float)t * 0.33f;

  const float mercuryR = 2.8f, mercurySize = 0.12f;
  const float venusR = 4.2f, venusSize = 0.30f;
  const float earthR = 6.2f, earthSize = 0.36f;
  const float moonR = 0.85f, moonSize = 0.10f;
  const float marsR = 9.0f, marsSize = 0.22f;

  /* Orbitas */
  drawOrbit(mercuryR);
  drawOrbit(venusR);
  drawOrbit(earthR);
  drawOrbit(marsR);

  /* Sol — sin iluminacion para que luzca emisivo */
  glDisable(GL_LIGHTING);
  glColor3f(1.0f, 0.88f, 0.10f);
  glutSolidSphere(1.4, 32, 32);

  /* Luz puntual en el origen (sol) */
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  GLfloat sunLightPos[] = {0.0f, 0.0f, 0.0f, 1.0f};
  GLfloat sunDiff[] = {1.0f, 0.95f, 0.8f, 1.0f};
  GLfloat sunAmb[] = {0.04f, 0.04f, 0.04f, 1.0f};
  glLightfv(GL_LIGHT0, GL_POSITION, sunLightPos);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiff);
  glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmb);

  /* Mercurio */
  glPushMatrix();
  glTranslatef(mercuryR * cosf(mercuryA), 0.0f, mercuryR * sinf(mercuryA));
  glColor3f(0.55f, 0.55f, 0.55f);
  glutSolidSphere(mercurySize, 12, 12);
  glPopMatrix();

  /* Venus */
  glPushMatrix();
  glTranslatef(venusR * cosf(venusA), 0.0f, venusR * sinf(venusA));
  glColor3f(0.88f, 0.72f, 0.28f);
  glutSolidSphere(venusSize, 16, 16);
  glPopMatrix();

  /* Tierra + Luna */
  float ex = earthR * cosf(earthA);
  float ez = earthR * sinf(earthA);
  glPushMatrix();
  glTranslatef(ex, 0.0f, ez);

  glColor3f(0.18f, 0.42f, 0.88f);
  glutSolidSphere(earthSize, 24, 24);

  /* Luna orbita la Tierra */
  glPushMatrix();
  glTranslatef(moonR * cosf(moonA), 0.0f, moonR * sinf(moonA));
  glColor3f(0.78f, 0.78f, 0.78f);
  glutSolidSphere(moonSize, 12, 12);
  glPopMatrix();

  glPopMatrix();

  /* Marte */
  glPushMatrix();
  glTranslatef(marsR * cosf(marsA), 0.0f, marsR * sinf(marsA));
  glColor3f(0.78f, 0.28f, 0.10f);
  glutSolidSphere(marsSize, 16, 16);
  glPopMatrix();
}

/* ------------------------------------------------------------------ */

static int currentExercise = 1;

/* ------------------------------------------------------------------ */
/* Ejercicio 5 - Camara trackball con mouse                            */
/* ------------------------------------------------------------------ */

static void drawGizmo() {
  glDisable(GL_LIGHTING);
  glLineWidth(2.5f);
  glBegin(GL_LINES);
  glColor3f(1.0f, 0.0f, 0.0f);
  glVertex3f(0, 0, 0);
  glVertex3f(2, 0, 0); /* X rojo  */
  glColor3f(0.0f, 1.0f, 0.0f);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 2, 0); /* Y verde */
  glColor3f(0.0f, 0.0f, 1.0f);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, 2); /* Z azul  */
  glEnd();
  glLineWidth(1.0f);

  /* Conos en las puntas */
  glPushMatrix();
  glColor3f(1.0f, 0.0f, 0.0f);
  glTranslatef(2.0f, 0, 0);
  glRotatef(90, 0, 1, 0);
  glutSolidCone(0.07, 0.22, 8, 1);
  glPopMatrix();

  glPushMatrix();
  glColor3f(0.0f, 1.0f, 0.0f);
  glTranslatef(0, 2.0f, 0);
  glRotatef(-90, 1, 0, 0);
  glutSolidCone(0.07, 0.22, 8, 1);
  glPopMatrix();

  glPushMatrix();
  glColor3f(0.0f, 0.0f, 1.0f);
  glTranslatef(0, 0, 2.0f);
  glutSolidCone(0.07, 0.22, 8, 1);
  glPopMatrix();

  /* Etiquetas */
  glColor3f(1.0f, 0.2f, 0.2f);
  glRasterPos3f(2.35f, 0.0f, 0.0f);
  glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'X');
  glColor3f(0.2f, 1.0f, 0.2f);
  glRasterPos3f(0.0f, 2.35f, 0.0f);
  glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Y');
  glColor3f(0.2f, 0.4f, 1.0f);
  glRasterPos3f(0.0f, 0.0f, 2.35f);
  glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Z');
}

void exercise5() {
  glRotatef(tbRotX, 1.0f, 0.0f, 0.0f);
  glRotatef(tbRotY, 0.0f, 1.0f, 0.0f);

  drawGizmo();
  drawGrid();

  glDisable(GL_LIGHTING);
  glColor3f(1.0f, 1.0f, 1.0f);
  glutWireTeapot(1.0);
}

/* Callbacks del mouse */
static void mouse_button_callback(GLFWwindow *win, int button, int action,
                                  int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && currentExercise == 5) {
    tbDragging = (action == GLFW_PRESS);
    if (tbDragging)
      glfwGetCursorPos(win, &tbLastX, &tbLastY);
  }
}

static void cursor_pos_callback(GLFWwindow *win, double x, double y) {
  if (tbDragging && currentExercise == 5) {
    tbRotY += (float)(x - tbLastX) * 0.4f;
    tbRotX += (float)(y - tbLastY) * 0.4f;
    tbLastX = x;
    tbLastY = y;
  }
}

static void scroll_callback(GLFWwindow *win, double xoff, double yoff) {
  if (currentExercise == 5) {
    tbZoom -= (float)yoff * 0.6f;
    if (tbZoom < 2.0f)
      tbZoom = 2.0f;
    if (tbZoom > 40.0f)
      tbZoom = 40.0f;
  }
}

/* ------------------------------------------------------------------ */

static void key_callback(GLFWwindow *win, int key, int scancode, int action,
                         int mods) {
  /* Teclas de un solo disparo */
  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_ESCAPE)
      glfwSetWindowShouldClose(win, GLFW_TRUE);
    if (key == GLFW_KEY_O)
      usePerspective = false;
    if (key == GLFW_KEY_P)
      usePerspective = true;
    if (key >= GLFW_KEY_1 && key <= GLFW_KEY_5)
      currentExercise = key - GLFW_KEY_0;
    if (key == GLFW_KEY_V && currentExercise == 4)
      followEarth = !followEarth;
  }

  /* Movimiento de camara (exercise3): disparo continuo con REPEAT */
  if ((action == GLFW_PRESS || action == GLFW_REPEAT) && currentExercise == 3) {
    const float step = 0.2f;
    if (key == GLFW_KEY_LEFT)
      camX -= step;
    if (key == GLFW_KEY_RIGHT)
      camX += step;
    if (key == GLFW_KEY_UP)
      camY += step;
    if (key == GLFW_KEY_DOWN)
      camY -= step;
    if (key == GLFW_KEY_W)
      camZ -= step;
    if (key == GLFW_KEY_S)
      camZ += step;
  }
}

int main(int argc, char **argv) {
  if (!glfwInit())
    return -1;

  GLFWwindow *window = glfwCreateWindow(
      900, 600, "Laboratorio 5 - Computacion Grafica", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_pos_callback);
  glfwSetScrollCallback(window, scroll_callback);

  /* Iluminacion */
  GLfloat lightAmbient[] = {0.2f, 0.2f, 0.2f, 1.0f};
  GLfloat lightDiffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
  GLfloat lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
  glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
  glEnable(GL_NORMALIZE);
  glEnable(GL_COLOR_MATERIAL);
  glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
  GLfloat matSpec[] = {0.4f, 0.4f, 0.4f, 1.0f};
  glMaterialfv(GL_FRONT, GL_SPECULAR, matSpec);
  glMateriali(GL_FRONT, GL_SHININESS, 48);

  while (!glfwWindowShouldClose(window)) {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    if (currentExercise == 4)
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    else
      glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)width / (float)height;
    if (usePerspective || currentExercise == 3 || currentExercise == 4 ||
        currentExercise == 5) {
      gluPerspective(60.0, aspect, 0.1, 200.0);
    } else {
      float size = 5.77f;
      if (width >= height)
        glOrtho(-size * aspect, size * aspect, -size, size, -50.0f, 50.0f);
      else
        glOrtho(-size, size, -size / aspect, size / aspect, -50.0f, 50.0f);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (currentExercise == 3) {
      gluLookAt(camX, camY, camZ, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
      GLfloat lightPos[] = {3.0f, 6.0f, 5.0f, 1.0f};
      glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    } else if (currentExercise == 4) {
      double t = glfwGetTime();
      float earthA = (float)t * 0.63f;
      float earthR = 6.2f;
      float ex = earthR * cosf(earthA);
      float ez = earthR * sinf(earthA);
      if (followEarth) {
        /* Camara detras de la Tierra, ligeramente elevada — sigue la orbita */
        float camDist = 3.5f, camH = 2.0f;
        gluLookAt(ex + cosf(earthA) * camDist, camH,
                  ez + sinf(earthA) * camDist, ex, 0.0f, ez, 0.0f, 1.0f, 0.0f);
      } else {
        /* Vista cenital fija: camara directamente encima del Sol mirando abajo
         */
        gluLookAt(0.0f, 22.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f);
      }
    } else if (currentExercise == 5) {
      gluLookAt(0.0f, 0.0f, tbZoom, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
      GLfloat lightPos[] = {5.0f, 8.0f, 10.0f, 1.0f};
      glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    } else {
      glTranslatef(0.0f, 0.0f, -10.0f);
      GLfloat lightPos[] = {4.0f, 6.0f, 8.0f, 1.0f};
      glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    }

    switch (currentExercise) {
    case 1:
      exercise1();
      break;
    case 2:
      exercise2();
      break;
    case 3:
      exercise3();
      break;
    case 4:
      exercise4();
      break;
    case 5:
      exercise5();
      break;
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
