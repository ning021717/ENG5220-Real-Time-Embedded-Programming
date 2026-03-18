#include "VoiceSynthesizer.hpp"
#include <iostream>
#include <cstdlib>

VoiceSynthesizer::VoiceSynthesizer() 
    : hasNewText(false), keepRunning(true) {
    
    std::cout << "[INFO] Initializing Audio Engine (USB Sound Card)..." << std::endl;
    audioThread = std::thread(&VoiceSynthesizer::workerThread, this);
}

VoiceSynthesizer::~VoiceSynthesizer() {
    keepRunning = false;
    cv.notify_all();
    if (audioThread.joinable()) {
        audioThread.join();
    }
}

void VoiceSynthesizer::speak(const std::string& text) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        textToSpeak = text;
        hasNewText = true;
    }
    // Wake up the audio thread without consuming CPU
    cv.notify_one(); 
}

void VoiceSynthesizer::workerThread() {
    while (keepRunning) {
        std::string localText;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return hasNewText || !keepRunning; });
            
            if (!keepRunning) break;
            
            localText = textToSpeak;
            hasNewText = false;
        }

        // Synthesize voice using ALSA / espeak to the default USB audio device
        std::cout << "[VOICE] Speaking: " << localText << std::endl;
        std::string speakCmd = "espeak-ng -v en -s 140 -a 200 \"" + localText + "\" > /dev/null 2>&1";
        int ret = system(speakCmd.c_str());
        (void)ret;
    }
}
