/**
 * @file receiver.cpp
 * @brief C++ Frontend: UDP Receiver, State Machine, and TTS Engine
 */

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>

using namespace std;

#define PORT 5005
#define BUFFER_SIZE 1024

// Function to invoke the TTS engine (espeak) in the background
void speak(const string& text) {
    string safe_text = text;
    // Replace spaces with underscores to prevent shell command errors
    replace(safe_text.begin(), safe_text.end(), ' ', '_');
    string command = "espeak \"" + safe_text + "\" &";
    system(command.c_str());
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];

    // 1. Create UDP socket (POSIX standard)
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        cerr << "Error: Socket creation failed!" << endl;
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 2. Bind the socket to the port
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error: Bind failed! Is the port already in use?" << endl;
        close(sockfd);
        return -1;
    }

    cout << "========================================" << endl;
    cout << " C++ Trunk Online. Listening on Port " << PORT << endl;
    cout << "========================================" << endl;

    socklen_t len = sizeof(client_addr);
    string last_spoken = "";

    // 3. Main listening loop
    while (true) {
        int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, MSG_WAITALL, 
                        (struct sockaddr *)&client_addr, &len);
        buffer[n] = '\0';
        string received_text(buffer);

        // 4. State Machine & NLP Logic
        // Ignore meaningless buffer filling states
        if (received_text != "Gathering Data..." && received_text != "Static Gesture") {
            
            // Prevent repeating the same word constantly
            if (received_text != last_spoken) {
                cout << "\n>>> [DYNAMIC INTENT DETECTED]: " << received_text << " <<<" << endl;
                
                // --- NLP Polish (Optional) ---
                // Transform raw gesture labels into natural sentences
                string speech_text = received_text;
                if (received_text.find("Thank you") != string::npos) {
                    speech_text = "Thank you very much";
                }

                // Trigger audio
                speak(speech_text);
                
                // Update state
                last_spoken = received_text;
            }
        } else {
            // Reset the state machine when hand stops moving
            // This allows the user to repeat the SAME gesture after a pause
            if (received_text == "Static Gesture") {
                last_spoken = ""; 
            }
        }
    }

    close(sockfd);
    return 0;
}
