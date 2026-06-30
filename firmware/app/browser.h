/*
 * SDcard_browser.h
 *
 *  Copyright 2024 Phonicbloom Ltd.
 *
 *  Created on: 25 Jul 2025
 *      Author: Claude AI (Opus 4 model), modified by mario
 *
 *  Part of the Pocket Varan firmware. SPDX-License-Identifier: MIT
 *
 *  Find more information at:
 *  http://phonicbloom.com/diy/
 *  http://gechologic.com/gechologists/
 *  http://loopstyler.com/
 *
 */

#include <iostream>
#include <experimental/filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <thread>

#include "hal/display.h"

#ifndef SDCARD_BROWSER_H_
#define SDCARD_BROWSER_H_

namespace fs = std::experimental::filesystem;

class FileSystemBrowser {
private:
    fs::path currentPath;
    std::vector<fs::directory_entry> entries;
    size_t selectedIndex;
    bool inFileView;
    std::string fileContent;
    size_t fileScrollPosition;

    // Button states
    enum Button {
        NONE = 0,
        UP = 1,
        DOWN = 2,
        LEFT = 3,
        RIGHT = 4
    };

    // Debouncing
    std::chrono::steady_clock::time_point lastButtonPress;
    const std::chrono::milliseconds debounceDelay{150};

    Display *disp; //reference to OLED driver object
    int buttons_state = 0;

public:
    FileSystemBrowser(Display *display, const std::string& startPath = ".")
        : currentPath(fs::absolute(startPath))
        , selectedIndex(0)
        , inFileView(false)
        , fileScrollPosition(0)
        , lastButtonPress(std::chrono::steady_clock::now()) {

    	disp = display;
        refreshDirectory();
    }

    // Main update loop - call this repeatedly
    void update() {

    	printf("SDcard_browser()::update()\n");

    	int button = read_buttons();

        // Simple debouncing
        auto now = std::chrono::steady_clock::now();

        if (now - lastButtonPress < debounceDelay) {

        	printf("SDcard_browser()::update(): button #%d debounced! (now=%d, lastButtonPress=%d, debounceDelay=%d)\n", button, now, lastButtonPress, debounceDelay);

        	return;
        }

        if (button != NONE) {

        	printf("SDcard_browser()::update(): button #%d pressed\n", button);

        	lastButtonPress = now;
            handleButton(button);
            updateDisplay();
        }
    }

    void updateButtons(int buttons) {

    	printf("SDcard_browser()::updateButtons(buttons=%d)\n", buttons);

    	buttons_state = buttons;
    }

    // Display current state
    void updateDisplay() {
        std::string line1, line2;

        printf("SDcard_browser()::updateDisplay()\n");

        if (inFileView) {
            // Show file content
            line1 = "FILE: " + currentPath.filename().string();
            line2 = getFileContentLine();
        } else {
            // Show directory listing
            line1 = "DIR: " + currentPath.filename().string();
            if (entries.empty()) {
                line2 = "(empty)";
            } else {
                line2 = getSelectedEntryDisplay();
            }
        }

        //display_text(line1, line2);
        disp->show_menu((char*)line1.data(), (char*)line2.data(), 0);
    }

private:
    // Dummy hardware interface methods
    int read_buttons() {
        // TODO: Replace with actual hardware reading
        // Returns: 0=none, 1=up, 2=down, 3=left, 4=right

    	printf("SDcard_browser()::read_buttons()\n");

    	int bs = buttons_state;
    	buttons_state = 0;

    	return bs;
    }

    void display_text(const std::string& line1, const std::string& line2) {
        // TODO: Replace with actual OLED display code
        std::cout << "Line 1: " << line1 << std::endl;
        std::cout << "Line 2: " << line2 << std::endl;
        std::cout << "---" << std::endl;
    }

    void handleButton(int button) {
        if (inFileView) {
            handleFileViewButton(button);
        } else {
            handleDirectoryViewButton(button);
        }
    }

    void handleDirectoryViewButton(int button) {
        switch (button) {
            case UP:
                if (selectedIndex > 0) {
                    selectedIndex--;
                }
                break;

            case DOWN:
                if (selectedIndex < entries.size() - 1) {
                    selectedIndex++;
                }
                break;

            case LEFT:
                // Go to parent directory
                if (currentPath.has_parent_path() && currentPath != currentPath.root_path()) {
                    currentPath = currentPath.parent_path();
                    refreshDirectory();
                }
                break;

            case RIGHT:
                // Enter subdirectory or open file
                if (!entries.empty()) {
                    auto& selected = entries[selectedIndex];
                    if (fs::is_directory(selected)) {
                        currentPath = selected.path();
                        refreshDirectory();
                    } else if (fs::is_regular_file(selected)) {
                        openFile(selected.path());
                    }
                }
                break;
        }
    }

    void handleFileViewButton(int button) {
        switch (button) {
            case UP:
                if (fileScrollPosition > 0) {
                    fileScrollPosition--;
                }
                break;

            case DOWN:
                fileScrollPosition++;
                break;

            case LEFT:
                // Close file
                inFileView = false;
                fileContent.clear();
                fileScrollPosition = 0;
                break;

            case RIGHT:
                // Do nothing in file view
                break;
        }
    }

    void refreshDirectory() {
        entries.clear();
        selectedIndex = 0;

        try {
            for (const auto& entry : fs::directory_iterator(currentPath)) {
                entries.push_back(entry);
            }

            // Sort entries: directories first, then files
            std::sort(entries.begin(), entries.end(),
                [](const fs::directory_entry& a, const fs::directory_entry& b) {
                    bool aIsDir = fs::is_directory(a);
                    bool bIsDir = fs::is_directory(b);
                    if (aIsDir != bIsDir) return aIsDir;
                    return a.path().filename() < b.path().filename();
                });
        } catch (const fs::filesystem_error& e) {
            // Handle permission errors, etc.
            entries.clear();
        }
    }

    void openFile(const fs::path& filePath) {

    	printf("SDcard_browser()::openFile(const fs::path& filePath=%s)\n", filePath);
    	/*
    	try {
            // For demo purposes, just read first few lines
            // In real use, you'd want to handle large files differently
            std::ifstream file(filePath);
            if (file.is_open()) {
                std::string line;
                fileContent.clear();
                int lineCount = 0;
                while (std::getline(file, line) && lineCount < 100) {
                    fileContent += line + "\n";
                    lineCount++;
                }
                file.close();
                inFileView = true;
                fileScrollPosition = 0;
            }
        } catch (...) {
            fileContent = "Error reading file";
            inFileView = true;
            fileScrollPosition = 0;
        }
        */
    }

    std::string getSelectedEntryDisplay() {
        if (entries.empty()) return "";

        auto& selected = entries[selectedIndex];
        std::string name = selected.path().filename().string();

        // Truncate if too long for display
        const size_t maxLen = 16;
        if (name.length() > maxLen) {
            name = name.substr(0, maxLen - 3) + "...";
        }

        // Add indicator for type
        if (fs::is_directory(selected)) {
            return "[" + name + "]";
        } else {
            return " " + name;
        }
    }

    std::string getFileContentLine() {
        if (fileContent.empty()) return "(empty file)";

        // Find the line at current scroll position
        size_t pos = 0;
        size_t lineNum = 0;
        while (pos < fileContent.length() && lineNum < fileScrollPosition) {
            pos = fileContent.find('\n', pos) + 1;
            lineNum++;
            if (pos == 0) break; // No more newlines
        }

        if (pos >= fileContent.length()) {
            return "(end of file)";
        }

        // Extract current line
        size_t endPos = fileContent.find('\n', pos);
        if (endPos == std::string::npos) {
            endPos = fileContent.length();
        }

        std::string line = fileContent.substr(pos, endPos - pos);

        // Truncate for display
        const size_t maxLen = 20;
        if (line.length() > maxLen) {
            line = line.substr(0, maxLen - 3) + "...";
        }

        return line;
    }
};

#endif /* SDCARD_BROWSER_H_ */
