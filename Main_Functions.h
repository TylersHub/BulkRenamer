#pragma once
#ifndef Main_Functions
#define Main_Functions

//===================
// Include Statements
//===================

#include <iostream>
#include <fstream>
#include <sstream>
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
#include <stb_image.h>
#include <tinyfiledialogs.h>
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
#include "resource.h"
#include "Shaders.h"

//==========
// Variables
//==========

namespace fs = std::filesystem;
std::vector <std::string> displayFilesRenamed; // Vector to store names of each renamed file to be displayed
std::string ConvertOld, ConvertNew, ConvertPath, ConvertExtension; // Strings for converting character list to string
bool toggleStateNVG = false; // Toggle variable for toggle switch
double xpos, ypos; // Mouse position for toggle switch



//===============
// Main Functions
//===============

void errorCallback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
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

// Renames file if old_name is entire file name (excluding extension)
void FileRenamerFull(const std::string& FilePath, const std::string& old_name, const std::string& new_name, bool subDir, bool includeFolders) {
    fs::path target_directory = FilePath;
    std::ostringstream renamedFileString;

    if (!subDir)
    {
        for (const auto& entry : fs::directory_iterator(target_directory)) {
            //if (!entry.is_regular_file()) continue; // Skip if not a regular file

            renamedFileString.str("");

            if (includeFolders ? (entry.is_regular_file() || entry.is_directory()) : entry.is_regular_file()) {
                // Extract the filename without the extension
                std::string filename_without_extension = entry.path().stem().string();
                std::string extension = entry.path().extension().string();

                // Check if the filename matches old_name
                if (filename_without_extension == old_name) {
                    // Construct new filename with the same extension
                    std::string new_filename = new_name + extension;

                    // Full path for the new file
                    auto new_path = entry.path().parent_path() / new_filename;

                    try {
                        // Rename the file
                        fs::rename(entry.path(), new_path);
                        std::cout << "Renamed " << entry.path().filename() << " to " << new_path.filename() << std::endl; // Prints file name in console

                        renamedFileString << "Renamed " << entry.path().filename() << " to " << new_path.filename() << std::endl; // Adds file name string to output string stream
                        displayFilesRenamed.push_back(renamedFileString.str()); // Adds file name string to vector for display
                    }
                    catch (const fs::filesystem_error& e) {
                        std::cerr << "Failed to rename " << entry.path() << " to " << new_path.filename() << ": " << e.what() << std::endl;
                    }
                }

            }
        }
    }
    else if (subDir) {
        std::vector<fs::path> paths;

        // Step 1: Collect paths
        for (const auto& entry : fs::recursive_directory_iterator(target_directory)) {
            if (includeFolders ? (entry.is_regular_file() || entry.is_directory()) : entry.is_regular_file()) {
                // Check if the filename matches old_name, excluding extension for files
                std::string filename = entry.path().filename().replace_extension().string();
                if (filename == old_name) {
                    paths.push_back(entry.path());
                }
            }
        }

        // Step 2: Sort paths by depth, descending
        std::sort(paths.begin(), paths.end(), [](const fs::path& a, const fs::path& b) {
            return std::distance(a.begin(), a.end()) > std::distance(b.begin(), b.end());
            });

        // Step 3: Rename
        for (const auto& path : paths) {
            // Determine new path
            renamedFileString.str("");
            auto new_filename = path.filename().replace_extension().string() == old_name ?
                path.filename().replace_extension().string().replace(0, old_name.length(), new_name) + path.extension().string() :
                path.filename().string();
            auto new_path = path.parent_path() / new_filename;

            try {
                fs::rename(path, new_path);
                std::cout << "Renamed " << path << " to " << new_path << std::endl;

                renamedFileString << "Renamed " << path.filename() << " to " << new_path.filename() << std::endl;
                displayFilesRenamed.push_back(renamedFileString.str());
            }
            catch (const fs::filesystem_error& e) {
                std::cerr << "Failed to rename " << path << " to " << new_path.filename() << ": " << e.what() << std::endl;
            }
        }
    }
}

// Renames file if old_name is found in part of file name (excluding extension)
void FileRenamerPartial(const std::string& FilePath, const std::string& old_name, const std::string& new_name, bool subDir, bool includeFolders) {
    fs::path target_directory = FilePath;
    std::ostringstream renamedFileString;

    if (!subDir) {
        for (const auto& entry : fs::directory_iterator(target_directory)) {
            renamedFileString.str("");

            if (includeFolders ? (entry.is_regular_file() || entry.is_directory()) : entry.is_regular_file()) {
                // Extract the complete filename
                std::string filename = entry.path().filename().string();

                // Find position of old_name within the filename
                size_t pos = filename.find(old_name);
                if (pos != std::string::npos) {
                    // Replace old_name with new_name in the filename
                    std::string new_filename = filename.substr(0, pos) + new_name + filename.substr(pos + old_name.length());

                    // Full path for the new file
                    auto new_path = entry.path().parent_path() / new_filename;

                    try {
                        // Rename the file
                        fs::rename(entry.path(), new_path);
                        std::cout << "Renamed " << entry.path().filename() << " to " << new_path.filename() << std::endl;

                        renamedFileString << "Renamed " << entry.path().filename() << " to " << new_path.filename() << std::endl;
                        displayFilesRenamed.push_back(renamedFileString.str());
                    }
                    catch (const fs::filesystem_error& e) {
                        std::cerr << "Failed to rename " << entry.path() << " to " << new_path.filename() << ": " << e.what() << std::endl;
                    }
                }
            }
        }
    }
    else if (subDir) {
        std::vector<fs::path> paths;

        // Step 1: Collect paths
        for (const auto& entry : fs::recursive_directory_iterator(target_directory)) {
            if (includeFolders ? (entry.is_regular_file() || entry.is_directory()) : entry.is_regular_file()) {
                if (entry.path().filename().string().find(old_name) != std::string::npos) {
                    paths.push_back(entry.path());
                }
            }
        }

        // Step 2: Sort paths by depth, descending
        std::sort(paths.begin(), paths.end(), [](const fs::path& a, const fs::path& b) {
            // Count the depth by counting the number of elements in the path
            return std::distance(a.begin(), a.end()) > std::distance(b.begin(), b.end());
            });


        // Step 3: Rename
        for (const auto& path : paths) {
            renamedFileString.str("");
            auto new_path = path.parent_path() / path.filename().string().replace(path.filename().string().find(old_name), old_name.length(), new_name);
            try {
                fs::rename(path, new_path);
                std::cout << "Renamed " << path << " to " << new_path << std::endl;

                renamedFileString << "Renamed " << path.filename() << " to " << new_path.filename() << std::endl;
                displayFilesRenamed.push_back(renamedFileString.str());
            }
            catch (const fs::filesystem_error& e) {
                std::cerr << "Failed to rename " << path << " to " << new_path.filename() << ": " << e.what() << std::endl;
            }
        }
    }
}

// Renames file if old_name is entire file name (including extension)
void FileRenamerFull_Ext(const std::string& FilePath, const std::string& old_name, const std::string& new_name, std::string& ext, bool subDir, bool includeFolders) {
    fs::path target_directory = FilePath;
    std::vector<fs::path> paths;
    std::string normalized_ext = (ext.front() == '.') ? ext : "." + ext; // Ensure the extension starts with a dot.
    std::ostringstream renamedFileString;

    // Step 1: Collect Paths
    if (subDir) {
        for (const auto& entry : fs::recursive_directory_iterator(target_directory)) {
            bool isFile = entry.is_regular_file();
            bool isDir = entry.is_directory();
            if ((includeFolders && (isFile || isDir)) || (!includeFolders && isFile)) {
                std::string filename = entry.path().filename().string();
                std::string filename_without_extension = entry.path().stem().string();
                std::string extension = entry.path().extension().string();

                // Match exact filename or directory name, and check extension if it's a file
                if ((filename_without_extension == old_name && (isDir || (isFile && extension == normalized_ext)))) {
                    paths.push_back(entry.path());
                }
            }
        }
    }
    else if (!subDir) {
        for (const auto& entry : fs::directory_iterator(target_directory)) {
            bool isFile = entry.is_regular_file();
            bool isDir = entry.is_directory();
            if ((includeFolders && (isFile || isDir)) || (!includeFolders && isFile)) {
                std::string filename = entry.path().filename().string();
                std::string filename_without_extension = entry.path().stem().string();
                std::string extension = entry.path().extension().string();

                // Match exact filename or directory name, and check extension if it's a file
                if ((filename_without_extension == old_name && (isDir || (isFile && extension == normalized_ext)))) {
                    paths.push_back(entry.path());
                }
            }
        }
    }

    // Step 2: Sort paths by depth, descending
    std::sort(paths.begin(), paths.end(), [](const fs::path& a, const fs::path& b) {
        return std::distance(a.begin(), a.end()) > std::distance(b.begin(), b.end());
        });

    // Step 3: Rename
    for (const auto& path : paths) {
        std::string filename = path.filename().string();
        std::string new_filename = filename.replace(filename.find(old_name), old_name.length(), new_name);
        fs::path new_path = path.parent_path() / new_filename;
        try {
            fs::rename(path, new_path);
            std::cout << "Renamed " << path << " to " << new_path << std::endl;

            renamedFileString << "Renamed " << path.filename() << " to " << new_path.filename() << std::endl;
            displayFilesRenamed.push_back(renamedFileString.str());
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "Failed to rename " << path << " to " << new_path.filename() << ": " << e.what() << std::endl;
        }
    }
}

// Renames file if old_name is found in part of file name (including extension)
void FileRenamerPartial_Ext(const std::string& FilePath, const std::string& old_name, const std::string& new_name, std::string& ext, bool subDir, bool includeFolders) {
    fs::path target_directory = FilePath;
    std::vector<fs::path> paths;
    std::string normalized_ext = (ext.front() == '.') ? ext : "." + ext; // Ensure the extension starts with a dot.
    std::ostringstream renamedFileString;

    // Step 1: Collect Paths
    if (subDir) {
        for (const auto& entry : fs::recursive_directory_iterator(target_directory)) {
            bool isFile = entry.is_regular_file();
            bool isDir = entry.is_directory();
            if ((includeFolders && (isFile || isDir)) || (!includeFolders && isFile)) {
                std::string filename = entry.path().filename().string();
                std::string filename_without_extension = entry.path().stem().string();
                std::string extension = entry.path().extension().string();

                // Match exact filename or directory name, and check extension if it's a file
                if ((entry.path().filename().string().find(old_name) != std::string::npos) && (isDir || (isFile && extension == normalized_ext))) {
                    paths.push_back(entry.path());
                }
            }
        }
    }
    else if (!subDir) {
        for (const auto& entry : fs::directory_iterator(target_directory)) {
            bool isFile = entry.is_regular_file();
            bool isDir = entry.is_directory();
            if ((includeFolders && (isFile || isDir)) || (!includeFolders && isFile)) {
                std::string filename = entry.path().filename().string();
                std::string filename_without_extension = entry.path().stem().string();
                std::string extension = entry.path().extension().string();

                // Match exact filename or directory name, and check extension if it's a file
                if ((entry.path().filename().string().find(old_name) != std::string::npos) && (isDir || (isFile && extension == normalized_ext))) {
                    paths.push_back(entry.path());
                }
            }
        }
    }

    // Step 2: Sort paths by depth, descending
    std::sort(paths.begin(), paths.end(), [](const fs::path& a, const fs::path& b) {
        // Count the depth by counting the number of elements in the path
        return std::distance(a.begin(), a.end()) > std::distance(b.begin(), b.end());
        });


    // Step 3: Rename
    for (const auto& path : paths) {
        auto new_path = path.parent_path() / path.filename().string().replace(path.filename().string().find(old_name), old_name.length(), new_name);
        try {
            fs::rename(path, new_path);
            std::cout << "Renamed " << path << " to " << new_path << std::endl;

            renamedFileString << "Renamed " << path.filename() << " to " << new_path.filename() << std::endl;
            displayFilesRenamed.push_back(renamedFileString.str());
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "Failed to rename " << path << " to " << new_path.filename() << ": " << e.what() << std::endl;
        }
    }
}

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

void DrawNanoVGTextBox(NVGcontext* vg, float x, float y, float width, float height, float strokeWidth) {
    nvgBeginPath(vg); //Starts a new path
    nvgRect(vg, x, y, width, height); // Rectangle at position (100, 100) with width 200 and height 150
    nvgStrokeWidth(vg, strokeWidth); // Set the width of the stroke to 2.0f
    nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); // Stroke color (black)
    nvgFill(vg); // Fill the path
    nvgStroke(vg); // Draw the stroke
}

void DrawNanoVGText(NVGcontext* vg, float x, float y, float size, int funcFont, const char *displayText) {
    nvgBeginPath(vg); //Starts a new path
    nvgFontSize(vg, size);
    nvgFontFaceId(vg, funcFont); // Use the font ID instead of the name
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
    nvgText(vg, x, y, displayText, NULL);
}

void DrawNanoVGButton(NVGcontext* vg, float x, float y, float width, float height) {
    nvgBeginPath(vg); //Starts a new path
    nvgRect(vg, x, y, width, height); //Draws a rectangle
    nvgFill(vg); //Fills the path with the current fill style
    nvgStrokeWidth(vg, 1.0f); //Sets line width of stroke
    nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 255)); //Sets the color of stroke
    nvgStroke(vg); // Makes stroke
}

void DrawNanoVGTextOnButton(NVGcontext* vg, float x, float y, float size, int funcFont, const char* displayText) {
    nvgBeginPath(vg); //Starts a new path
    nvgFontSize(vg, size);
    nvgFontFaceId(vg, funcFont); // Use the font ID instead of the name
    nvgFillColor(vg, nvgRGBA(0, 0, 0, 255));
    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE); //Aligns the text to center
    nvgText(vg, x, y, displayText, NULL);
}

void ImGuiCheckBox(float x, float y, bool& CheckboxValue, const char *CheckboxID, const char *CheckboxLabel) {
    ImGui::SetNextWindowPos(ImVec2(x,y));
    ImGui::Begin(CheckboxID, nullptr,
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

    // Create a checkbox Variable containing Checkbox Value
    static bool checkboxValue1 = false;
    bool& CheckBoxValRef1 = checkboxValue1; // Reference to the checkbox value
    ImGui::Checkbox(CheckboxLabel, &CheckboxValue); // ##checkbox is an empty identifier to avoid label text

    ImGui::End(); //End of Checkbox 1
}

template <std::size_t arr_size>
void ImGuiInputTextBox(float x, float y, float width, float height, const char* InputBoxID, char(&text)[arr_size]) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::Begin(InputBoxID, nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground
    );

    // Create Text Variable to gather text input for input Box
    ImGui::InputText(InputBoxID, text, arr_size);
    ImGui::End(); // End of Input Box
}

void onExit() {
    try {
        std::cout << "Application Exited." << std::endl;
    }
    catch (const fs::filesystem_error& e) {
        std::cerr << "Error Upon Exiting Application" << ": " << e.what() << std::endl;
    }
}

#endif Main_Functions