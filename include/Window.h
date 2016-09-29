#include <gl_core_3_3.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

#include <curl/curl.h>
#include <oauth.h>
#include <xmalloc.h>
#include <oauthConfig.h>

static void key_pressed(GLFWwindow*, int, int, int, int);
static void window_resized(GLFWwindow*, int, int);

void setProjectionMatrix(GLuint*, int, int);
void drawMainMenu(GLuint*, int, int);
void renderText(GLuint*, const char*, GLfloat, GLfloat, unsigned int, unsigned int, float*);
static int genFontTextures();
void texDump(GLuint*, int, int);
static char* flattenStringArray(const char**, size_t, const char*);
static size_t stringSize(const char*);
void setFontColor(float*, int);
void glPrintError(GLenum);
void curlPrintError(CURLcode curlstatus);
const char *ttwytter_request(const char*, const char*, const char*);
static void *initTwitterConnection(void *);
static size_t write_callback(void *, size_t, size_t, void *);
char* findBoss(char*, size_t);

struct Character {
    GLuint   TextureID;  // ID handle of the glyph texture
    GLfloat  Size[2];    // Size of glyph
    GLfloat  Bearing[2]; // Offset from baseline to left/top of glyph
    GLuint   Advance;    // Offset to advance to next glyph
};

const char* raidNames[] = {
    "Tiamat Omega",
    "Yggdrasil Omega",
    "Leviathan Omega",
    "Colossus Omega",
    "Luminiera Omega",
    "Celeste Omega",
    "Nezha",
    "Macula Marius",
    "Apollo",
    "Dark Angel Olivia",
    "Medusa",
    "Twin Elements",
    "Grand Order",
    "Proto Bahamut"};
const int raidNamesLength = 14;
const char *raidShortcuts[raidNamesLength] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N" };

const char* twitterNames[] = {
    "Lv50 ティアマト・マグナ", "Lvl 50 Tiamat Omega",
    "Lv60 ユグドラシル・マグナ", "Lvl 60 Yggdrasil Omega",
    "Lv60 リヴァイアサン・マグナ", "Lvl 60 Leviathan Omega",
    "Lv70 コロッサス・マグナ", "Lvl 70 Colossus Omega",
    "Lv75 シュヴァリエ・マグナ", "Lvl 75 Luminiera Omega",
    "Lv75 セレスト・マグナ", "Lvl 75 Celeste Omega",
    "Lv100 ナタク", "Lvl 100 Nezha",
    "Lv100 マキュラ・マリウス", "Lvl 100 Macula Marius",
    "Lv100 アポロン", "Lvl 100 Apollo",
    "Lv100 Dエンジェル・オリヴィエ", "Lvl 100 Dark Angel Olivia",
    "Lv100 メドゥーサ", "Lvl 100 Medusa",
    "Lv100 フラム＝グラス", "Lvl 100 Twin Elements",
    "Lv100 ジ・オーダー・グランデ", "Lvl 100 Grand Order",
    "Lv100 プロトバハムート", "Lvl 100 Proto Bahamut"
};
const int twitterNamesLength = raidNamesLength*2;

struct curlArgs {
    const char* arg1; // url
    curl_slist* arg2; // slist
    char* arg3; // postData
};

// Linked list object to as as buffer until OpenGL is able to render text
struct raidBuffer {
    unsigned int raidNumber; // Set on Twitter msg rec
    char *raidID; // Set on Twitter msg rec
    char *raidBoss; // Set on Twitter msg rec
    char *formattedText; // Formatted in main thread
    struct raidBuffer *next;
} raidBufferNode;
