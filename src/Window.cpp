#include <Window.h>

volatile sig_atomic_t twitConnectionInitialized = 0;
volatile sig_atomic_t autoCopy = 0;
raidList fileRaidNames;
GLuint VAO, VBO;
GLuint program;
int fontSizes[] = {36, 18, 16, 15, 14};
static const char *fontNames[] = {"PTS55F.ttf", "PTS75F.ttf"};
const int nFontNames = 2;
Character charArray[nFontNames][sizeof(fontSizes)/sizeof(int)][128];

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
    int screen_width = 535;
    int screen_height = 480;

    //printf("Convert 0x30cf = %2x%2x%2x\n", 
    //        (unsigned char)(codepointToUTF8(0x30cf)>>16),
    //        (unsigned char)(codepointToUTF8(0x30cf)>>8),
    //        (unsigned char)(codepointToUTF8(0x30cf)>>0));

    printf("Reading raid names from file\n");
    fileRaidNames.twitterOutputList = (char**)malloc(0);
    fileRaidNames.nTwitterOutputList = 0;
    fileRaidNames.raidNameList = getRaidNames(&(fileRaidNames.nRaidNameList));

    fileRaidNames.nRaidNameLegend = fileRaidNames.nRaidNameList>>1;
    fileRaidNames.raidNameLegend = (char**)malloc(sizeof(char*) * 
            fileRaidNames.nRaidNameLegend);
    fileRaidNames.legendShortcut = (char*)malloc(sizeof(char) *
            fileRaidNames.nRaidNameLegend);
    fileRaidNames.raidNameLegendOn = (int*)calloc(fileRaidNames.nRaidNameLegend,
            sizeof(int));
    for(unsigned int i = 1; i < fileRaidNames.nRaidNameList; i += 2){
        size_t nameLen = 0;
        unsigned int namePos = 0;
        
        while(fileRaidNames.raidNameList[i][nameLen] != '\0') ++nameLen;

        while(fileRaidNames.raidNameList[i][namePos] != ' ') ++namePos;
        ++namePos;
        while(fileRaidNames.raidNameList[i][namePos] != ' ') ++namePos;
        ++namePos;
    
        if(nameLen < namePos){
            printf("Something's wrong with the raidNames.txt formatting\n");
            printf("Expecting 2nd column to have format: \"Lvl xx Name\"\n");
            break;
        }

        fileRaidNames.raidNameLegend[i>>1] = (char*)malloc(nameLen-namePos + 1);
        strncpy(fileRaidNames.raidNameLegend[i>>1],
                fileRaidNames.raidNameList[i] + namePos,
                nameLen-namePos);
        fileRaidNames.raidNameLegend[i>>1][nameLen-namePos] = '\0';

        if((i>>1) < 26)
            fileRaidNames.legendShortcut[i>>1] = 'A' + (i>>1);
        else if((i>>1) < 52)
            fileRaidNames.legendShortcut[i>>1] = 'a' + (i>>1) - 26;

        //fileRaidNames.legendShortcut[i>>1] = 'A';

        //fileRaidNames.raidNameLegend[i>>1] = fileRaidNames.raidNameList[i];
    }

    printf("Flattening raid names list\n");
    char* raidNamesFlat = flattenStringArray(fileRaidNames.raidNameList,
            fileRaidNames.nRaidNameList,
            ",");
    printf("%s\n", raidNamesFlat);

    printf("Constructing twitter filter\n");
    char* twitFilter = (char*)malloc(sizeof("track=\"dummytxt,") +
            stringSize(raidNamesFlat));
    strcpy(twitFilter, "track=\"dummytxt,");
    strcat(twitFilter, raidNamesFlat);

    printf("Twitter Request:\n%s\n", twitFilter);
    ttwytter_request("POST",
            "https://stream.twitter.com/1.1/statuses/filter.json",
            twitFilter);

    // Wait until the twitter thread is established
    // Otherwise, things get screwy
    while(!twitConnectionInitialized);

    // Initialize the library
    printf("Initializing GLFW lib\n");
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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
    glfwSwapInterval(1);

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

    if(genFontTextures() == -1){
        printf("Problem loading fonts\n");
        return -1;
    }

    // load an image file directly as a new OpenGL texture
    GLuint bgTex = SOIL_load_OGL_texture(
         "naru.png",
         SOIL_LOAD_AUTO,
         SOIL_CREATE_NEW_ID,
         SOIL_FLAG_MIPMAPS | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
        );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // check for an error during the load process
    if(0 == bgTex)
        printf("SOIL loading error: '%s'\n", SOIL_last_result());
    else printf("Background img loaded sucessfully\n");

    // Loop until the user closes the window
    printf("Starting main GLFW Loop\n");
    while (!glfwWindowShouldClose(window)){
        // Render here
        glClearColor(0.92f, 0.92f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwGetFramebufferSize(window, &screen_width, &screen_height);
        glViewport(0, 0, screen_width, screen_height);

        //setFontColor(fontColor, 1);
        //renderText(&program, text, 20.0f, 200.0f, 0.7f, fontColor);

        drawMainMenu(&program, screen_width, screen_height, bgTex);
        //printf("Timer: %f\n", glfwGetTime());

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void drawMainMenu(GLuint* prog, int width, int height, GLuint bgTex){
    // FONTSIZE_BASE = 36PX
    float fontColor[3];

    // Draw background image
    glUniform3f(glGetUniformLocation(program, "textColor"), 1.0, 1.0, 1.0);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    GLfloat bgVerts[6][4] = {
        { 0              , (GLfloat)height , 0.0 , 0.0 } ,
        { 0              , 0               , 0.0 , 1.0 } ,
        { (GLfloat)width , 0               , 1.0 , 1.0 } ,

        { 0              , (GLfloat)height , 0.0 , 0.0 } ,
        { (GLfloat)width , 0               , 1.0 , 1.0 } ,
        { (GLfloat)width , (GLfloat)height , 1.0 , 0.0 }
    };
    glBindTexture(GL_TEXTURE_2D, bgTex);
    // Update content of VBO memory
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bgVerts), bgVerts);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Render quad
    glDrawArrays(GL_TRIANGLES, 0, 6);
    // Release hold
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Draw text data
    int headerLocX = 15;
    int headerLocY = height-50;
    setFontColor(fontColor, 0);
    renderText(prog, "GBF Raid IDs", headerLocX, headerLocY, 1, 0, fontColor);
    renderText(prog,
            "Press the key next to the boss' names to filter incoming IDs",
            headerLocX, headerLocY-25, 0, 2, fontColor);

    //int selectLocX = 15;
    //int selectLocY = height-115-20*raidNamesLength;
    int selectLocX = 200;
    int selectLocY = height-105;
    renderText(prog, "-1- Select All",
            selectLocX, selectLocY,0, 2, fontColor);
    renderText(prog, "-2- Select None",
            selectLocX, selectLocY-20, 0, 2, fontColor);
    setFontColor(fontColor, 3);
    renderText(prog, "-3- Toggle Sound",
            selectLocX, selectLocY-40, 0, 2, fontColor);

    if(autoCopy) setFontColor(fontColor, 0);
    else setFontColor(fontColor, 3);
    renderText(prog, "-4- Toggle Auto-Copy",
            selectLocX, selectLocY-60, 0, 2, fontColor);

    int tableLocX = 15;
    int tableLocY = height - 105;
    int tableCheckCharacterLength = 25;

    for(unsigned int i = 0; i < fileRaidNames.nRaidNameLegend; i++){
        char *raidShortcutDisplay = (char*)malloc(
                2*strlen("(") + // Make space for parenthesis
                sizeof(char) +  // Make space for shortcut char
                1);             // Make space for null termination
        strcpy(raidShortcutDisplay, "-");
        char shortcutDisplay[2] = {fileRaidNames.legendShortcut[i], '\0'};
        strcat(raidShortcutDisplay, shortcutDisplay);
        strcat(raidShortcutDisplay, "-");

        if(fileRaidNames.raidNameLegendOn[i])
            setFontColor(fontColor, 0);
        else
            setFontColor(fontColor, 3);
        renderText(prog, raidShortcutDisplay, tableLocX, tableLocY - 20*i,
                0, 2, fontColor);

        renderText(prog, fileRaidNames.raidNameLegend[i],
                tableLocX + tableCheckCharacterLength,
                tableLocY - 20*i, 0, 2, fontColor);

        free(raidShortcutDisplay);
    }

    int raidListLocX = 200;
    int raidListLocY = height-105-5*20;
    int raidListElemLocX = 210;
    int raidListElemLocY = height-105-5*20-20;
    int listLineSizeY = 19;
    setFontColor(fontColor, 0);
    renderText(prog, "Raid List:", raidListLocX, raidListLocY, 1, 2, fontColor);

    int minDisplayElement = 0;
    unsigned int nElementsToDisplay = 8;
    if(fileRaidNames.nTwitterOutputList > nElementsToDisplay)
        minDisplayElement =
            fileRaidNames.nTwitterOutputList - nElementsToDisplay;

    for(int i = fileRaidNames.nTwitterOutputList-1;
            i >= minDisplayElement; i--){
        setFontColor(fontColor, 0);
        renderText(prog, fileRaidNames.twitterOutputList[i], raidListElemLocX,
                raidListElemLocY-listLineSizeY*
                (fileRaidNames.nTwitterOutputList-i-1), 0, 4, fontColor);
    }

}

const char *ttwytter_request(const char *http_method, const char *url,
        const char *url_enc_args){

    struct curl_slist *slist = NULL;
    struct curlArgs pthreadArgs;
    const char *consumer_key = 
        "u0F80FdfqkskJPj43N8qzDi6w";
    const char *consumer_secret = 
        "EQovmAAo5MQIuHy0MvXNL6vJdF5j34DrcCw1lWxUKVIooSjU5g";
    const char *user_token = 
        "2784168906-L6G5msk0FCu5SbPcajGirYaUiEQFFm15ToehdSB";
    const char *user_secret = 
        "ocfwu7wSfDAWB5mja5IqaFdiGx7y20PEYNET6yCHkozUr";

    char *ser_url, **argv, *auth_params,
         auth_header[1024], *non_auth_params, *final_url, *temp_url;
    int argc;
    pthread_t tid[1];

    ser_url = (char*)malloc(strlen(url) + strlen(url_enc_args) + 2);
    sprintf(ser_url, "%s?%s", url, url_enc_args);

    argv = (char**)malloc(0);
    argc = oauth_split_url_parameters(ser_url, &argv);
    free(ser_url);

    temp_url = oauth_sign_array2(&argc,
            &argv,
            NULL,
            OA_HMAC,
            http_method,
            consumer_key,
            consumer_secret,
            user_token,
            user_secret);
    free(temp_url);

    auth_params = oauth_serialize_url_sep(argc, 1, argv, ", ", 6);
    sprintf(auth_header, "Authorization: OAuth %s", auth_params);
    slist = curl_slist_append(slist, auth_header);
    free(auth_params);

    non_auth_params = oauth_serialize_url_sep(argc, 1, argv, "", 1);

    final_url = (char*)malloc(strlen(url) + strlen(non_auth_params));

    strcpy(final_url, url);
    char *postData = non_auth_params;

    for (int i = 0; i < argc; i++ ){
        free(argv[i]);   
    }
    free(argv);

    curl_global_init(CURL_GLOBAL_ALL);

    pthreadArgs.arg1 = url;
    pthreadArgs.arg2 = slist;
    pthreadArgs.arg3 = postData;
    pthreadArgs.arg4 = 30;

    int pthreadError;
    pthreadError = pthread_create(&tid[0],
            NULL,
            initTwitterConnection,
            (void*)&pthreadArgs);

    if(pthreadError != 0) printf("Problem creating thread");

    return "a";
}


static void *initTwitterConnection(void *arguments){
    struct curlArgs *twitArgs = (curlArgs*)arguments;
    CURL *curl;

    struct curlArgs vtwitArgs;
    vtwitArgs.arg1 = twitArgs->arg1;
    vtwitArgs.arg2 = twitArgs->arg2;
    vtwitArgs.arg3 = twitArgs->arg3;
    vtwitArgs.arg4 = twitArgs->arg4;

    const char* url         = vtwitArgs.arg1;
    curl_slist* slist       = vtwitArgs.arg2;
    char* postData          = vtwitArgs.arg3;
    unsigned int retryDelay = vtwitArgs.arg4;

    // CURL Data loaded into thread. Main thread can now continue
    twitConnectionInitialized = 1;

    curl = curl_easy_init();

    CURLcode curlError;

    printf("CURL URL: %s\n", url);
    printf("CURL TIMEOUT: %i seconds\n", retryDelay);

    printf("Setting CURLOPT_URL\t\t");
    curlError = curl_easy_setopt(curl, CURLOPT_URL, url);
    curlPrintError(curlError);

    printf("Setting CURLOPT_USERAGENT\t");
    curlError = curl_easy_setopt(curl, CURLOPT_USERAGENT, "dummy-string");
    curlPrintError(curlError);

    printf("Setting CURLOPT_HTTPHEADER\t");
    curlError = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    curlPrintError(curlError);

    printf("Setting CURLOPT_POST\t\t");
    curlError = curl_easy_setopt(curl, CURLOPT_POST, 1);
    curlPrintError(curlError);

    printf("Setting CURLOPT_POSTFIELDS\t");
    curlError = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
    curlPrintError(curlError);

    printf("Nulling SSL Peer Verification\t");
    curlError = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curlPrintError(curlError);

    printf("Nulling SSL Host Verification\t");
    curlError = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    curlPrintError(curlError);

    printf("Setting CURL callback func\t");
    curlError = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curlPrintError(curlError);

    printf("Executing CURL req\t\t\n");
    curlPrintError(curl_easy_perform(curl)); /* Execute the request! */

    curl_easy_cleanup(curl);

    printf("Twitter connection destroyed. Waiting %i seconds\n", retryDelay);
    sleep(retryDelay);

    vtwitArgs.arg4 = vtwitArgs.arg4 * 2;

    initTwitterConnection((void*)&vtwitArgs);

    return NULL;
}

static size_t write_callback(void *ptr, size_t size,
        size_t nmemb, void *userdata){

    size_t realSize = size * nmemb;

    char* data = (char*)malloc(realSize+1); // +1 for NULL ending
    strcpy(data, (char*)ptr);

    printf("*** We read %u bytes from file\n", realSize);
    printf("%s\n\n", data);
    char* offset = strstr(data, "ID");
    int jump;
    if(offset != NULL){
        char* ID = (char*)malloc(sizeof(char)*8 + 1);
        unsigned int validID = 1;

        //printf("Character: %c\n", offset[2]);
        if(offset[2] == ':') jump = 4;
        else jump = 8;

        strncpy(ID, offset+jump, 8);
        ID[8] = '\0';
        
        for(unsigned int i = 0; i < 8; i++){
            if((ID[i] >= 'A' && ID[i] <= 'F') ||
                    (ID[i] >= '0' && ID[i] <= '9'));
            else{
                validID = 0;
                printf("INVALID ID READ\n");
                break;
            }
        }

        if(validID){
            printf("ID: %s\n", ID);
            findBoss(data, realSize, ID);
        }
        free(ID);
    }

    free(data);
    return realSize;
}

char* findBoss(char* jsonData, size_t jsonDataSize, char* ID){
    size_t actualDataSize = 0;
    size_t dataIndex = 0;
    char* retData = (char*)malloc(jsonDataSize);
    char* displayLine = NULL;
    char* unicodeString = (char*)malloc(sizeof(char)*4 + 1);
    char* unicodeStringUpper = (char*)malloc(sizeof(char)*2 + 1);
    char* unicodeStringLower = (char*)malloc(sizeof(char)*2 + 1);

    while(dataIndex < jsonDataSize){
        if(jsonData[dataIndex] == '\\' && jsonData[dataIndex+1] == 'u'){
            strncpy(unicodeString, jsonData+dataIndex+2, 4);
            strncpy(unicodeStringUpper, unicodeString, 2);
            strncpy(unicodeStringLower, unicodeString+2, 2);
            unicodeStringUpper[2] = '\0';
            unicodeStringLower[2] = '\0';

            //char unicodeCode[2];
            //unicodeCode[1] = (int)strtol(unicodeStringUpper, NULL, 16);
            //unicodeCode[0] = (int)strtol(unicodeStringLower, NULL, 16);
            uint16_t unicodeCode_16 = 
                ((uint16_t)strtol(unicodeStringUpper, NULL, 16)<<8) | 
                ((uint16_t)strtol(unicodeStringLower, NULL, 16)<<0);

            //printf("Unicode string: %s\nUnicode point: %s%s\n\n",
            //        unicodeString, unicodeStringUpper, unicodeStringLower);

            retData[actualDataSize+0] = codepointToUTF8(unicodeCode_16)>>16;
            retData[actualDataSize+1] = codepointToUTF8(unicodeCode_16)>>8;
            retData[actualDataSize+2] = codepointToUTF8(unicodeCode_16);

            //printf("Convert %2x%2x= %lx\n",
            //        (unsigned char)unicodeCode[1],
            //        (unsigned char)unicodeCode[0],
            //        (unsigned long)codepointToUTF8(unicodeCode_16));

            actualDataSize += 3; // Converted utf8 code adds 3 bytes
            dataIndex += 6; // 6 bytes of rec text parsed
        }
        else{
            //printf("%c\n", jsonData[dataIndex]);
            retData[actualDataSize] = jsonData[dataIndex];
            actualDataSize++;
            dataIndex++;
        }
    }

    unsigned int bossIndex = 0;
    while(bossIndex < fileRaidNames.nRaidNameList){
        if(strstr(retData, fileRaidNames.raidNameList[bossIndex]) != NULL){
            if(bossIndex%2 != 1) bossIndex++;
            printf("Boss: %s\n", fileRaidNames.raidNameList[bossIndex]);
            break;
        }

        bossIndex++;
    }

    if(fileRaidNames.raidNameLegendOn[bossIndex>>1]){
        size_t bossNameLength = 0;
        while(fileRaidNames.raidNameList[bossIndex][bossNameLength] != '\0')
            bossNameLength++;

        const char* displayLineFormat = "ID: %s - %s";
        size_t displayLineFormatSize = strlen(displayLineFormat) +
            bossNameLength + 8 + 1; // 8 = ID size, 1 for Null termination
        displayLine = (char*)malloc(displayLineFormatSize); 

        snprintf(displayLine, displayLineFormatSize, displayLineFormat,
                ID, fileRaidNames.raidNameList[bossIndex]);

        if(autoCopy) setClipboard(ID, 9);

        fileRaidNames.nTwitterOutputList++;
        fileRaidNames.twitterOutputList = 
            (char**)realloc(fileRaidNames.twitterOutputList,
                    fileRaidNames.nTwitterOutputList * sizeof(char*));
        fileRaidNames.twitterOutputList[fileRaidNames.nTwitterOutputList-1]
            = displayLine;

        printf("%s\n",
                fileRaidNames.twitterOutputList[fileRaidNames.nTwitterOutputList
                -1]);
    }

    free(unicodeString);
    free(unicodeStringUpper);
    free(unicodeStringLower);

    printf("%s\n", retData);

    free(retData);
    return (char*)NULL;
}

uint32_t codepointToUTF8(uint16_t codepoint){
    uint32_t output   = 0x000000;
    uint16_t lowMask  = 0b0000000000111111;
    uint16_t midMask  = 0b0000111111000000;
    uint16_t highMask = 0b1111000000000000;

    //printf("Test: %x\n", (0b11100000 | (codepoint & highMask)>>12));

    output = (0b10000000 | (codepoint & lowMask)) |
             (0b10000000 | (codepoint & midMask)>>6)<<8 |
             (0b11100000 | (codepoint & highMask)>>12)<<16;

    return output;
}

char** getRaidNames(unsigned int *nRaidNames){
    char* fileRaidNamesFlat = NULL;
    // Read raid Names from file
    FILE *sourceFile = fopen("raidNames.txt", "r");
    if(sourceFile == NULL) printf("Can't open names file\n");
    else printf("Opened names file\n");
    // Get file size
    fseek(sourceFile, 0, SEEK_END);
    size_t readSize = ftell(sourceFile);
    // Allocate space for boss names
    fileRaidNamesFlat = (char*)malloc(sizeof(char)*readSize + 2);
    if(fileRaidNamesFlat == NULL) printf("Memory error - names.txt\n");
    // Read file into source variable
    rewind(sourceFile);
    fread(fileRaidNamesFlat, 1, readSize, sourceFile);
    printf("%s\n", fileRaidNamesFlat);
    //for(unsigned int i = 0; i < readSize; i++){
    //    printf("%c\t%x\n", fileRaidNamesFlat[i],
    //            (unsigned char)fileRaidNamesFlat[i]);
    //}

    unsigned int nNames = 0;
    for(unsigned int i = 0; i < readSize; i++){
        if(fileRaidNamesFlat[i] == '\n' || fileRaidNamesFlat[i] == ',')
            nNames++;
    }

    char **twitNames;
    twitNames = (char**)malloc(nNames * sizeof(char*));

    char* pBossName = strtok(fileRaidNamesFlat, ",\n");
    unsigned int twitNamesIndex = 0;
    while(twitNamesIndex < nNames){
        twitNames[twitNamesIndex] = pBossName;
        twitNamesIndex++;
        pBossName = strtok(NULL, ",\n");
    }

    //printf("Number of names: %i\n", nNames);
    //for(unsigned int i = 0; i < nNames; i++)
    //    printf("Number: %i\n\t%s\n", i, twitNames[i]);
    
    *nRaidNames = nNames;
    return twitNames;
}

void renderText(GLuint* prog, const char* text, GLfloat x, GLfloat y,
        unsigned int selectedFont, unsigned int fontSizeIndex, float* color){

    // Activate corresponding render state
    glUniform3f(glGetUniformLocation(*prog, "textColor"), color[0],
            color[1], color[2]);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Iterate through all characters
    for(size_t c = 0; c < strlen(text); c++){
        Character ch = charArray[selectedFont][fontSizeIndex][(size_t)text[c]];
        //printf("Character info - %c\nSize: %f, %f\nBearing: %f, %f
        //        \nAdvance: %i\nTexture ID: %i\n\n", 
        //        text[c], ch.Size[0], ch.Size[1], ch.Bearing[0],
        //        ch.Bearing[1], ch.Advance, ch.TextureID);

        GLfloat xpos = x + ch.Bearing[0];
        GLfloat ypos = y - (ch.Size[1] - ch.Bearing[1]);

        GLfloat w = ch.Size[0];
        GLfloat h = ch.Size[1];
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
        x += (ch.Advance >> 6); // Bitshift by 6 to get value in pixels
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void setClipboard(char* text, unsigned int len){
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), text, len);
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}


static int genFontTextures(){
    FT_Library fontLibrary;
    FT_Face fontFace;
    FT_Error error;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for(unsigned int n = 0; n < nFontNames; n++){
        // Load font library
        printf("Initializing FreeType\n");
        error = FT_Init_FreeType(&fontLibrary);
        if(error){
            printf("Cannot load FreeType library\n");
            return -1;
        }

        // Load font file into library
        printf("Loading font file\n");
        error = FT_New_Face(fontLibrary, fontNames[n], 0, &fontFace);
        if(error == FT_Err_Unknown_File_Format){
            printf("Font opened, but format is unknown\n");
        }
        else if(error){
            printf("Font file cannot be read - %s", fontNames[n]);
        }

        for(unsigned int i = 0; i < sizeof(fontSizes)/sizeof(int); i++){
            // Set font size
            FT_Set_Pixel_Sizes(fontFace, 0, fontSizes[i]);

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
                charArray[n][i][c].TextureID = texture;
                charArray[n][i][c].Size[0] = fontFace->glyph->bitmap.width;
                charArray[n][i][c].Size[1] = fontFace->glyph->bitmap.rows;
                charArray[n][i][c].Bearing[0] = fontFace->glyph->bitmap_left;
                charArray[n][i][c].Bearing[1] = fontFace->glyph->bitmap_top;
                charArray[n][i][c].Advance = fontFace->glyph->advance.x;

                //texDump(&texture, charArray[c].Size[0], charArray[c].Size[1]);
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    FT_Done_Face(fontFace);
    FT_Done_FreeType(fontLibrary);

    return 0;
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

static char* flattenStringArray(char** strArray,
        size_t nElements, const char* sep){

    char* retString;
    const char* dummy = "";
    size_t strArraySize = 0;
    
    // Count total size of input array
    for(unsigned int i = 0; i < nElements; i++)
        strArraySize += strlen(strArray[i]);

    // Allocate space for return string
    retString = (char*)malloc(strArraySize + // Space for all the names
            (nElements-1)*sizeof(char) + // Space for the seperator
            3); // Space for null termination + 2 " characters

    // Initialize return string w/ first value
    //strcpy(retString, "");
    strcpy(retString, strArray[0]);

    // Loop through input array outputting them into return value
    for(size_t i = 1; i < nElements; i++){
        // Get number of characters to write into return string
        size_t nChars = snprintf((char*)dummy, 0, "%s,%s",
                retString, (char*)strArray[i]);
        // Actually write into return string
        snprintf(retString, nChars+1, "%s%s%s", retString,
                (char*)sep, (char*)strArray[i]);
    }

    strcat(retString, "\"");
    return retString;
}

static size_t stringSize(const char* ary){
    size_t flatSize = 0;
    while(ary[flatSize] != '\0') ++flatSize;
    return flatSize;
}

static void key_pressed(GLFWwindow* window, int key, int scancode,
        int action,int mods){
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS){
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    for(unsigned int i = 0; i < fileRaidNames.nRaidNameLegend; i++){
        if(key == fileRaidNames.legendShortcut[i] && action == GLFW_PRESS){
            printf("Key pressed: %c\n", key);

            if(fileRaidNames.raidNameLegendOn[i]){
                printf("Toggled Off\n");
                fileRaidNames.raidNameLegendOn[i] = 0;
            }
            else{
                printf("Toggled On\n");
                fileRaidNames.raidNameLegendOn[i] = 1;
            }
        } 
    }
    if(key == '1' && action == GLFW_PRESS){
        for(unsigned int i = 0; i < fileRaidNames.nRaidNameLegend; i++){
            fileRaidNames.raidNameLegendOn[i] = 1;
        }
    }
    if(key == '2' && action == GLFW_PRESS){
        for(unsigned int i = 0; i < fileRaidNames.nRaidNameLegend; i++){
            fileRaidNames.raidNameLegendOn[i] = 0;
        }
    }
    if(key == '4' && action == GLFW_PRESS){
        if(autoCopy) autoCopy = 0;
        else autoCopy = 1;
        printf("AutoCopy: %i\n", autoCopy);
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
            colorVar[0] = 0.1f;
            colorVar[1] = 0.1f;
            colorVar[2] = 0.1f;
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

void curlPrintError(CURLcode curlstatus){
    if(curlstatus == CURLE_OK)
        printf("CURLE_OK\n");
    else if(curlstatus == CURLE_UNSUPPORTED_PROTOCOL)
        printf("CURLE_UNSUPPORTED_PROTOCOL\n");
    else if(curlstatus == CURLE_FAILED_INIT)
        printf("CURLE_FAILED_INIT\n");
    else if(curlstatus == CURLE_URL_MALFORMAT)
        printf("CURLE_URL_MALFORMAT\n");
    else if(curlstatus == CURLE_NOT_BUILT_IN)
        printf("CURLE_NOT_BUILT_IN\n");
    else if(curlstatus == CURLE_COULDNT_RESOLVE_PROXY)
        printf("CURLE_COULDNT_RESOLVE_PROXY\n");
    else if(curlstatus == CURLE_COULDNT_RESOLVE_HOST)
        printf("CURLE_COULDNT_RESOLVE_HOST\n");
    else if(curlstatus == CURLE_COULDNT_CONNECT)
        printf("CURLE_COULDNT_CONNECT\n");
    else if(curlstatus == CURLE_FTP_WEIRD_SERVER_REPLY)
        printf("CURLE_FTP_WEIRD_SERVER_REPLY\n");
    else if(curlstatus == CURLE_REMOTE_ACCESS_DENIED)
        printf("CURLE_REMOTE_ACCESS_DENIED\n");
    else if(curlstatus == CURLE_FTP_ACCEPT_FAILED)
        printf("CURLE_FTP_ACCEPT_FAILED\n");
    else if(curlstatus == CURLE_FTP_WEIRD_PASS_REPLY)
        printf("CURLE_FTP_WEIRD_PASS_REPLY\n");
    else if(curlstatus == CURLE_FTP_ACCEPT_TIMEOUT)
        printf("CURLE_FTP_ACCEPT_TIMEOUT\n");
    else if(curlstatus == CURLE_FTP_WEIRD_PASV_REPLY)
        printf("CURLE_FTP_WEIRD_PASV_REPLY\n");
    else if(curlstatus == CURLE_FTP_WEIRD_227_FORMAT)
        printf("CURLE_FTP_WEIRD_227_FORMAT\n");
    else if(curlstatus == CURLE_FTP_CANT_GET_HOST)
        printf("CURLE_FTP_CANT_GET_HOST\n");
    else if(curlstatus == CURLE_HTTP2)
        printf("CURLE_HTTP2\n");
    else if(curlstatus == CURLE_FTP_COULDNT_SET_TYPE)
        printf("CURLE_FTP_COULDNT_SET_TYPE\n");
    else if(curlstatus == CURLE_PARTIAL_FILE)
        printf("CURLE_PARTIAL_FILE\n");
    else if(curlstatus == CURLE_FTP_COULDNT_RETR_FILE)
        printf("CURLE_FTP_COULDNT_RETR_FILE\n");
    else if(curlstatus == CURLE_QUOTE_ERROR)
        printf("CURLE_QUOTE_ERROR\n");
    else if(curlstatus == CURLE_HTTP_RETURNED_ERROR)
        printf("CURLE_HTTP_RETURNED_ERROR\n");
    else if(curlstatus == CURLE_WRITE_ERROR)
        printf("CURLE_WRITE_ERROR\n");
    else if(curlstatus == CURLE_UPLOAD_FAILED)
        printf("CURLE_UPLOAD_FAILED\n");
    else if(curlstatus == CURLE_READ_ERROR)
        printf("CURLE_READ_ERROR\n");
    else if(curlstatus == CURLE_OUT_OF_MEMORY)
        printf("CURLE_OUT_OF_MEMORY\n");
    else if(curlstatus == CURLE_OPERATION_TIMEDOUT)
        printf("CURLE_OPERATION_TIMEDOUT\n");
    else if(curlstatus == CURLE_FTP_PORT_FAILED)
        printf("CURLE_FTP_PORT_FAILED\n");
    else if(curlstatus == CURLE_FTP_COULDNT_USE_REST)
        printf("CURLE_FTP_COULDNT_USE_REST\n");
    else if(curlstatus == CURLE_RANGE_ERROR)
        printf("CURLE_RANGE_ERROR\n");
    else if(curlstatus == CURLE_HTTP_POST_ERROR)
        printf("CURLE_HTTP_POST_ERROR\n");
    else if(curlstatus == CURLE_SSL_CONNECT_ERROR)
        printf("CURLE_SSL_CONNECT_ERROR\n");
    else if(curlstatus == CURLE_BAD_DOWNLOAD_RESUME)
        printf("CURLE_BAD_DOWNLOAD_RESUME\n");
    else if(curlstatus == CURLE_FILE_COULDNT_READ_FILE)
        printf("CURLE_FILE_COULDNT_READ_FILE\n");
    else if(curlstatus == CURLE_LDAP_CANNOT_BIND)
        printf("CURLE_LDAP_CANNOT_BIND\n");
    else if(curlstatus == CURLE_LDAP_SEARCH_FAILED)
        printf("CURLE_LDAP_SEARCH_FAILED\n");
    else if(curlstatus == CURLE_FUNCTION_NOT_FOUND)
        printf("CURLE_FUNCTION_NOT_FOUND\n");
    else if(curlstatus == CURLE_ABORTED_BY_CALLBACK)
        printf("CURLE_ABORTED_BY_CALLBACK\n");
    else if(curlstatus == CURLE_BAD_FUNCTION_ARGUMENT)
        printf("CURLE_BAD_FUNCTION_ARGUMENT\n");
    else if(curlstatus == CURLE_INTERFACE_FAILED)
        printf("CURLE_INTERFACE_FAILED\n");
    else if(curlstatus == CURLE_TOO_MANY_REDIRECTS)
        printf("CURLE_TOO_MANY_REDIRECTS\n");
    else if(curlstatus == CURLE_UNKNOWN_OPTION)
        printf("CURLE_UNKNOWN_OPTION\n");
    else if(curlstatus == CURLE_TELNET_OPTION_SYNTAX)
        printf("CURLE_TELNET_OPTION_SYNTAX\n");
    else if(curlstatus == CURLE_PEER_FAILED_VERIFICATION)
        printf("CURLE_PEER_FAILED_VERIFICATION\n");
    else if(curlstatus == CURLE_GOT_NOTHING)
        printf("CURLE_GOT_NOTHING\n");
    else if(curlstatus == CURLE_SSL_ENGINE_NOTFOUND)
        printf("CURLE_SSL_ENGINE_NOTFOUND\n");
    else if(curlstatus == CURLE_SSL_ENGINE_SETFAILED)
        printf("CURLE_SSL_ENGINE_SETFAILED\n");
    else if(curlstatus == CURLE_SEND_ERROR)
        printf("CURLE_SEND_ERROR\n");
    else if(curlstatus == CURLE_RECV_ERROR)
        printf("CURLE_RECV_ERROR\n");
    else if(curlstatus == CURLE_SSL_CERTPROBLEM)
        printf("CURLE_SSL_CERTPROBLEM\n");
    else if(curlstatus == CURLE_SSL_CIPHER)
        printf("CURLE_SSL_CIPHER\n");
    else if(curlstatus == CURLE_SSL_CACERT)
        printf("CURLE_SSL_CACERT\n");
    else if(curlstatus == CURLE_BAD_CONTENT_ENCODING)
        printf("CURLE_BAD_CONTENT_ENCODING\n");
    else if(curlstatus == CURLE_LDAP_INVALID_URL)
        printf("CURLE_LDAP_INVALID_URL\n");
    else if(curlstatus == CURLE_FILESIZE_EXCEEDED)
        printf("CURLE_FILESIZE_EXCEEDED\n");
    else if(curlstatus == CURLE_USE_SSL_FAILED)
        printf("CURLE_USE_SSL_FAILED\n");
    else if(curlstatus == CURLE_SEND_FAIL_REWIND)
        printf("CURLE_SEND_FAIL_REWIND\n");
    else if(curlstatus == CURLE_SSL_ENGINE_INITFAILED)
        printf("CURLE_SSL_ENGINE_INITFAILED\n");
    else if(curlstatus == CURLE_LOGIN_DENIED)
        printf("CURLE_LOGIN_DENIED\n");
    else if(curlstatus == CURLE_TFTP_NOTFOUND)
        printf("CURLE_TFTP_NOTFOUND\n");
    else if(curlstatus == CURLE_TFTP_PERM)
        printf("CURLE_TFTP_PERM\n");
    else if(curlstatus == CURLE_REMOTE_DISK_FULL)
        printf("CURLE_REMOTE_DISK_FULL\n");
    else if(curlstatus == CURLE_TFTP_ILLEGAL)
        printf("CURLE_TFTP_ILLEGAL\n");
    else if(curlstatus == CURLE_TFTP_UNKNOWNID)
        printf("CURLE_TFTP_UNKNOWNID\n");
    else if(curlstatus == CURLE_REMOTE_FILE_EXISTS)
        printf("CURLE_REMOTE_FILE_EXISTS\n");
    else if(curlstatus == CURLE_TFTP_NOSUCHUSER)
        printf("CURLE_TFTP_NOSUCHUSER\n");
    else if(curlstatus == CURLE_CONV_FAILED)
        printf("CURLE_CONV_FAILED\n");
    else if(curlstatus == CURLE_CONV_REQD)
        printf("CURLE_CONV_REQD\n");
    else if(curlstatus == CURLE_SSL_CACERT_BADFILE)
        printf("CURLE_SSL_CACERT_BADFILE\n");
    else if(curlstatus == CURLE_REMOTE_FILE_NOT_FOUND)
        printf("CURLE_REMOTE_FILE_NOT_FOUND\n");
    else if(curlstatus == CURLE_SSH)
        printf("CURLE_SSH\n");
    else if(curlstatus == CURLE_SSL_SHUTDOWN_FAILED)
        printf("CURLE_SSL_SHUTDOWN_FAILED\n");
    else if(curlstatus == CURLE_AGAIN)
        printf("CURLE_AGAIN\n");
    else if(curlstatus == CURLE_SSL_CRL_BADFILE)
        printf("CURLE_SSL_CRL_BADFILE\n");
    else if(curlstatus == CURLE_SSL_ISSUER_ERROR)
        printf("CURLE_SSL_ISSUER_ERROR\n");
    else if(curlstatus == CURLE_FTP_PRET_FAILED)
        printf("CURLE_FTP_PRET_FAILED\n");
    else if(curlstatus == CURLE_RTSP_CSEQ_ERROR)
        printf("CURLE_RTSP_CSEQ_ERROR\n");
    else if(curlstatus == CURLE_RTSP_SESSION_ERROR)
        printf("CURLE_RTSP_SESSION_ERROR\n");
    else if(curlstatus == CURLE_FTP_BAD_FILE_LIST)
        printf("CURLE_FTP_BAD_FILE_LIST\n");
    else if(curlstatus == CURLE_CHUNK_FAILED)
        printf("CURLE_CHUNK_FAILED\n");
    else if(curlstatus == CURLE_NO_CONNECTION_AVAILABLE)
        printf("CURLE_NO_CONNECTION_AVAILABLE\n");
    else if(curlstatus == CURLE_SSL_PINNEDPUBKEYNOTMATCH)
        printf("CURLE_SSL_PINNEDPUBKEYNOTMATCH\n");
    else if(curlstatus == CURLE_SSL_INVALIDCERTSTATUS)
        printf("CURLE_SSL_INVALIDCERTSTATUS\n");
    else if(curlstatus == CURLE_HTTP2_STREAM)
        printf("CURLE_HTTP2_STREAM\n");
}
