"""
@file data_collector.py
@brief Temporal Gesture Dataset Creator (Records 105D flattened feature vectors to CSV)
"""

import cv2
import numpy as np
import tensorflow.lite as tflite
import time
import math
import os
import csv
from collections import deque

# ==========================================
# DATASET CONFIGURATION
# ==========================================
CSV_FILE = "gesture_dataset.csv"
# Map keyboard keys '1', '2', '3', '4' to gesture labels
LABEL_MAP = {
    ord('1'): "ThankYou",
    ord('2'): "Hello",
    ord('3'): "Yes",
    ord('4'): "No"
}
current_label = "ThankYou" # Default label
recording_feedback_frames = 0

vector_memory = deque(maxlen=15)
last_wrist_x = 0.0
last_wrist_y = 0.0

def calculate_distance(p1_idx, p2_idx, landmarks, roi_w, roi_h):
    x1 = (landmarks[p1_idx * 3] / 256.0) * roi_w
    y1 = (landmarks[p1_idx * 3 + 1] / 256.0) * roi_h
    x2 = (landmarks[p2_idx * 3] / 256.0) * roi_w
    y2 = (landmarks[p2_idx * 3 + 1] / 256.0) * roi_h
    return math.hypot(x2 - x1, y2 - y1)

print("Loading TFLite model...")
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

print("\n=======================================================")
print(" DATA COLLECTOR ONLINE")
print(" Press '1'-'4' to change the Target Label.")
print(" Press 'r' to RECORD the last 15 frames into CSV.")
print(" Press ESC to exit.")
print("=======================================================\n")

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
        
        # 1. Feature Engineering
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

        # Append 7D vector to sliding window
        feature_vector = [delta_x, delta_y, t_ratio, i_ratio, m_ratio, r_ratio, p_ratio]
        vector_memory.append(feature_vector)

        # Draw Landmarks
        for i in range(21):
            x = int((landmarks[i * 3] / 256.0) * roi_w)
            y = int((landmarks[i * 3 + 1] / 256.0) * roi_h)
            cv2.circle(frame, (roi_x + x, roi_y + y), 5, (0, 0, 255), -1)

    # UI Display
    cv2.putText(frame, f"Target: {current_label}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 0), 2)
    cv2.putText(frame, f"Buffer: {len(vector_memory)}/15", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)

    # Visual Feedback for Recording
    if recording_feedback_frames > 0:
        cv2.putText(frame, ">>> SAVED! <<<", (200, 240), cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 0, 255), 4)
        recording_feedback_frames -= 1

    cv2.imshow("Dataset Collector", frame)

    # ==========================================
    # KEYBOARD CONTROLS
    # ==========================================
    key = cv2.waitKey(5) & 0xFF
    if key == 27: # ESC
        break
    elif key in LABEL_MAP:
        current_label = LABEL_MAP[key]
        print(f"Switched label to: {current_label}")
    elif key == ord('r') or key == ord('R'):
        # Only record if the memory buffer is completely full
        if len(vector_memory) == 15:
            
            # FLATTEN the 15x7 matrix into a single 105-dimensional list
            flat_vector = []
            for vec in vector_memory:
                flat_vector.extend(vec)
            
            # Append the text label at the very end
            flat_vector.append(current_label)

            # Write to CSV
            file_exists = os.path.isfile(CSV_FILE)
            with open(CSV_FILE, mode='a', newline='') as f:
                writer = csv.writer(f)
                # Write headers if file is brand new
                if not file_exists:
                    headers = [f"f_{i}" for i in range(105)] + ["label"]
                    writer.writerow(headers)
                writer.writerow(flat_vector)
            
            print(f"[RECORDED] {current_label} -> gesture_dataset.csv")
            recording_feedback_frames = 10 # Flash message for 10 frames
            
            # Purge the buffer to prevent accidentally recording the exact same data twice
            vector_memory.clear() 

cap.release()
cv2.destroyAllWindows()
