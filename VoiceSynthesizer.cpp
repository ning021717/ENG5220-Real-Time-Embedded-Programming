#include "VoiceSynthesizer.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

namespace {
    /** ALSA device for aplay. Override with env SLT_ALSA_DEVICE (e.g. plughw:1,0). */
    std::string alsaPlaybackDevice() {
        const char* env = std::getenv("SLT_ALSA_DEVICE");
        if (env && std::strlen(env) > 0) return std::string(env);
        return "plughw:2,0";
    }
} // namespace

VoiceSynthesizer::VoiceSynthesizer() 
    : hasNewText(false), keepRunning(true) {
    
    std::cout << "[INFO] Pre-generating audio files (A-Z)..." << std::endl;
    // Generate WAV files once at startup so playback uses aplay (fast)
    // instead of cold-starting espeak-ng on every letter (~2-3s overhead).
    for (char c = 'A'; c <= 'Z'; c++) {
        std::string letter(1, c);
        std::string path = "/tmp/slt_" + letter + ".wav";
        std::string cmd = "espeak-ng -v en -s 130 -a 200 -w " + path
                          + " \"" + letter + "\" > /dev/null 2>&1";
        system(cmd.c_str());
    }
    std::cout << "[INFO] Audio Engine ready." << std::endl;
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

        std::cout << "[VOICE] Speaking: " << localText << std::endl;
        std::string wavPath = "/tmp/slt_" + localText + ".wav";
        std::string cmd;
        if (access(wavPath.c_str(), F_OK) == 0) {
            // Pre-generated WAV exists — aplay starts in ~100ms vs espeak-ng ~2-3s.
            // Route explicitly to the USB sound card (plughw:2,0) so aplay doesn't
            // fall back to the HDMI output (card 0).
            cmd = "aplay -q -D " + alsaPlaybackDevice() + " " + wavPath
                  + " > /dev/null 2>&1";
        } else {
            cmd = "espeak-ng -v en -s 130 -a 200 \"" + localText + "\" > /dev/null 2>&1";
        }
        int ret = system(cmd.c_str());
        (void)ret;
    }
}
