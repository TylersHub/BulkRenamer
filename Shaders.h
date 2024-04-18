#pragma once
#ifndef Shaders
#define Shaders

// Initializing FreeType and Shaders

GLuint textShaderProgram; // Shader program for rendering text
GLint projectionLoc, textColorLoc; // Uniform locations for projection matrix and text color
GLuint textVAO, textVBO; // Vertex array object and vertex buffer object for text rendering

FT_Library ft; // FreeType library variable
FT_Face face; // FreeType font face variable

glm::mat4 projectionMatrix;

// Function to initialize FreeType
void initFreeType() {
    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Failed to initialize FreeType\n");
        glfwTerminate();
        exit(-1);
    }

    if (FT_New_Face(ft, "trebuc.ttf", 0, &face)) {
        fprintf(stderr, "Failed to load font\n");
        FT_Done_FreeType(ft); // Cleanup FreeType on failure
        glfwTerminate();
        exit(-1);
    }

    FT_Set_Pixel_Sizes(face, 0, 48); // Set font size
}

// Function to initialize shaders
void initShaders() {
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>\n"
        "out vec2 TexCoords;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
        "    TexCoords = vertex.zw;\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "in vec2 TexCoords;\n"
        "out vec4 color;\n"
        "uniform sampler2D text;\n"
        "uniform vec3 textColor;\n"
        "void main()\n"
        "{\n"
        "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);\n"
        "    color = vec4(textColor, 1.0) * sampled;\n"
        "}\n\0";

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // Check for compile errors
    GLint vertexCompileStatus;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vertexCompileStatus);
    if (vertexCompileStatus != GL_TRUE) {
        GLint logLength;
        glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(logLength);
        glGetShaderInfoLog(vertexShader, logLength, NULL, log.data());
        std::cerr << "Vertex shader compilation failed:\n" << log.data() << std::endl;
        // Optionally, you can also delete the shader object here
        //glDeleteShader(vertexShader);
    }
    else if (vertexCompileStatus == GL_TRUE) {
        std::cout << "Vertex shader compiled successfully" << std::endl;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // Check for compile errors
    GLint fragmentCompileStatus;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fragmentCompileStatus);
    if (fragmentCompileStatus == GL_FALSE) {
        // Compilation failed, retrieve and print the compilation log
        GLint logLength;
        glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(logLength);
        glGetShaderInfoLog(fragmentShader, logLength, nullptr, log.data());
        std::cerr << "Fragment shader compilation failed:\n" << log.data() << std::endl;
    }
    else if (fragmentCompileStatus == GL_TRUE) {
        std::cout << "Fragment shader compiled successfully" << std::endl;
    }

    // Link shaders
    textShaderProgram = glCreateProgram();
    glAttachShader(textShaderProgram, vertexShader);
    glAttachShader(textShaderProgram, fragmentShader);
    glLinkProgram(textShaderProgram);
    // Check for linking errors
    GLint linkStatus;
    glGetProgramiv(textShaderProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        // Linking failed, retrieve and print the linking log
        GLint logLength;
        glGetProgramiv(textShaderProgram, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> log(logLength);
        glGetProgramInfoLog(textShaderProgram, logLength, nullptr, log.data());
        std::cerr << "Shader program linking failed:\n" << log.data() << std::endl;
    }
    else if (linkStatus == GL_TRUE) {
        std::cout << "Shader program linked successfully" << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get uniform locations
    projectionLoc = glGetUniformLocation(textShaderProgram, "projection");
    textColorLoc = glGetUniformLocation(textShaderProgram, "textColor");

    // Set up vertex buffer objects and vertex array objects
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Function to render text
void renderText(float x, float y, const char* text) {
    glUseProgram(textShaderProgram);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projectionMatrix[0][0]);
    glUniform3f(textColorLoc, 1.0f, 1.0f, 1.0f); // Set text color

    glBindVertexArray(textVAO);

    for (const char* c = text; *c; c++) {
        if (FT_Load_Char(face, *c, FT_LOAD_RENDER)) {
            fprintf(stderr, "Failed to load glyph\n");
            continue;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

        float xpos = x + face->glyph->bitmap_left;
        float ypos = y - (face->glyph->bitmap.rows - face->glyph->bitmap_top);

        float w = static_cast<float>(face->glyph->bitmap.width);
        float h = static_cast<float>(face->glyph->bitmap.rows);


        GLfloat vertices[6][4] = {
            { xpos, ypos + h, 0.0, 0.0 },
            { xpos, ypos, 0.0, 1.0 },
            { xpos + w, ypos, 1.0, 1.0 },
            { xpos, ypos + h, 0.0, 0.0 },
            { xpos + w, ypos, 1.0, 1.0 },
            { xpos + w, ypos + h, 1.0, 0.0 }
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        x += (face->glyph->advance.x >> 6);
    }

    glBindVertexArray(0);
    glUseProgram(0);
}

#endif Shaders