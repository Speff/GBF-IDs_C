#include <gl_core_3_3.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

//#include <glm.h>
#include <stdio.h>
#include <cstdio>
#include <map>

static void key_pressed(GLFWwindow*, int, int, int, int);
static void window_resized(GLFWwindow*, int, int);

void setProjectionMatrix(GLuint*, int, int);
void drawMainMenu(GLuint*, int, int);
void renderText(GLuint*, char const*, GLfloat, GLfloat, GLfloat, float*);
void texDump(GLuint*, int, int);
void setFontColor(float*, int);
void glPrintError(GLenum);

struct Character {
    GLuint   TextureID;  // ID handle of the glyph texture
    GLfloat  Size[2];    // Size of glyph
    GLfloat  Bearing[2]; // Offset from baseline to left/top of glyph
    GLuint   Advance;    // Offset to advance to next glyph
};
