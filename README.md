## 🚀 Project Status

### ✅ Milestone 1 — 2026-02-11 (Hardware + Real-time Pipeline Stabilization)
- **Camera Pipeline:** Functional (Raspberry Pi Camera Module v2 / IMX219, libcamera validated)
- **Real-time Capture:** Threaded acquisition loop implemented (non-blocking capture)
- **Gesture Recognition:** Integrated into runtime loop (feature pipeline not yet validated against live input)
- **Output:** Console printing + file logging scaffolded (pending verified letter outputs)

### ✅ Milestone 2 — 2026-02-18 (Closed-loop CV System Completed)
A complete end-to-end embedded CV pipeline is now operational:

- 👁️ **Visual Perception:** Camera V2 (IMX219) frame capture + OpenCV preprocessing
- 🧠 **Core Algorithm:** HSV skin segmentation + KNN classification
- 💾 **Data Engineering:** Custom data collection tool + dataset for gestures (A, B, C)
- 🗣️ **Interactive Output:** TTS (Text-to-Speech) integrated for real-time voice feedback

**Current Capability**
- Real-time recognition of **A / B / C** with spoken output
- Stable continuous frame acquisition on Raspberry Pi
- Modular pipeline ready for dataset expansion (full alphabet) and latency benchmarking

**Next Steps**
- Expand dataset to full alphabet (A–Z)
- Add temporal smoothing / dynamic gesture support
- Measure and report latency (hand gesture → label → speech)
- Improve robustness under varying lighting conditions
