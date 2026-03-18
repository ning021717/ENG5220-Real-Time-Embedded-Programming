## 🚀 Build and Run Instructions

This project strictly adheres to real-time deterministic design principles, utilizing `libcamera` for video capture, `ALSA`/`espeak` for hardware audio, and multithreading with condition variables for non-blocking I/O.

### 1. Prerequisites
Ensure you have the required dependencies installed on your Raspberry Pi 5:

sudo apt update
sudo apt install libopencv-dev cmake espeak-ng gpiod libgpiod-dev

### 2. Compilation
We use `cmake` for build management. To compile the data collector, trainer, and main application:

mkdir -p build && cd build
cmake ..
make -j4

### 3. Execution Workflow

* **Step 1 (Data Collection):** To record new binary mask gestures:
  `libcamerify ./CaptureImages`

* **Step 2 (Model Training):** To dynamically train the KNN model on existing datasets:
  `./TrainApp`

* **Step 3 (Real-Time Inference):** To launch the multithreaded recognizer with USB Audio Output:
  `libcamerify ./MainApp`



# 🚀 Project Status (Updated: 2026-03-18)
# social media link: https://www.instagram.com/signspeakglasses/

## 📌 Latest Milestone: Final Real-Time Architecture & OOP Integration
The system has successfully transitioned from a functional prototype to a **deterministic, industrial-grade real-time embedded system**. The architecture now fully utilizes event-driven multithreading, strict OOP encapsulation, and dedicated hardware audio routing.

---

### ✅ Milestone 1 — 2026-02-11 (Hardware & Pipeline Stabilization)
* **Camera Pipeline:** Functional (Raspberry Pi Camera Module v2 / IMX219, libcamera validated).
* **Real-time Capture:** Threaded acquisition loop implemented (non-blocking capture).
* **Gesture Recognition:** Integrated into runtime loop.

### ✅ Milestone 2 — 2026-02-18 (Closed-loop CV System Completed)
* **👁️ Visual Perception:** Camera V2 frame capture + OpenCV preprocessing.
* **🧠 Core Algorithm:** HSV skin segmentation + KNN classification.
* **💾 Data Engineering:** Custom data collection tool + initial dataset (A, B, C).

### ✅ Milestone 3 — 2026-02-24 (Full Dataset Expansion & Repository Recovery)
* **📚 Dataset Completion:** Successfully expanded from 3 classes to the **full alphabet (A–Z)**.
* **🛡️ Version Control:** Recovered project core and synchronized local workspace with the remote GitHub repository.

### 🔥 Milestone 4 — 2026-03-18 (Final Deterministic RT System)
* **⚙️ Event-Driven Multithreading:** Implemented strict real-time producer/consumer architecture using `std::condition_variable` and mutexes. Achieved zero polling, zero `sleep()` calls, and 0% idle CPU utilization.
* **🧩 SOLID OOP Encapsulation:** Decoupled monolithic code into highly cohesive classes (`CameraManager`, `GestureRecognizer`, `VoiceSynthesizer`).
* **🔊 Hardware Audio Pipeline:** Integrated ALSA and `espeak-ng` routed through a dedicated USB Sound Card and PAM8403 Amplifier for loud, robust TTS feedback.
* **🛡️ Fault Tolerance & Anti-Spam:** Engineered a self-healing camera loop for physical hardware drops, a POSIX signal interceptor for graceful shutdowns, and a deterministic frame-based state machine to prevent audio spamming.
* **🎯 CV Pipeline Upgrade:** Upgraded `CaptureImages` to dynamically adjust HSV boundaries and save pure Binary Masks, exponentially increasing KNN inference accuracy.

---

## 📊 Current Capability
* **Strict Real-Time Performance:** System processes video streams and triggers hardware audio events flawlessly within real-time deadlines.
* **High-Fidelity AI Inference:** Dynamic KNN model trained on pristine binary masks for all 26 alphabet classes.
* **Industrial-Grade Reliability:** Safely handles hardware disconnects and POSIX interrupt signals (Ctrl+C double-tap force quit) without creating zombie processes.

---

## 📂 Project Structure
```text
.
├── dataset/                   # Organized binary mask gesture images (A-Z)
├── main.cpp                   # Event-driven consumer thread & GUI
├── capture_images.cpp         # Multi-threaded binary mask acquisition tool
├── train.cpp                  # Dynamic feature extraction & KNN training
├── CameraManager.cpp/.hpp     # Hardware-level libcamera producer thread
├── GestureRecognizer.cpp/.hpp # Encapsulated HSV/KNN AI engine
├── VoiceSynthesizer.cpp/.hpp  # ALSA/espeak audio engine thread
├── knn_model.xml              # Trained AI model artifact
└── README.md                  # Project documentation & logs
