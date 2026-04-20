# SignSpeak Glasses — Real-Time Sign Language Translator

A real-time embedded system running on Raspberry Pi that recognises American
Sign Language (ASL) hand gestures (A–Z) from a camera and speaks the detected
letter aloud via a text-to-speech engine.

**Architecture:** libcamera hardware event → blocking-I/O callback (producer
thread) → `condition_variable` wakes consumer thread → YCrCb skin segmentation
+ bounding-box-normalised KNN inference → espeak-ng TTS.

Social media: <https://www.instagram.com/signspeakglasses/>

Here's our demonstration for simple British sign language translation

[Watch the video] (https://youtube.com/shorts/U6g8pVhTe1o?si=l6YbsBsCTOMWB2KJ))

- More details about the process, progress, and final product demonstrations can be found on social media.
---

## Hardware Requirements

- Raspberry Pi 4 / 5 running **Debian Trixie** (64-bit)
- Raspberry Pi Camera Module v2 (IMX219) connected via CSI ribbon cable
- USB sound card + speaker (for audio output)
## Hardware 3D printing
The custom hardware chassis for this project was built from scratch.

3D Modeling: Autodesk Fusion 360

3D Printer: Bambu Lab P1S

Material: Standard PLA (1.75mm)
<img width="1470" height="956" alt="截屏2026-04-19 18 22 03" src="https://github.com/user-attachments/assets/62ed1967-e959-4d57-94d7-2a80b05a7fa1" />

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
rectangle; after 6 stable frames the detected letter is spoken aloud. Press
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
├── main.cpp                        # Consumer thread, GUI, signalfd shutdown
├── capture_images.cpp              # libcamera callback → normalised binary-mask collector
├── train.cpp                       # KNN training from dataset/ (bbox-normalised features)
├── augment_dataset.cpp             # Offline dataset augmentation (rotation/noise/perspective)
├── CameraManager.cpp/.hpp          # libcam2opencv wrapper, ROI extraction
├── GestureRecognizer.cpp/.hpp      # YCrCb segmentation + bbox-normalised KNN inference
├── VoiceSynthesizer.cpp/.hpp       # espeak-ng TTS background thread
├── GestureRecognizerUnitTests.cpp  # Unit tests (CI)
├── knn_model.xml                   # Pre-trained KNN model (A–Z)
├── fix_cam.sh                      # Camera module reset utility (run if camera hangs)
├── CMakeLists.txt
└── README.md
```



## Design Highlights

| Principle | Implementation |
|-----------|---------------|
| Blocking I/O wakes threads | libcamera kernel event → `hasFrame()` callback wakes consumer via `condition_variable` |
| No polling / no `sleep()` | Producer thread sleeps in libcamera's `poll()`; signal shutdown via `signalfd` + `read()` |
| C++ virtual-function callbacks | `CameraManager` inherits `Libcam2OpenCV::Callback`; `VoiceSynthesizer` uses `condition_variable` |
| OOP encapsulation | `CameraManager`, `GestureRecognizer`, `VoiceSynthesizer` — each owns its thread and state |
| cmake + CTest | Five targets; CI builds and runs unit tests on every push |

---

## SOLID Design Rationale

| Principle | How it is applied |
|-----------|------------------|
| **Single Responsibility** | Each class has exactly one reason to change: `CameraManager` handles only libcamera I/O and ROI cropping; `GestureRecognizer` handles only computer-vision segmentation and KNN inference; `VoiceSynthesizer` handles only TTS scheduling. `main.cpp` is the thin orchestrator that wires them together. |
| **Open / Closed** | `GestureRecognizer::predict()` can be replaced by a different ML backend (e.g. SVM, neural net) without touching `main.cpp` — the public interface `predict(roi, outMask) → string` is stable. |
| **Liskov Substitution** | `CameraManager::FrameHandler` inherits `Libcam2OpenCV::Callback` and overrides `hasFrame()`. Any code that holds a `Callback*` can use it without knowing the concrete type — the substitution is transparent. |
| **Interface Segregation** | Each class exposes the minimal public API its clients need. `GestureRecognizer` exposes only `predict()` and `isModelLoaded()`; callers are not forced to know about YCrCb thresholds except when they explicitly bind a GUI trackbar. |
| **Dependency Inversion** | `main.cpp` depends on the `FrameCallback` abstraction (`std::function<void(const cv::Mat&)>`), not on the concrete `CameraManager` or libcamera types. Swapping the camera source requires no change to the inference or TTS layers. |

> **Deliberate trade-off — `CR_MIN/MAX`, `CB_MIN/MAX` are public in `GestureRecognizer`.**
> OpenCV's `createTrackbar()` requires a raw `int*` pointer; there is no setter-based variant.
> Making these four ints public is the only way to bind live GUI sliders without introducing global variables.
> All other internal state (`knn`, `IMG_SIZE`, `getLabelText`) remains private.

---

## Real-Time Latency Analysis

The application must process each camera frame and produce a spoken letter within a perceptible response window. Human perception of audio delay becomes noticeable above ≈ 150 ms.

| Stage | Measured / estimated latency | Design decision |
|-------|------------------------------|-----------------|
| libcamera frame period | 33 ms (30 fps) | Hardware limit; acceptable for gesture recognition |
| Camera DMA → callback | < 1 ms | Kernel delivers via blocking `poll()` on media-controller fd |
| `condition_variable` wake (producer → consumer) | < 0.1 ms | Kernel scheduler; no polling overhead |
| YCrCb conversion + morphological ops (440 × 380 px) | ~3–6 ms on Pi 4 | Single-pass; within one frame budget |
| Bounding-box crop + KNN inference (50 × 50 = 2 500 dims) | ~2–8 ms on Pi 4 | Linear scan over training set; dominates if dataset > 2 000 samples |
| Debounce (6 consecutive matching frames) | 6 × 33 ms = ~200 ms | Eliminates false positives; acceptable for letter-by-letter output |
| `aplay` pre-generated WAV playback | ~100 ms startup + audio duration | WAVs pre-generated at startup to avoid 2–3 s cold espeak-ng launch |
| `signalfd` + `read()` shutdown latency | < 1 ms | Kernel delivers signal synchronously to fd; no async-signal-unsafe handler |

**Total inference-to-speech latency** (excluding debounce): ≈ 110–120 ms — well within the 150 ms perceptibility threshold.

The debounce window (6 frames ≈ 200 ms) is a deliberate design choice: gesture recognition on noisy binary masks produces single-frame mispredictions; requiring 6 consecutive agreeing frames filters these without introducing subjectively noticeable lag for a human signer.

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
