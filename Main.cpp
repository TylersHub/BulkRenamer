//#include <GL/glut.h>
#include <iostream>
#include <fstream>
#include <sstream>
//#include <regex> //For strings
#include <string>
#include <vector>
#define GLAD_APIENTRY APIENTRY
#include <glad/glad.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#define GLFW_INCLUDE_WIN32
#include <GLFW/glfw3native.h>
#include <nanovg.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>
//#include <nanovg_stb_image.h>
#include <stb_image.h>
#include "tinyfiledialogs.h"
#include <windows.h>
#include <glm/glm.hpp>  // Include glm header for matrix operations
#include <glm/gtc/type_ptr.hpp>  // Include glm header for value_ptr function
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <boost/algorithm/string.hpp>
#include <cstdlib> // for std::atexit and system()
#include <cctype>
#include <algorithm>
#include <filesystem>
#include <regex>


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



std::string openFileDialog() {
    const char* filterPatterns[2] = { "*" }; // Accept all file types

    char const* file = tinyfd_openFileDialog(
        "Open File",
        "",
        1,
        filterPatterns,
        NULL,
        0
    );
    if (file != NULL) {
        return std::string(file);
    }
    else {
        return std::string(); // Return empty string if users cancels the dialog
    }
}


std::string openFolderDialog() {
    char const* folder = tinyfd_selectFolderDialog(
        "Select Folder",
        NULL // Default folder path (NULL for no default folder)
    );
    if (folder != NULL) {
        return std::string(folder);
    }
    else {
        return std::string(); // Return empty string if user cancels the dialog
    }
}


void errorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

std::string userText;


void replaceInFile(std::string filePath, std::string oldWord, std::string newWord) {

    // Trim trailing whitespace from newWord
    //newWord = trimTrailingWhitespace(newWord);

    std::ifstream file(filePath);

    if (!file.is_open()) {
        std::cout << "Error Opening File\n";
        return;
    }

    // Read the file into memory
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        size_t pos = 0;
        while ((pos = line.find(oldWord, pos)) != std::string::npos) {
            line.replace(pos, oldWord.length(), newWord);
            pos += newWord.length();
        }
        lines.push_back(line);
    }

    file.close();

    // Write the modified lines back to the file
    std::ofstream fileOut(filePath);
    for (const auto& line : lines) {
        fileOut << line << '\n';
    }
    fileOut.close();
}

std::string arrayToString(const char* arr) {
    // Find the length of the array
    size_t length = 0;
    while (arr[length] != '\0') {
        length++;
    }

    // Find the index of the last non-whitespace character
    size_t lastNonWhitespaceIndex = length;
    while (lastNonWhitespaceIndex > 0 && std::isspace(arr[lastNonWhitespaceIndex - 1])) {
        lastNonWhitespaceIndex--;
    }

    // Create a string from the non-whitespace characters
    return std::string(arr, arr + lastNonWhitespaceIndex);
}

// The file renaming functions

namespace fs = std::filesystem;

std::vector <std::string> displayFilesRenamed;

// Renames file if old_name is entire file name (excluding extension)
void FileRenamerFull(const std::string& FilePath, const std::string& old_name, const std::string& new_name, bool subDir) {
    fs::path target_directory = FilePath;
    std::ostringstream renamedFileString;

    if (!subDir)
    {
        for (const auto& entry : fs::directory_iterator(target_directory)) {
            if (!entry.is_regular_file()) continue; // Skip if not a regular file

            // Extract the filename without the extension
            std::string filename_without_extension = entry.path().stem().string();
            std::string extension = entry.path().extension().string();

            // Check if the filename matches old_name
            if (filename_without_extension == old_name) {
                // Construct new filename with the same extension
                std::string new_filename = new_name + extension;

                // Full path for the new file
                auto new_path = entry.path().parent_path() / new_filename;

                // Rename the file
                fs::rename(entry.path(), new_path);
                std::cout << "Renamed " << entry.path().filename() << " to " << new_filename << std::endl; // Prints file name in console

                renamedFileString << "Renamed " << entry.path().filename() << " to " << new_filename; // Adds file name string to output string stream
                displayFilesRenamed.push_back(renamedFileString.str()); // Adds file name string to vector for display

            }
        }
    }
    else if (subDir) {
        for (const auto& entry : fs::recursive_directory_iterator(target_directory)) {
            if (!entry.is_regular_file()) continue; // Skip if not a regular file

            // Extract the filename without the extension
            std::string filename_without_extension = entry.path().stem().string();
            std::string extension = entry.path().extension().string();

            // Check if the filename matches old_name
            if (filename_without_extension == old_name) {
                // Construct new filename with the same extension
                std::string new_filename = new_name + extension;

                // Full path for the new file
                auto new_path = entry.path().parent_path() / new_filename;

                // Rename the file
                fs::rename(entry.path(), new_path);
                std::cout << "Renamed " << entry.path() << " to " << new_path << std::endl;

                renamedFileString << "Renamed " << entry.path().filename() << " to " << new_filename; // Adds file name string to output string stream
                displayFilesRenamed.push_back(renamedFileString.str()); // Adds file name string to vector for display
            }
        }
    }
}

// Renames file if old_name is found in part of file name
void FileRenamerPartial(const std::string& FilePath, const std::string& old_name, const std::string& new_name, bool subDir) {
    fs::path target_directory = FilePath;
    std::ostringstream renamedFileString;

    if (!subDir) {
        for (const auto& entry : fs::directory_iterator(target_directory)) {
            if (!entry.is_regular_file()) continue; // Skip if not a regular file

            // Extract the complete filename
            std::string filename = entry.path().filename().string();

            // Find position of old_name within the filename
            size_t pos = filename.find(old_name);
            if (pos != std::string::npos) {
                // Replace old_name with new_name in the filename
                std::string new_filename = filename.substr(0, pos) + new_name + filename.substr(pos + old_name.length());

                // Full path for the new file
                auto new_path = entry.path().parent_path() / new_filename;

                // Rename the file
                fs::rename(entry.path(), new_path);
                std::cout << "Renamed " << entry.path().filename() << " to " << new_filename << std::endl;

                renamedFileString << "Renamed " << entry.path().filename() << " to " << new_filename << "\n"; // Adds file name string to output string stream
                displayFilesRenamed.push_back(renamedFileString.str()); // Adds file name string to vector for display
            }
        }
    }
    else if (subDir) {
        for (const auto& entry : fs::recursive_directory_iterator(target_directory)) {
            if (!entry.is_regular_file()) continue; // Skip if not a regular file

            // Extract the complete filename
            std::string filename = entry.path().filename().string();

            // Find position of old_name within the filename
            size_t pos = filename.find(old_name);
            if (pos != std::string::npos) {
                // Replace old_name with new_name in the filename
                std::string new_filename = filename.substr(0, pos) + new_name + filename.substr(pos + old_name.length());

                // Full path for the new file
                auto new_path = entry.path().parent_path() / new_filename;

                // Rename the file
                fs::rename(entry.path(), new_path);
                std::cout << "Renamed " << entry.path() << " to " << new_path << std::endl;

                renamedFileString << "Renamed " << entry.path().filename() << " to " << new_filename << "\n"; // Adds file name string to output string stream
                displayFilesRenamed.push_back(renamedFileString.str()); // Adds file name string to vector for display
            }
        }
    }
}

std::string user_convert_old, user_convert_new, user_path, user_extension; // Strings for converting character list to string

void onExit() {
    std::cout << "Application Exited." << std::endl;
}

bool toggleStateNVG = false; // Toggle variable for toggle switch
double xpos, ypos; // Mouse position for toggle switch

// Draws Toggle Switch
void DrawNanoVGToggleSwitch(NVGcontext* vg, float x, float y, float width, float height) {

    if (xpos >= x && xpos <= x + width && ypos >= y && ypos <= y + height) {
        // Toggle Switch is hovered
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
    }
    else {
        // Default Toggle Switch color
        nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
    }
    // Draw background of Toggle Switch
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x, y, width, height, height * 0.5f);
    nvgFill(vg);
    nvgStrokeWidth(vg, 1.0f); //Sets line width of stroke
    nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); //Sets the color of stroke
    nvgStroke(vg); // Makes stroke

    // Draw handle of Toggle Switch
    float handleX = x + (toggleStateNVG ? width - height : 0);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, handleX, y, height, height, height * 0.5f);
    nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
    nvgFill(vg);
}


int main() {

    if (FT_Init_FreeType(&ft)) {
        fprintf(stderr, "Failed to initialize FreeType\n");
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, "trebuc.ttf", 0, &face)) {
        fprintf(stderr, "Failed to load font\n");
        return -1;
    }

    FT_Set_Pixel_Sizes(face, 0, 48); // Set font size

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // Set GLFW error callback
    glfwSetErrorCallback(errorCallback);

    // Set OpenGL version to 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Enable high-DPI awareness, adjust DPI to monitor resulution
    //glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);


    // Creat GLFW Window
    GLFWwindow* window = glfwCreateWindow(600, 600, "BulkRenamer", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }


    // Initialize FreeType and shaders
    //initFreeType();
    //initShaders();

    // Set character callback to handle text input
    //glfwSetCharCallback(window, charCallback);

    // 0x antialiasing
    glfwWindowHint(GLFW_SAMPLES, 0);

    // Load the dialog icon (in window)
   HICON hIcon = (HICON)LoadImage(NULL, TEXT("New_BulkRenamer_Logo.ico"), IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED);

    if (hIcon)
    {

        // Get the native window handle
        HWND hwnd = glfwGetWin32Window(window);

        // Set the icon
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    //Disables Full screen option
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);

    // Makes the Window and Window context current
    glfwMakeContextCurrent(window);

    //Loads OpenGL function pointers and extensions with GLAD and GLFW by getting the address of the functions 
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    initFreeType();
    initShaders();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;


    // Setup Platform/Renderer bindings
    const char* glsl_version = "#version 130";
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    //glfwSetMouseButtonCallback(window, MouseButtonCallbackToggleSwitch);

    // NanoVG initialization
    NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);

    // Load the font
    int font = nvgCreateFont(vg, "Trebuche", "trebuc.ttf");
    if (font == -1) {
        fprintf(stderr, "Failed to load font\n");
        return -1;
    }

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); //Color of the window background

    glfwSetWindowSizeLimits(window, 600, 600, 600, 600); //Sets the minimum and maximum size of the window to 600x600

    // Load Image
    int imageHandle = nvgCreateImage(vg, "New_BulkRenamer_Logo_Background.png", 0);

    // Set texture filtering parameters
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Set texture filtering parameters
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    int imageWidth, imageHeight;
    nvgImageSize(vg, imageHandle, &imageWidth, &imageHeight);
    std::cout << " " << imageWidth << " " << imageHeight << "\n";
    //unsigned char* image = stbi_load("BulkRenamer_Logo.png", &imageWidth, &imageHeight, &imageChannels, 0);

    if (imageHandle == 0) {
        fprintf(stderr, "Failed to load image\n");
        return -1;
    }

    float newWidth = 1.5f * static_cast<float>(imageWidth);
    float newHeight = 1.5f * static_cast<float>(imageHeight);

    float imageX = (600 - newWidth) * 0.5f;
    float imageY = (600 - newHeight) * 0.5f;

    bool fileSelected = false; // flag to check if a file is selected
    bool fileSelectedText = false; // flag to check if a file is selected for text
    std::string selectedFile; // stores the path of the selected file

    bool isMouseOverButton = false;
    bool isButtonClicked = false;

    bool isMouseOverButton2 = false;
    bool isButtonClicked2 = false;

    bool isToggleSwitchClicked = false;

    //std::string str1, str2; // Strings for converting character list to string

    std::cout << " " << newWidth << " " << newHeight << "\n";

    //io.FontGlobalScale = 1.0f; // Change this value to adjust global font size
    ImFont* ImGui_font_normal = io.Fonts->AddFontFromFileTTF("trebuc.ttf", 24.0f); // Load the font file with a font size of x points
    ImFont* ImGui_font_small = io.Fonts->AddFontFromFileTTF("trebuc.ttf", 20.0f); // Load the font file with a font size thats smaller
    ImFont* ImGui_font_extra_small = io.Fonts->AddFontFromFileTTF("trebuc.ttf", 16.0f);

    // Set the font as the default font for ImGui
    io.FontDefault = ImGui_font_normal;


    // Main Loop
    while (!glfwWindowShouldClose(window)) {


        // Get framebuffer size
        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        // Set viewport
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        // Renders contents to the window
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Start the Dear ImGui frame rendering
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        //ImGui::SetNextWindowBgAlpha(0.0f); // Transparent background

        ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        //ImGui::PushStyleColor(ImGuiCol_WindowBg, clear_color); // Transparent background

        ImGui::PushStyleColor(ImGuiCol_FrameBg, clear_color); // Transparent background for Text Box
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // Text Color

        // Checkbox 1
        {
            ImGui::SetNextWindowPos(ImVec2(15, 110));
            //ImGui::SetNextWindowSize(ImVec2(488, 40));
            ImGui::Begin("Check", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBackground
            );

            // Draw a border around the checkbox
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 checkboxPos = ImGui::GetCursorScreenPos();
            ImVec2 checkboxSize = ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize());
            ImVec2 minBound = ImVec2(checkboxPos.x - 1, checkboxPos.y - 1);
            ImVec2 maxBound = ImVec2(checkboxPos.x + checkboxSize.x + 6, checkboxPos.y + checkboxSize.y + 6);
            ImU32 fillColor = IM_COL32(255, 255, 255, 255); // Fill Color
            drawList->AddRectFilled(minBound, maxBound, fillColor);
            drawList->AddRect(minBound, maxBound, IM_COL32(0, 0, 0, 255)); // Black border
        }

            // Create a checkbox
            static bool checkboxValue1 = false;
            bool& CheckBoxValRef1 = checkboxValue1; // Reference to the checkbox value
            ImGui::Checkbox("File Extension", &CheckBoxValRef1); // ##checkbox is an empty identifier to avoid label text

            ImGui::End(); //End of Checkbox 1
     
        
        /*
        // Checkbox 2
        {
            ImGui::SetNextWindowPos(ImVec2(100, 360));
            ImGui::Begin("Check2", nullptr,
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoBackground
            );

            // Draw a border around the checkbox
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 checkboxPos = ImGui::GetCursorScreenPos();
            ImVec2 checkboxSize = ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize());
            ImVec2 minBound = ImVec2(checkboxPos.x - 1, checkboxPos.y - 1);
            ImVec2 maxBound = ImVec2(checkboxPos.x + checkboxSize.x + 6, checkboxPos.y + checkboxSize.y + 6);
            ImU32 fillColor = IM_COL32(255, 255, 255, 255); // Fill Color
            drawList->AddRectFilled(minBound, maxBound, fillColor);
            drawList->AddRect(minBound, maxBound, IM_COL32(0, 0, 0, 255)); // Black border
        }

            // Create a checkbox 2
            static bool checkboxValue2 = false;
            bool& CheckBoxValRef2 = checkboxValue2; // Reference to the checkbox value
            ImGui::Checkbox("Subdirectories", &CheckBoxValRef2); // ##checkbox is an empty identifier to avoid label text

            ImGui::End(); //End of Checkbox 2
            */


        //Text Input Box 1
        //{
        ImGui::SetNextWindowPos(ImVec2(195, -6));
        ImGui::SetNextWindowSize(ImVec2(310, 40));
        ImGui::Begin(" ", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground
        );

        // Create a text input box
        static char text1[32] = "";
        ImGui::InputText(" ", text1, IM_ARRAYSIZE(text1));

        ImGui::End(); //End of Text box 1

        //}

        //Text Input Box 2
        //{
        ImGui::SetNextWindowPos(ImVec2(165, 29));
        ImGui::SetNextWindowSize(ImVec2(310, 40));
        ImGui::Begin("  ", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground
        );

        static char text2[32] = "";
        ImGui::InputText("  ", text2, IM_ARRAYSIZE(text2));


        // Render the typed text without a background
        //ImGui::TextUnformatted(text);
        //Renders Text with Black color
        //ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "%s", text); 

        ImGui::End(); // End the ImGui window / End of Text box 2
        //}

        // NanoVG rendering
        nvgBeginFrame(vg, 600, 600, 1.0f);

        NVGpaint imgPaint = nvgImagePattern(vg, imageX, imageY, static_cast<float>(newWidth), static_cast<float>(newHeight), 0, imageHandle, 0.15f);

        // Draw Background Image
        nvgBeginPath(vg);
        nvgRect(vg, imageX, imageY, static_cast<float>(newWidth), static_cast<float>(newHeight)); // Position and size of the image rectangle
        nvgFillPaint(vg, imgPaint); // Fill the path with the image pattern
        //nvgImagePattern(vg, 300.0f, 300.0f, static_cast<float>(imageWidth), static_cast<float>(imageHeight), 0, imageTexture, 1.0f); // Use the texture as a pattern
        nvgFill(vg);

        // Draw text (old name)
        nvgBeginPath(vg); //Starts a new path
        nvgFontSize(vg, 20.0f);
        nvgFontFaceId(vg, font); // Use the font ID instead of the name
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
        nvgText(vg, 15.0f, 25.0f, "Enter current name:", NULL);

        // Draw Box 1
        nvgBeginPath(vg); //Starts a new path
        nvgRect(vg, 205.0f, 4.0f, 200, 27); // Rectangle at position (100, 100) with width 200 and height 150
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 255)); // Color of the rectangle
        nvgStrokeWidth(vg, 2.0f); // Set the width of the stroke to 2.0f
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); // Stroke color (black)
        nvgFill(vg); // Fill the path
        nvgStroke(vg); // Draw the stroke


        // Draw text (new name)
        nvgBeginPath(vg); //Starts a new path
        nvgFontSize(vg, 20.0f);
        nvgFontFaceId(vg, font); // Use the font ID instead of the name
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
        nvgText(vg, 15.0f, 60.0f, "Enter new name:", NULL);

        // Draw Box 2
        nvgBeginPath(vg); //Starts a new path
        nvgRect(vg, 175.0f, 38.0f, 200, 28); // Rectangle at position (100, 100) with width 200 and height 150
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 255)); // Color of the rectangle
        nvgStrokeWidth(vg, 2.0f); // Set the width of the stroke to 2.0f
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); // Stroke color (black)
        nvgFill(vg); // Fill the path
        nvgStroke(vg); // Draw the stroke



        // Draw Selection Button Variables
        float button1X = 400.0f;
        float button1Y = 280.0f;
        float buttonWidth1 = 150.0f;
        float buttonHeight1 = 50.0f;

        // Check if the mouse is over the button
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        isMouseOverButton = (mouseX >= button1X && mouseX <= button1X + buttonWidth1 && mouseY >= button1Y && mouseY <= button1Y + buttonHeight1);

        // Check if the left mouse button is pressed
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (isMouseOverButton) {
                isButtonClicked = true;
            }
        }
        else {
            if (isMouseOverButton && isButtonClicked) {
                // Open the file dialog when the button is clicked (non-blocking)
                /*if (toggleStateNVG) {
                    std::string file = openFileDialog();

                    if (!file.empty()) {
                        selectedFile = file;
                        fileSelected = true;
                        fileSelectedText = true;
                        printf("Selected file: %s\n", selectedFile.c_str());
                    }
                }
                else {
                    std::string file = openFolderDialog();

                    if (!file.empty()) {
                        selectedFile = file;
                        fileSelected = true;
                        fileSelectedText = true;
                        printf("Selected file: %s\n", selectedFile.c_str());
                    }
                }*/

                std::string file = openFolderDialog();

                if (!file.empty()) {
                    selectedFile = file;
                    fileSelected = true;
                    fileSelectedText = true;
                    printf("Selected file: %s\n", selectedFile.c_str());
                }
            }
            isButtonClicked = false;
        }

        std::string displayedFile = selectedFile.c_str();  // Display only the first x amount of characters

        // Update button color based on state
        if (isButtonClicked) {
            // Button is inactive when a file is already selected
            nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
        }
        else if (isMouseOverButton) {
            // Button is hovered
            nvgFillColor(vg, nvgRGBA(200, 200, 200, 255));
        }
        else {
            // Default button color
            nvgFillColor(vg, nvgRGBA(150, 150, 150, 255));
        }


        // Draw Button 1 (Select a Folder)
        nvgBeginPath(vg); //Starts a new path
        nvgRect(vg, button1X, button1Y, buttonWidth1, buttonHeight1); //Draws a rectangle
        nvgFill(vg); //Fills the path with the current fill style
        nvgStrokeWidth(vg, 1.0f); //Sets line width of stroke
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); //Sets the color of stroke
        nvgStroke(vg); // Makes stroke

        // Draw Text on Button 1
        nvgFontSize(vg, 18.0f);
        nvgFontFaceId(vg, font); // Use the font ID instead of the name
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE); //Aligns the text to center
        nvgText(vg, button1X + buttonWidth1 / 2.0f, button1Y + buttonHeight1 / 2.0f, "Select a Folder", NULL);

        // Draw Box 3
        nvgBeginPath(vg); //Starts a new path
        nvgRect(vg, 72.0f, 288.0f, 320, 33); // Rectangle at position (100, 100) with width 200 and height 150
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 255)); //Color of the rectangle
        nvgStrokeWidth(vg, 2.0f); // Set the width of the stroke to 2.0f
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); // Stroke color (black)
        nvgFill(vg); // Fill the path
        nvgStroke(vg); // Draw the stroke

        //Text Input Box 3

        ImGui::SetNextWindowPos(ImVec2(66, 283));
        ImGui::SetNextWindowSize(ImVec2(488, 40));
        ImGui::Begin("   ", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground
        );

        ImGui::PushFont(ImGui_font_small);

        static char text3[96] = "";
        ImGui::InputText("   ", text3, IM_ARRAYSIZE(text3));

        // Writes File Path when file or folder is selected
        if (fileSelectedText) {
            std::string displayedFile = selectedFile.c_str();  // Display only the first x amount of characters

            for (int i = 0; i < displayedFile.size(); i++) {
                text3[i] = displayedFile[i];
            }

            //ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "%s", displayedFile.c_str());
        }

        ImGui::PopFont();

        ImGui::End(); //End of Text box 3

        
        // Checkbox 2 Variables
        static bool checkboxValue2 = false;
        bool& CheckBoxValRef2 = checkboxValue2; // Reference to the checkbox value

        // Textbox 4 Variable for Checkbox 2
        static char text4[32] = "";

        static bool checkboxValue3 = false;
        bool& CheckBoxValRef3 = checkboxValue3; // Reference to the checkbox value

        //If statement to move Checkbox 2 (Subdirectories) if File extension directory is enabled
        if (CheckBoxValRef1) {
            // Checkbox 2
            {
                ImGui::SetNextWindowPos(ImVec2(15, 170));
                ImGui::Begin("Check2", nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoBackground
                );

                // Draw a border around the checkbox
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 checkboxPos = ImGui::GetCursorScreenPos();
                ImVec2 checkboxSize = ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize());
                ImVec2 minBound = ImVec2(checkboxPos.x - 1, checkboxPos.y - 1);
                ImVec2 maxBound = ImVec2(checkboxPos.x + checkboxSize.x + 6, checkboxPos.y + checkboxSize.y + 6);
                ImU32 fillColor = IM_COL32(255, 255, 255, 255); // Fill Color
                drawList->AddRectFilled(minBound, maxBound, fillColor);
                drawList->AddRect(minBound, maxBound, IM_COL32(0, 0, 0, 255)); // Black border
            }

            // Create a checkbox 2
            //static bool checkboxValue2 = false;
            //bool& CheckBoxValRef2 = checkboxValue2; // Reference to the checkbox value
            ImGui::Checkbox("Subdirectories", &CheckBoxValRef2); // ##checkbox is an empty identifier to avoid label text

            ImGui::End(); //End of Checkbox 2

            // Checkbox 3 (Rename Folders)

            {
                ImGui::SetNextWindowPos(ImVec2(15, 210));
                ImGui::Begin("Check3", nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoBackground
                );

                // Draw a border around the checkbox
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 checkboxPos = ImGui::GetCursorScreenPos();
                ImVec2 checkboxSize = ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize());
                ImVec2 minBound = ImVec2(checkboxPos.x - 1, checkboxPos.y - 1);
                ImVec2 maxBound = ImVec2(checkboxPos.x + checkboxSize.x + 6, checkboxPos.y + checkboxSize.y + 6);
                ImU32 fillColor = IM_COL32(255, 255, 255, 255); // Fill Color
                drawList->AddRectFilled(minBound, maxBound, fillColor);
                drawList->AddRect(minBound, maxBound, IM_COL32(0, 0, 0, 255)); // Black border
            }


            // Create a checkbox 3
            //static bool checkboxValue3 = false;
            //bool& CheckBoxValRef3 = checkboxValue3; // Reference to the checkbox value
            ImGui::Checkbox("Rename Folders", &CheckBoxValRef3); // ##checkbox is an empty identifier to avoid label text

            ImGui::End(); //End of Checkbox 3

            //Text Input Box 4
            {
                ImGui::SetNextWindowPos(ImVec2(110, 141));
                ImGui::SetNextWindowSize(ImVec2(310, 40));
                ImGui::Begin("    ", nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoBackground
                );

                //static char text4[32] = "";
                ImGui::InputText("    ", text4, IM_ARRAYSIZE(text4));

                ImGui::End(); // End the ImGui window / End of Text box 4
            }

            // Draw text (File extension)
            nvgBeginPath(vg); //Starts a new path
            nvgFontSize(vg, 20.0f);
            nvgFontFaceId(vg, font); // Use the font ID instead of the name
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
            nvgText(vg, 70.0f, 162.0f, "File Type:", NULL);

            // Draw Box 2
            nvgBeginPath(vg); //Starts a new path
            nvgRect(vg, 120.0f, 150.0f, 100, 28); // Rectangle at position (100, 100) with width 200 and height 150
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 255)); // Color of the rectangle
            nvgStrokeWidth(vg, 2.0f); // Set the width of the stroke to 2.0f
            nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); // Stroke color (black)
            nvgFill(vg); // Fill the path
            nvgStroke(vg); // Draw the stroke
        }
        else {
            // Checkbox 2
            {
                ImGui::SetNextWindowPos(ImVec2(15, 150));
                ImGui::Begin("Check2", nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoBackground
                );

                // Draw a border around the checkbox
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 checkboxPos = ImGui::GetCursorScreenPos();
                ImVec2 checkboxSize = ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize());
                ImVec2 minBound = ImVec2(checkboxPos.x - 1, checkboxPos.y - 1);
                ImVec2 maxBound = ImVec2(checkboxPos.x + checkboxSize.x + 6, checkboxPos.y + checkboxSize.y + 6);
                ImU32 fillColor = IM_COL32(255, 255, 255, 255); // Fill Color
                drawList->AddRectFilled(minBound, maxBound, fillColor);
                drawList->AddRect(minBound, maxBound, IM_COL32(0, 0, 0, 255)); // Black border
            }

            // Create a checkbox 2
            //static bool checkboxValue2 = false;
            //bool& CheckBoxValRef2 = checkboxValue2; // Reference to the checkbox value
            ImGui::Checkbox("Subdirectories", &CheckBoxValRef2); // ##checkbox is an empty identifier to avoid label text

            ImGui::End(); //End of Checkbox 2

            // Checkbox 3 (Rename Folders)

            {
                ImGui::SetNextWindowPos(ImVec2(15, 190));
                ImGui::Begin("Check3", nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoBackground
                );

                // Draw a border around the checkbox
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 checkboxPos = ImGui::GetCursorScreenPos();
                ImVec2 checkboxSize = ImVec2(ImGui::GetFontSize(), ImGui::GetFontSize());
                ImVec2 minBound = ImVec2(checkboxPos.x - 1, checkboxPos.y - 1);
                ImVec2 maxBound = ImVec2(checkboxPos.x + checkboxSize.x + 6, checkboxPos.y + checkboxSize.y + 6);
                ImU32 fillColor = IM_COL32(255, 255, 255, 255); // Fill Color
                drawList->AddRectFilled(minBound, maxBound, fillColor);
                drawList->AddRect(minBound, maxBound, IM_COL32(0, 0, 0, 255)); // Black border
            }


            // Create a checkbox 3
            //static bool checkboxValue3 = false;
            //bool& CheckBoxValRef3 = checkboxValue3; // Reference to the checkbox value
            ImGui::Checkbox("Rename Folders", &CheckBoxValRef3); // ##checkbox is an empty identifier to avoid label text

            ImGui::End(); //End of Checkbox 3
        }


        // Draw Rename Button Variables
        float button2X = 225.0f;
        float button2Y = 355.0f;
        float buttonWidth2 = 125.0f;
        float buttonHeight2 = 50.0f;

        // Check if the mouse is over the button
        glfwGetCursorPos(window, &mouseX, &mouseY);

        isMouseOverButton2 = (mouseX >= button2X && mouseX <= button2X + buttonWidth2 && mouseY >= button2Y && mouseY <= button2Y + buttonHeight2);


        // Update button color based on state
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            // Button color when button is pressed
            nvgFillColor(vg, nvgRGBA(0, 150, 0, 255));
        }
        else if (isMouseOverButton2) {
            // Button is hovered
            nvgFillColor(vg, nvgRGBA(0, 200, 0, 255));
        }
        else {
            // Default button color
            nvgFillColor(vg, nvgRGBA(0, 150, 0, 255));
        }


        // Draw Button 2 (Rename)
        nvgBeginPath(vg); //Starts a new path
        nvgRect(vg, button2X, button2Y, buttonWidth2, buttonHeight2); //Draws a rectangle
        nvgFill(vg); //Fills the path with the current fill style
        nvgStrokeWidth(vg, 1.0f); //Sets line width of stroke
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); //Sets the color of stroke
        nvgStroke(vg); // Makes stroke

        // Draw Text on Button
        nvgFontSize(vg, 24.0f);
        nvgFontFaceId(vg, font); // Use the font ID instead of the name
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE); //Aligns the text to center
        nvgText(vg, button2X + buttonWidth2 / 2.0f, button2Y + buttonHeight2 / 2.0f, "Rename", NULL);


        // Draw toggle switch using NanoVG
        DrawNanoVGToggleSwitch(vg, 21, 80, 60, 30);

        glfwGetCursorPos(window, &xpos, &ypos);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (xpos >= 21 && xpos <= 21 + 60 && ypos >= 80 && ypos <= 80 + 30) {
                isToggleSwitchClicked = true;
            }
        }
        else {
            // Check if the click is within the toggle switch area
            if ((xpos >= 21 && xpos <= 21 + 60 && ypos >= 80 && ypos <= 80 + 30) && isToggleSwitchClicked) {
                toggleStateNVG = !toggleStateNVG; // Toggle the state
            }
            isToggleSwitchClicked = false;
        }

        // Changes Word when clicking toggle switch
        if (toggleStateNVG) {
            // Draw text (toggle switch)
            nvgBeginPath(vg); //Starts a new path
            nvgFontSize(vg, 20.0f);
            nvgFontFaceId(vg, font); // Use the font ID instead of the name
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
            nvgText(vg, 150.0f, 95.0f, "Entire Name", NULL);
        }
        else {
            // Draw text (toggle switch)
            nvgBeginPath(vg); //Starts a new path
            nvgFontSize(vg, 20.0f);
            nvgFontFaceId(vg, font); // Use the font ID instead of the name
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
            nvgText(vg, 150.0f, 95.0f, "Part of Name", NULL);
        }


        // Draw text (Files Renamed)
        nvgBeginPath(vg); //Starts a new path
        nvgFontSize(vg, 20.0f);
        nvgFontFaceId(vg, font); // Use the font ID instead of the name
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
        nvgText(vg, 90.0f, 470.0f, "Files Renamed:", NULL);

        // Draw Output Box 1
        nvgBeginPath(vg); //Starts a new path
        nvgRect(vg, 175.0f, 460.0f, 300, 130); // Rectangle at position (100, 100) with width 200 and height 150
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 255)); // Color of the rectangle
        nvgStrokeWidth(vg, 2.0f); // Set the width of the stroke to 2.0f
        nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); // Stroke color (black)
        nvgFill(vg); // Fill the path
        nvgStroke(vg); // Draw the stroke

        //Text Output Box 1 (Displays Renamed Files)

        ImGui::SetNextWindowPos(ImVec2(170, 458));
        ImGui::SetNextWindowSize(ImVec2(310, 134));
        ImGui::Begin("###TextWindow", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            //ImGuiWindowFlags_NoScrollbar |
            //ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground
        );

        ImGui::PushFont(ImGui_font_extra_small); // Add font size to text

        // Create a text output box
        for (const auto& renamedFile : displayFilesRenamed){
            ImGui::TextWrapped("%s", renamedFile.c_str());
            }

        ImGui::PopFont(); // Remove font size after text rendered

        ImGui::End(); //End of Text Output box 1
         

        //Ends the frame for NanoVG and draws the content to the screen
        nvgEndFrame(vg);

        user_convert_old = arrayToString(text1);
        user_convert_new = arrayToString(text2);
        user_path = arrayToString(text3);
        user_extension = arrayToString(text4);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (isMouseOverButton2) {
                isButtonClicked2 = true;
            }
        }
        else {
            if (isMouseOverButton2 && isButtonClicked2) {
                if (toggleStateNVG) {
                    FileRenamerFull(user_path, user_convert_old, user_convert_new, CheckBoxValRef2);
                }
                else {
                    FileRenamerPartial(user_path, user_convert_old, user_convert_new, CheckBoxValRef2);
                }
            }
            isButtonClicked2 = false;
        }


        
        ImGui::PopStyleColor(); // Restore default text color
        ImGui::PopStyleColor(); // Restore default window background color

        //Render ImGui
        ImGui::Render();

        // Render ImGui draw data
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        //Swaps the front and back buffers of the specified window
        glfwSwapBuffers(window);
        //Handles the events like keyboard, mouse, etc.
        glfwPollEvents();

        // Draw user text input
        /*nvgBeginPath(vg);
        nvgFontSize(vg, 20.0f);
        nvgFontFaceId(vg, font);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
        nvgText(vg, 20.0f, 200.0f, userText.c_str(), NULL);*/

    }

    // Cleanup

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup FreeType and GLFW
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Cleanup OpenGL resources
    glDeleteProgram(textShaderProgram);
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);

    //DestroyIcon(hIcon);
    //nvgDeleteGL3(vg);
    glfwDestroyWindow(window);
    glfwTerminate();

    std::atexit(onExit);
    
    return 0;
}
