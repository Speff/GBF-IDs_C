#include <Window.h>

GLuint VAO, VBO;
Character charArray[128];
GLuint program;

const GLchar* vertShdr =
"#version 330 core\n\
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n\
out vec2 TexCoords;\n\
\n\
uniform mat4 projection;\n\
\n\
void main()\n\
{\n\
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n\
    TexCoords = vertex.zw;\n\
} ";

const GLchar* fragShdr =
"#version 330 core\n\
in vec2 TexCoords;\n\
out vec4 color;\n\
\n\
uniform sampler2D text;\n\
uniform vec3 textColor;\n\
\n\
void main()\n\
{    \n\
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n\
    color = vec4(textColor, 1.0) * sampled;\n\
}  ";

int main(void){
    GLFWwindow* window;
    int screen_width = 420;
    int screen_height = 480;
    //const char* text = "Lorem ipsum dolor sit.....";

    FT_Library fontLibrary;
    FT_Face fontFace;
    FT_Error error;

    // Initialize the library
    printf("Initializing GLFW lib\n");
    if (!glfwInit())
        return -1;

    // Create a windowed mode window and its OpenGL context
    printf("Initializing GLFW window\n");
    window = glfwCreateWindow(screen_width, screen_height,
            "GBF-IDs", NULL, NULL);
    if (!window){
        printf("Problem creating window\n");
        glfwTerminate();
        return -1;
    }

    glViewport(0, 0, screen_width, screen_height);

    // Make the window's context current
    printf("Making GLFW context current\n");
    glfwMakeContextCurrent(window);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Create a callback for keypresses
    printf("Setting GLFW callbacks\n");
    glfwSetKeyCallback(window, key_pressed);
    glfwSetWindowSizeCallback(window, window_resized);

    // Compile the shaders
    GLuint vertex, fragment;
    GLint compilationSuccess;
    GLchar infoLog[512];

    // Vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertShdr, NULL);
    glCompileShader(vertex);
    //Print compilation errors
    glGetShaderiv(vertex, GL_COMPILE_STATUS, &compilationSuccess);
    if(!compilationSuccess){
        glGetShaderInfoLog(vertex, 512, NULL, infoLog);
        printf("Vert shader compilation failed: %s\n", infoLog);
    }
    // Fragment shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragShdr, NULL);
    glCompileShader(fragment);
    //Print compilation errors
    glGetShaderiv(fragment, GL_COMPILE_STATUS, &compilationSuccess);
    if(!compilationSuccess){
        glGetShaderInfoLog(fragment, 512, NULL, infoLog);
        printf("Frag shader compilation failed: %s\n", infoLog);
    }

    // Create shader program
    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    // Print linking errors
    glGetProgramiv(program, GL_LINK_STATUS, &compilationSuccess);
    if(!compilationSuccess){
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        printf("Program linking failed: %s\n", infoLog);
    }
    glUseProgram(program);

    // Delete shaders
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*6*4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    setProjectionMatrix(&program, screen_width, screen_height);

    // Load font library
    printf("Initializing FreeType\n");
    error = FT_Init_FreeType(&fontLibrary);
    if(error){
        printf("Cannot load FreeType library\n");
        return -1;
    }

    // Load font file into library
    printf("Loading font file\n");
    error = FT_New_Face(fontLibrary, "PTS55F.ttf", 0, &fontFace);
    if(error == FT_Err_Unknown_File_Format){
        printf("Font opened, but format is unknown\n");
    }
    else if(error){
        printf("Font file cannot be read");
    }

    // Set font size
    FT_Set_Pixel_Sizes(fontFace, 0, 36);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for(GLubyte c = 0; c < 128; c++){
        // Load character glyph
        if(FT_Load_Char(fontFace, c, FT_LOAD_RENDER)){
            printf("Error-FreeType: Failed to load glyph\n");
            //continue;
        }

        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                fontFace->glyph->bitmap.width,
                fontFace->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                fontFace->glyph->bitmap.buffer);

        //printf("BITMAP BUFFER-----%u (%1s)\n%u\nWidth: %u\nHeight: %u\n",
        //        texture, &c, (unsigned int)fontFace->glyph->bitmap.buffer,
        //        fontFace->glyph->bitmap.width, fontFace->glyph->bitmap.rows);
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Store character for later use
        charArray[c].TextureID = texture;
        charArray[c].Size[0] = fontFace->glyph->bitmap.width;
        charArray[c].Size[1] = fontFace->glyph->bitmap.rows;
        charArray[c].Bearing[0] = fontFace->glyph->bitmap_left;
        charArray[c].Bearing[1] = fontFace->glyph->bitmap_top;
        charArray[c].Advance = fontFace->glyph->advance.x;

        //texDump(&texture, charArray[c].Size[0], charArray[c].Size[1]);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    FT_Done_Face(fontFace);
    FT_Done_FreeType(fontLibrary);

    // Loop until the user closes the window
    printf("Starting main GLFW Loop\n");
    while (!glfwWindowShouldClose(window)){
        // Render here
        glClearColor(0.95f, 0.95f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwGetFramebufferSize(window, &screen_width, &screen_height);
        glViewport(0, 0, screen_width, screen_height);

        //setFontColor(fontColor, 1);
        //renderText(&program, text, 20.0f, 200.0f, 0.7f, fontColor);

        drawMainMenu(&program, screen_width, screen_height);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void drawMainMenu(GLuint* prog, int width, int height){
    // FONTSIZE_BASE = 36PX
    float fontColor[3];
    float font_baseSize = 36.0f;
    float scale14px = 14.0f/font_baseSize;
    float scale15px = 15.0f/font_baseSize;
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
    int raidNamesLength = 14;

    setFontColor(fontColor, 0);
    renderText(prog, "GBF Raid IDs", 15, height-15-36, 1.0, fontColor);
    renderText(prog,
            "Press the key next to the boss' names to filter incoming IDs",
            15, height-65-15, scale15px, fontColor);

    int tableLocX = 15;
    int tableLocY = height - 110;
    int tableCheckCharacterLength = 15;

    for(int i = 0; i < raidNamesLength; i++){
        renderText(prog, raidNames[i], tableLocX + tableCheckCharacterLength,
                tableLocY - 19*i, scale15px, fontColor);
    }

}

void renderText(GLuint* prog, char const* text, GLfloat x, GLfloat y,
        GLfloat scale, float* color){

    // Activate corresponding render state
    glUniform3f(glGetUniformLocation(*prog, "textColor"), color[0],
            color[1], color[2]);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Iterate through all characters
    for(size_t c = 0; c < strlen(text); c++){
        Character ch = charArray[(size_t)text[c]];
        //printf("Character info - %c\nSize: %f, %f\nBearing: %f, %f
        //        \nAdvance: %i\nTexture ID: %i\n\n", 
        //        text[c], ch.Size[0], ch.Size[1], ch.Bearing[0],
        //        ch.Bearing[1], ch.Advance, ch.TextureID);

        GLfloat xpos = x + ch.Bearing[0] * scale;
        GLfloat ypos = y - (ch.Size[1] - ch.Bearing[1]) * scale;

        GLfloat w = ch.Size[0] * scale;
        GLfloat h = ch.Size[1] * scale;
        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }
        };
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        //glPrintError(glGetError());
        // Now advance cursors for next glyph
        // (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void texDump(GLuint* texID, int W, int H) {
    char fileName[100];
    
    if(snprintf(fileName, 100, "tex-%u.tga", *texID) < 0){
        printf("Program writing name for tex dump\n");
        return;
    }

    FILE   *out = fopen(fileName,"wb");
    char   *pixel_data = new char[4*W*H];
    short  TGAhead[] = { 0, 2, 0, 0, 0, 0, (short)W, (short)H, 32 };

    printf("Dumping texture-%u\n", *texID);

    //glBindTexture(GL_TEXTURE_2D, *texID);
    //glGetTexImage(
    //        GL_TEXTURE_2D,
    //        0,
    //        GL_RED_INTEGER,
    //        GL_UNSIGNED_BYTE,
    //        pixel_data);
            
    //glReadBuffer(GL_FRONT);
    //glReadPixels(0, 0, W, H, GL_BGRA, GL_UNSIGNED_BYTE, pixel_data);

    fwrite(TGAhead, sizeof(TGAhead), 1, out);
    fwrite(pixel_data, 4*W*H, 1, out);
    fclose(out);
}

void setProjectionMatrix(GLuint *prog, int screen_width, int screen_height){
    // Set projection uniform
    GLfloat projMat[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat left    = 0.0f;
    GLfloat right   = (GLfloat)screen_width;
    GLfloat bottom  = 0.0f;
    GLfloat top     = (GLfloat)screen_height;
    GLfloat nearVal = 0.0f;
    GLfloat farVal  = 1.0f;
    GLfloat tx      = -(right + left)/(right-left);
    GLfloat ty      = -(top + bottom)/(top-bottom);
    GLfloat tz      = -(farVal + nearVal)/(farVal - nearVal);

    projMat[0]  *= 2.0f / (right - left);
    projMat[5]  *= 2.0f / (top - bottom);
    projMat[10] *= -2.0f / (farVal - nearVal);
    projMat[12]  = tx;
    projMat[13]  = ty;
    projMat[14]  = tz;

    //printf("Ortho matrix:\n\
    //        %+1.3f %+1.3f %+1.3f %+1.3f\n\
    //        %+1.3f %+1.3f %+1.3f %+1.3f\n\
    //        %+1.3f %+1.3f %+1.3f %+1.3f\n\
    //        %+1.3f %+1.3f %+1.3f %+1.3f\n",
    //        projMat[0],   projMat[1],   projMat[2],   projMat[3],
    //        projMat[4],   projMat[5],   projMat[6],   projMat[7],
    //        projMat[8],   projMat[9],   projMat[10],  projMat[11],
    //        projMat[12],  projMat[13],  projMat[14],  projMat[15]);

    GLint loc = glGetUniformLocation(*prog, "projection");
    if(loc != -1){
        glUniformMatrix4fv(loc, 1, GL_FALSE, projMat);
    }
    else printf("Can't get projection uniform location\n");

}

static void key_pressed(GLFWwindow* window, int key, int scancode,
        int action,int mods){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static void window_resized(GLFWwindow* window, int width, int height){
    setProjectionMatrix(&program, width, height);
}

void error_callback(int error, const char* description){
    printf("Error: %s\n", description);
}

void setFontColor(float* colorVar, int colorChoice){
    switch(colorChoice){
        case 0: // Black
            colorVar[0] = 0.05f;
            colorVar[1] = 0.05f;
            colorVar[2] = 0.05f;
            break;
        case 1: // White
            colorVar[0] = 1.0f;
            colorVar[1] = 1.0f;
            colorVar[2] = 1.0f;
            break;
        case 2: // Light Grey
            colorVar[0] = 0.2f;
            colorVar[1] = 0.2f;
            colorVar[2] = 0.2f;
            break;
        case 3: // Dark Grey
            colorVar[0] = 0.7f;
            colorVar[1] = 0.7f;
            colorVar[2] = 0.7f;
            break;
        case 4: // Blue
            colorVar[0] = 0.0f;
            colorVar[1] = 0.0f;
            colorVar[2] = 1.0f;
            break;
        case 5: // Yellow
            colorVar[0] = 1.0f;
            colorVar[1] = 1.0f;
            colorVar[2] = 0.0f;
            break;
        case 6: // Purple
            colorVar[0] = 1.0f;
            colorVar[1] = 0.0f;
            colorVar[2] = 1.0f;
            break;
        case 7: // Cyan
            colorVar[0] = 0.0f;
            colorVar[1] = 1.0f;
            colorVar[2] = 1.0f;
            break;

        default: // Black
            colorVar[0] = 0.0f;
            colorVar[1] = 0.0f;
            colorVar[2] = 0.0f;
    }
}

void glPrintError(GLenum error){
    if(error == GL_NO_ERROR)
		printf("OpenGL Error: GL_NO_ERROR\n");
    else if(error == GL_INVALID_ENUM)
		printf("OpenGL Error: GL_INVALID_ENUM\n");
    else if(error == GL_INVALID_VALUE)
		printf("OpenGL Error: GL_INVALID_VALUE\n");
    else if(error == GL_INVALID_OPERATION)
		printf("OpenGL Error: GL_INVALID_OPERATION\n");
    else if(error == GL_INVALID_FRAMEBUFFER_OPERATION)
		printf("OpenGL Error: GL_INVALID_FRAMEBUFFER_OPERATION\n");
    else if(error == GL_OUT_OF_MEMORY)
		printf("OpenGL Error: GL_OUT_OF_MEMORY\n");
}
