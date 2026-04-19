# SignSpeak Glasses — Real-Time Sign Language Translator

A real-time embedded system running on Raspberry Pi that recognises American
Sign Language (ASL) hand gestures (A–Z) from a camera and speaks the detected
letter aloud via a text-to-speech engine.

**Architecture:** libcamera hardware event → blocking-I/O callback (producer
thread) → `condition_variable` wakes consumer thread → HSV segmentation + KNN
inference → espeak-ng TTS.

Social media: <https://www.instagram.com/signspeakglasses/>

---

## Hardware Requirements

- Raspberry Pi 4 / 5 running **Debian Trixie** (64-bit)
- Raspberry Pi Camera Module v2 (IMX219) connected via CSI ribbon cable
- USB sound card + speaker (for audio output)

---

## 1. Install System Dependencies

Run the following on your Raspberry Pi (fresh Debian Trixie image):

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    pkgconf \
    libopencv-dev \
    libcamera-dev \
    espeak-ng
```

> `pkgconf` is required so that CMake can locate libcamera via `pkg-config`.

---

## 2. Build and Install the libcamera2opencv Wrapper

This project uses [libcamera2opencv](https://github.com/berndporr/libcamera2opencv)
— a thin wrapper that delivers libcamera frames via a C++ virtual-function
callback, providing the hardware-event-driven, blocking-I/O wakeup pattern
required by this course.

```bash
git clone https://github.com/berndporr/libcamera2opencv.git
cd libcamera2opencv
cmake .
make -j4
sudo make install
sudo ldconfig
cd ..
```

---

## 3. Clone This Repository

```bash
git clone https://github.com/ning021717/ENG5220-Real-Time-Embedded-Programming.git
cd ENG5220-Real-Time-Embedded-Programming
```

---

## 4. Build the Project

```bash
mkdir -p build
cd build
cmake ..
make -j4
cd ..
```

All binaries are placed inside `build/`. Run them **from the project root**
(where `knn_model.xml` lives) as shown below.

---

## 5. Run

### Step 1 — Data Collection (optional, dataset already included)

Collect binary-mask training images for a single letter. Run from the project
root:

```bash
./build/CaptureImages
```

Enter the letter (A–Z) when prompted. Adjust the HSV trackbars until the hand
appears white and the background black, then press `s` to save frames and `q`
to quit. Images are saved to `dataset/<LETTER>/`.

### Step 2 — Train the KNN Model (optional, model already included)

```bash
./build/TrainApp
```

Reads all images under `dataset/` and writes `knn_model.xml` to the project
root.

### Step 3 — Real-Time Inference

```bash
./build/MainApp
```

Shows two windows (live ROI + binary mask). Hold a hand gesture inside the blue
rectangle; after 10 stable frames the detected letter is spoken aloud. Press
**ESC** or **Ctrl+C** to exit cleanly.

---

## 6. Run the Unit Tests

```bash
cd build
ctest --output-on-failure -V
```

The `GestureRecognizerUnitTests` suite runs without camera hardware and is also
executed automatically by GitHub Actions CI on every push.

---

## Project Structure

```
.
├── main.cpp                   # Consumer thread, GUI, signalfd shutdown
├── capture_images.cpp         # libcamera callback → binary-mask collector
├── train.cpp                  # KNN training from dataset/
├── CameraManager.cpp/.hpp     # libcam2opencv wrapper, ROI extraction
├── GestureRecognizer.cpp/.hpp # HSV segmentation + KNN inference
├── VoiceSynthesizer.cpp/.hpp  # espeak-ng TTS background thread
├── GestureRecognizerUnitTests.cpp  # Unit tests (CI)
├── knn_model.xml              # Pre-trained KNN model (A–Z)
├── CMakeLists.txt
└── README.md
```

---

## Design Highlights

| Principle | Implementation |
|-----------|---------------|
| Blocking I/O wakes threads | libcamera kernel event → `hasFrame()` callback wakes consumer via `condition_variable` |
| No polling / no `sleep()` | Producer thread sleeps in libcamera's `poll()`; signal shutdown via `signalfd` + `read()` |
| C++ virtual-function callbacks | `CameraManager::FrameHandler` inherits `Libcam2OpenCV::Callback`; `VoiceSynthesizer` uses `condition_variable` |
| OOP encapsulation | `CameraManager`, `GestureRecognizer`, `VoiceSynthesizer` — each owns its thread and state |
| cmake + CTest | Four targets; CI builds and runs unit tests on every push |

---

## Milestones

### Milestone 1 — 2026-02-11 (Hardware & Pipeline Stabilisation)
- Raspberry Pi Camera Module v2 (IMX219) validated with libcamera
- Initial threaded capture and gesture recognition loop

### Milestone 2 — 2026-02-18 (Closed-Loop CV System)
- HSV skin segmentation + KNN classification integrated end-to-end
- Custom data-collection tool; initial dataset (A, B, C)

### Milestone 3 — 2026-02-24 (Full Dataset A–Z)
- Expanded dataset to all 26 letters
- Repository recovered and synchronised with remote

### Milestone 4 — 2026-04-19 (Final Deterministic RT Architecture)
- **libcamera** replaces OpenCV V4L2 polling — camera now driven by kernel
  hardware events via blocking I/O
- **signalfd** replaces `signal()` — shutdown signal handled via blocking
  `read()` on a file descriptor, not an async-signal-unsafe callback
- `CameraManager` refactored to inherit `Libcam2OpenCV::Callback` (virtual
  function callback pattern)
- `capture_images.cpp` refactored with same callback + `condition_variable`
- CMakeLists updated to detect libcamera/cam2opencv; Pi-only targets skipped
  gracefully in CI
