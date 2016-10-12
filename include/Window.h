#include <gl_core_3_3.h>
#include <GLFW/glfw3.h>
#include <SOIL.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h> 
#include <windows.h>

#include <curl/curl.h>
#include <oauth.h>
#include <xmalloc.h>
#include <oauthConfig.h>

static void key_pressed(GLFWwindow*, int, int, int, int);
static void window_resized(GLFWwindow*, int, int);

void setProjectionMatrix(GLuint*, int, int);
void drawMainMenu(GLuint*, int, int, GLuint);
void renderText(GLuint*, const char*, GLfloat, GLfloat,
        unsigned int, unsigned int, float*);
void setClipboard(char*, unsigned int);
static int genFontTextures();
void texDump(GLuint*, int, int);
char** getRaidNames(unsigned int*);
uint32_t codepointToUTF8(uint16_t);
static char* flattenStringArray(char**, size_t, const char*);
static size_t stringSize(const char*);
void setFontColor(float*, int);
void glPrintError(GLenum);
void curlPrintError(CURLcode curlstatus);
const char *ttwytter_request(const char*, const char*, const char*);
static void *initTwitterConnection(void *);
static size_t write_callback(void *, size_t, size_t, void *);
char* findBoss(char*, size_t, char*);

struct Character {
    GLuint   TextureID;  // ID handle of the glyph texture
    GLfloat  Size[2];    // Size of glyph
    GLfloat  Bearing[2]; // Offset from baseline to left/top of glyph
    GLuint   Advance;    // Offset to advance to next glyph
};

struct raidList {
    char** raidNameList;
    unsigned int nRaidNameList;
    char** raidNameLegend;
    char* legendShortcut;
    int* raidNameLegendOn;
    unsigned int nRaidNameLegend;
    char** twitterOutputList;
    unsigned int nTwitterOutputList;
};

struct curlArgs {
    const char*  arg1; // url
    curl_slist*  arg2; // slist
    char*        arg3; // postData
    unsigned int arg4; // Retry delay
};

// Linked list object to as as buffer until OpenGL is able to render text
struct raidBuffer {
    unsigned int raidNumber; // Set on Twitter msg rec
    char *raidID; // Set on Twitter msg rec
    char *raidBoss; // Set on Twitter msg rec
    char *formattedText; // Formatted in main thread
    struct raidBuffer *next;
} raidBufferNode;
