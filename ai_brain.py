"""
@file ai_brain.py
@brief Final AI Brain: Real-time Feature Extraction + Random Forest Inference + UDP Communication
"""

import cv2
import numpy as np
import tensorflow.lite as tflite
import time
import math
import socket
import joblib
from collections import deque
import warnings

# Suppress scikit-learn warnings about feature names
warnings.filterwarnings("ignore", category=UserWarning)

# ==========================================
# 1. Microservice IPC Configuration
# ==========================================
UDP_IP = "127.0.0.1"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# ==========================================
# 2. Load the Trained AI Chip
# ==========================================
print("Loading Random Forest Gesture Model...")
try:
    rf_model = joblib.load("gesture_rf_model.pkl")
    print("AI Model Loaded Successfully!")
except Exception as e:
    print(f"Error loading model: {e}")
    exit()

# ==========================================
# 3. Temporal Memory & TFLite Setup
# ==========================================
vector_memory = deque(maxlen=15)
last_trigger_time = 0.0
COOLDOWN_DURATION = 1.5
last_wrist_x, last_wrist_y = 0.0, 0.0

def calculate_distance(p1_idx, p2_idx, landmarks, roi_w, roi_h):
    x1 = (landmarks[p1_idx * 3] / 256.0) * roi_w
    y1 = (landmarks[p1_idx * 3 + 1] / 256.0) * roi_h
    x2 = (landmarks[p2_idx * 3] / 256.0) * roi_w
    y2 = (landmarks[p2_idx * 3 + 1] / 256.0) * roi_h
    return math.hypot(x2 - x1, y2 - y1)

print("Loading raw TFLite model for skeleton extraction...")
interpreter = tflite.Interpreter(model_path="hand_landmark.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

landmark_output_index = -1
score_output_index = -1

for detail in output_details:
    if len(detail['shape']) > 1 and detail['shape'][1] == 63:
        landmark_output_index = detail['index']
    elif len(detail['shape']) == 2 and detail['shape'][1] == 1:
        if score_output_index == -1: 
            score_output_index = detail['index']

cap = cv2.VideoCapture(0, cv2.CAP_V4L2)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

roi_x, roi_y, roi_w, roi_h = 150, 80, 340, 340
pTime = 0

print("\n>>> FULL SYSTEM ONLINE <<<")
print(">>> Transmitting AI Predictions to C++ Trunk... <<<\n")

while cap.isOpened():
    success, frame = cap.read()
    if not success: break
    frame = cv2.flip(frame, 1)

    cv2.rectangle(frame, (roi_x, roi_y), (roi_x + roi_w, roi_y + roi_h), (0, 255, 0), 2)
    roi = frame[roi_y:roi_y+roi_h, roi_x:roi_x+roi_w]

    img_resized = cv2.resize(roi, (256, 256))
    img_rgb = cv2.cvtColor(img_resized, cv2.COLOR_BGR2RGB)
    input_data = np.expand_dims(img_rgb, axis=0).astype(np.float32) / 255.0

    interpreter.set_tensor(input_details[0]['index'], input_data)
    interpreter.invoke()

    hand_score = 1.0 
    if score_output_index != -1:
        hand_score = interpreter.get_tensor(score_output_index)[0][0]

    # --- HALLUCINATION FILTER ---
    if hand_score < 0.5:
        cv2.putText(frame, "No Hand", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
        vector_memory.clear()
        last_wrist_x, last_wrist_y = 0.0, 0.0
    else:
        # --- ABSOLUTE COOLDOWN ---
        if time.time() - last_trigger_time < COOLDOWN_DURATION:
            cv2.putText(frame, "Cooldown...", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 255), 2)
            vector_memory.clear()
            last_wrist_x, last_wrist_y = 0.0, 0.0
        else:
            landmarks = interpreter.get_tensor(landmark_output_index)[0]
            
            # --- FEATURE ENGINEERING ---
            palm_size = max(calculate_distance(0, 9, landmarks, roi_w, roi_h), 1.0)
            
            t_ratio = calculate_distance(4, 0, landmarks, roi_w, roi_h) / palm_size
            i_ratio = calculate_distance(8, 0, landmarks, roi_w, roi_h) / palm_size
            m_ratio = calculate_distance(12, 0, landmarks, roi_w, roi_h) / palm_size
            r_ratio = calculate_distance(16, 0, landmarks, roi_w, roi_h) / palm_size
            p_ratio = calculate_distance(20, 0, landmarks, roi_w, roi_h) / palm_size

            current_wrist_x = (landmarks[0] / 256.0) * roi_w
            current_wrist_y = (landmarks[1] / 256.0) * roi_h
            
            delta_x = current_wrist_x - last_wrist_x if last_wrist_x != 0.0 else 0.0
            delta_y = current_wrist_y - last_wrist_y if last_wrist_y != 0.0 else 0.0
            last_wrist_x, last_wrist_y = current_wrist_x, current_wrist_y

            # Add to sliding window
            feature_vector = [delta_x, delta_y, t_ratio, i_ratio, m_ratio, r_ratio, p_ratio]
            vector_memory.append(feature_vector)
            cv2.putText(frame, "Thinking...", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)

            # ==========================================
            # 4. RANDOM FOREST INFERENCE ENGINE
            # ==========================================
            if len(vector_memory) == 15:
                # Calculate total path movement to avoid predicting while hand is totally still
                path_length = sum(abs(v[0]) + abs(v[1]) for v in vector_memory)
                
                # Only predict if there is actual physical movement
                if path_length > 60.0: 
                    # Flatten the 15x7 matrix to 105D
                    flat_vector = []
                    for vec in vector_memory:
                        flat_vector.extend(vec)
                    
                    # AI Prediction!
                    prediction = rf_model.predict([flat_vector])[0]
                    
                    print(f"[{time.strftime('%H:%M:%S')}] AI Detected: {prediction}")
                    
                    # Transmit to C++ Trunk
                    sock.sendto(prediction.encode('utf-8'), (UDP_IP, UDP_PORT))
                    
                    # Display on screen and trigger Cooldown
                    cv2.putText(frame, f"AI: {prediction}", (180, 70), cv2.FONT_HERSHEY_SIMPLEX, 1.2, (0, 255, 255), 3)
                    last_trigger_time = time.time()
                    vector_memory.clear()
                else:
                    sock.sendto(b"Static Gesture", (UDP_IP, UDP_PORT))
                    # Slide the window instead of clearing it, waiting for movement
                    vector_memory.popleft() 

            # Draw Landmarks
            for i in range(21):
                x = int((landmarks[i * 3] / 256.0) * roi_w)
                y = int((landmarks[i * 3 + 1] / 256.0) * roi_h)
                cv2.circle(frame, (roi_x + x, roi_y + y), 5, (0, 0, 255), -1)

    # UI
    cTime = time.time()
    fps = 1 / (cTime - pTime) if (cTime - pTime) > 0 else 0
    pTime = cTime
    cv2.putText(frame, f"FPS: {int(fps)}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 0, 0), 2)

    cv2.imshow("AI Brain", frame)
    if cv2.waitKey(5) == 27: break

cap.release()
cv2.destroyAllWindows()
