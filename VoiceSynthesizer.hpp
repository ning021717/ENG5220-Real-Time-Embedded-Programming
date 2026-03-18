#ifndef VOICE_SYNTHESIZER_HPP
#define VOICE_SYNTHESIZER_HPP

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

class VoiceSynthesizer {
public:
    VoiceSynthesizer();
    ~VoiceSynthesizer();

    // Wakes up the background thread to speak the letter
    void speak(const std::string& text);

private:
    void workerThread();

    std::thread audioThread;
    std::mutex mtx;
    std::condition_variable cv;
    
    std::string textToSpeak;
    bool hasNewText;
    std::atomic<bool> keepRunning;
};

#endif // VOICE_SYNTHESIZER_HPP
