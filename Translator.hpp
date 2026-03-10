#ifndef TRANSLATOR_HPP
#define TRANSLATOR_HPP

#include <string>
#include <iostream>
#include <map>

class Translator {
public:
    /**
     * @brief Translate internal action ID to human-readable language (Chinese + English)
     * @param action Internal action identifier (e.g., "Action_A", "Action_B")
     * @note Implements simple debouncing to avoid repeated output of the same translation
     */
    void translate(std::string action) {
        // Mapping dictionary: convert internal action IDs to human-readable language
        static std::map<std::string, std::string> dict = {
            {"Action_A", "你好 (Hello)"},
            {"Action_B", "谢谢 (Thank You)"}
        };

        // Simple debouncing: avoid repeated output of the same translation result
        if (action != lastOutput) { 
            // Print translation (or "Recognizing..." if action not in dictionary)
            // \r: Carriage return to overwrite the same line; std::flush: force output immediately
            std::cout << "\r[Translation]: " << (dict.count(action) ? dict[action] : "Recognizing...") << std::flush;
            lastOutput = action;
        }
    }

private:
    std::string lastOutput;  // Store last translated action for debouncing check
};

#endif // OPENCV_TEST_TRANSLATOR_H  // End of header guard

