"""
@file feature_extractor.py
@brief Real-time 7D Feature Vector Extraction (Preparation for DTW/LSTM)
"""

import cv2
import numpy as np
import tensorflow.lite as tflite
import time
import math
from collections import deque

# ==========================================
# Temporal Memory for Feature Vectors
# We now store 7D vectors instead of raw 63D coordinates!
# ==========================================
# [delta_x, delta_y, thumb, index, mid, ring, pinky]
vector_memory = deque(maxlen=15)
last_wrist_x = 0.0
last_wrist_y = 0.0

def calculate_distance(p1_idx, p2_idx, landmarks, roi_w, roi_h):
    """Calculate pixel distance between two landmarks."""
    x1 = (landmarks[p1_idx * 3] / 256.0) * roi_w
    y1 = (landmarks[p1_idx * 3 + 1] / 256.0) * roi_h
    x2 = (landmarks[p2_idx * 3] / 256.0) * roi_w
    y2 = (landmarks[p2_idx * 3 + 1] / 256.0) * roi_h
    return math.hypot(x2 - x1, y2 - y1)

print("Loading raw TFLite model for Feature Extraction...")
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

print("Feature Extractor Online...")

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

    if hand_score < 0.5:
        cv2.putText(frame, "No Hand Detected", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
        vector_memory.clear()
        last_wrist_x, last_wrist_y = 0.0, 0.0
    else:
        landmarks = interpreter.get_tensor(landmark_output_index)[0]
        
        # ==========================================
        # FEATURE ENGINEERING CORE
        # ==========================================
        
        # 1. Spatial Features: Finger Flexion Ratios
        # Reference distance: Wrist (0) to Middle Finger MCP (9) represents palm size
        palm_size = calculate_distance(0, 9, landmarks, roi_w, roi_h)
        
        # Prevent division by zero
        if palm_size < 1.0: palm_size = 1.0 

        # Ratios: distance from Tip to Wrist / Palm Size
        # Values > 1.5 usually mean straight, < 1.0 usually mean completely bent
        thumb_ratio = calculate_distance(4, 0, landmarks, roi_w, roi_h) / palm_size
        index_ratio = calculate_distance(8, 0, landmarks, roi_w, roi_h) / palm_size
        mid_ratio   = calculate_distance(12, 0, landmarks, roi_w, roi_h) / palm_size
        ring_ratio  = calculate_distance(16, 0, landmarks, roi_w, roi_h) / palm_size
        pinky_ratio = calculate_distance(20, 0, landmarks, roi_w, roi_h) / palm_size

        # 2. Temporal Features: Instant Velocity
        current_wrist_x = (landmarks[0] / 256.0) * roi_w
        current_wrist_y = (landmarks[1] / 256.0) * roi_h
        
        delta_x = 0.0
        delta_y = 0.0
        
        if last_wrist_x != 0.0 and last_wrist_y != 0.0:
            delta_x = current_wrist_x - last_wrist_x
            delta_y = current_wrist_y - last_wrist_y
            
        last_wrist_x = current_wrist_x
        last_wrist_y = current_wrist_y

        # Build the 7D Vector!
        feature_vector = [delta_x, delta_y, thumb_ratio, index_ratio, mid_ratio, ring_ratio, pinky_ratio]
        vector_memory.append(feature_vector)

        # ==========================================
        # DATA VISUALIZATION
        # ==========================================
        # Print real-time finger states to the screen
        fingers_state = []
        for name, ratio in zip(["T", "I", "M", "R", "P"], [thumb_ratio, index_ratio, mid_ratio, ring_ratio, pinky_ratio]):
            state = "Up" if ratio > 1.5 else "Dn"
            fingers_state.append(f"{name}:{state}")
            
        state_str = " | ".join(fingers_state)
        
        cv2.putText(frame, f"Vector Captured: {len(vector_memory)}/15", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
        cv2.putText(frame, state_str, (10, 110), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 0), 2)
        cv2.putText(frame, f"dX:{delta_x:.1f} dY:{delta_y:.1f}", (10, 150), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 0, 255), 2)

        # Draw Landmarks
        for i in range(21):
            x = int((landmarks[i * 3] / 256.0) * roi_w)
            y = int((landmarks[i * 3 + 1] / 256.0) * roi_h)
            cv2.circle(frame, (roi_x + x, roi_y + y), 5, (0, 0, 255), -1)

    cTime = time.time()
    fps = 1 / (cTime - pTime) if (cTime - pTime) > 0 else 0
    pTime = cTime
    cv2.putText(frame, f"FPS: {int(fps)}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 0, 0), 2)

    cv2.imshow("Feature Extractor", frame)
    if cv2.waitKey(5) == 27: break

cap.release()
cv2.destroyAllWindows()
