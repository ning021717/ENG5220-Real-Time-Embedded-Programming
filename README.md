# 🚀 Project Status (Updated: 2026-03-11)
# 🌐 Social Media Link: https://www.instagram.com/signspeakglasses/

## 📌 Latest Milestone: Temporal Action Recognition & Microservice Architecture
The system has successfully evolved from a static component prototype (KNN/HSV) to a **Data-Driven Dynamic Sign Language Recognition System**. We completely overhauled the core to support temporal tracking, machine learning (Random Forest), and inter-process communication (IPC) for a decoupled, robust pipeline.

---

### ✅ Milestone 1 — 2026-02-11 (Hardware & Pipeline Stabilization)
* **Camera Pipeline:** Functional (Raspberry Pi Camera Module v2 / IMX219, libcamera validated).
* **Real-time Capture:** Threaded acquisition loop implemented (non-blocking capture).
* **Gesture Recognition:** Integrated into runtime loop.
* **Output:** Console printing + file logging scaffolded.

### ✅ Milestone 2 — 2026-02-18 (Closed-loop CV System Completed)
* **👁️ Visual Perception:** Camera V2 frame capture + OpenCV preprocessing.
* **🧠 Core Algorithm:** HSV skin segmentation + KNN classification.
* **💾 Data Engineering:** Custom data collection tool + initial dataset (A, B, C).
* **🗣️ Interactive Output:** TTS (Text-to-Speech) integrated for real-time voice feedback.

### ✅ Milestone 3 — 2026-02-24 (Full Dataset Expansion & Repository Recovery)
* **📚 Dataset Completion:** Successfully expanded from 3 classes to the **full static alphabet (A–Z)**.
* **🛡️ Version Control Resilience:** Recovered project core following a local environment reset.
* **🔐 Secure Workflow:** Implemented PAT authentication for secure remote synchronization.
* **📂 Organized Storage:** Standardized directory structure (`/dataset/A-Z/`) for automated model training.

### 🌟 NEW: Milestone 4 — 2026-03-11 (Dynamic Temporal Recognition & Microservice Architecture)
* **👁️ Deep Learning Vision:** Upgraded from fragile HSV color detection to robust **TFLite Bare-metal Tensor Inference** (21-point hand skeleton tracking).
* **⏱️ Temporal Engineering:** Engineered a 15-frame sliding window to capture physical movement trajectories. Extracted **7D Feature Vectors** (Instant Velocity `dX/dY` + Normalized Finger Flexion Ratios).
* **🧠 Machine Learning Engine:** Built a custom data collector and trained a **Random Forest Classifier** achieving ~92% validation accuracy on dynamic gesture trajectories (e.g., *Thank You*, *Hello*).
* **🛡️ System Hardening:** Diagnosed and bypassed Raspberry Pi's OverlayFS (Read-Only RAM disk limitation) to unlock full 64-bit OS / 64GB storage capabilities for deep learning deployment. Implemented *Absolute Temporal Cooldown* and *Hallucination Filters* to eliminate hardware debounce and return-stroke noise.

---

## 📊 Current Capability
* **Dynamic Word Recognition:** Shifted from recognizing static letters to understanding dynamic, multi-frame physical gestures based on temporal paths.
* **Environmental Robustness:** Totally immune to background clutter, lighting changes, and skin-tone variations thanks to TFLite skeletal extraction.
* **Decoupled Performance:** The AI vision pipeline runs independently from the UI/Audio logic (C++), communicating seamlessly via UDP (`Port 5005`).
* **Instant Extensibility:** New gestures can be added strictly via data-driven workflows (record CSV -> train `.pkl`) without altering core routing logic.



---

## 📂 Project Structure

```text
.
│
├── 🏛️ Legacy C++ Core (Milestone 1-3)
│   ├── main.cpp, header.h      # Legacy monolithic entry point
│   ├── CameraManager.cpp/.hpp  # OOP wrapper for libcamera pipeline
│   ├── GestureRecognizer.cpp   # Legacy HSV/Color-threshold logic
│   ├── capture_images.cpp      # Multi-threaded image acquisition tool
│   ├── train.cpp, predict.cpp  # Legacy KNN training & inference logic
│   └── hand_gesture.cpp        # Static gesture bounding box calculator
│
├── 🧪 Testing & Validation
│   ├── defects_test.cpp        # Unit tests for hardware/pipeline defects
│   └── SignDatabaseTest.cpp    # Tests for sign mapping storage
│
├── ⚙️ Build & Config
│   ├── CMakeLists.txt          # Unified C++ build configuration (Receiver & Legacy)
│   └── .gitignore              # Ignores large datasets (/dataset), .pkl, .tflite, and .xml models
│
└── README.md                   # Project documentation & logs
(Note: Large binary models like hand_landmark.tflite, gesture_rf_model.pkl, and the raw image /dataset/ folder are intentionally excluded from version control via .gitignore to maintain repository performance.)
