"""
@file brain_tflite.py
@brief TFLite Skeleton + Hand Presence Confidence + Absolute Cooldown
"""

import cv2
import numpy as np
import tensorflow.lite as tflite
import time
from collections import deque
import socket

# ==========================================
# Microservice IPC Configuration
# ==========================================
UDP_IP = "127.0.0.1"
UDP_PORT = 5005
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# ==========================================
# Temporal Memory & Absolute Cooldown
# ==========================================
memory_buffer = deque(maxlen=15)
last_trigger_time = 0.0          # Absolute system timestamp
COOLDOWN_DURATION = 1.5          # 1.5 seconds of silence to prevent "Return Stroke" noise

print("Loading raw TFLite model...")
interpreter = tflite.Interpreter(model_path="hand_landmark.tflite")
interpreter.allocate_tensors()

input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

landmark_output_index = -1
score_output_index = -1

# Dynamically extract BOTH the Landmark layer and the Hand Confidence layer
for detail in output_details:
    if len(detail['shape']) > 1 and detail['shape'][1] == 63:
        landmark_output_index = detail['index']
    elif len(detail['shape']) == 2 and detail['shape'][1] == 1:
        # Hand Flag / Confidence Score is usually a [1, 1] tensor
        if score_output_index == -1: 
            score_output_index = detail['index']

cap = cv2.VideoCapture(0, cv2.CAP_V4L2)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

roi_x, roi_y, roi_w, roi_h = 150, 80, 340, 340
pTime = 0

print("Brain Online. Transmitting to C++ Trunk...")

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

    # Fallback to 1.0 if score tensor is surprisingly not found
    hand_score = 1.0 
    if score_output_index != -1:
        hand_score = interpreter.get_tensor(score_output_index)[0][0]

    # ==========================================
    # 1. HALLUCINATION FILTER (Hand Detection)
    # ==========================================
    if hand_score < 0.5:
        # Model is looking at a face or background. Ignore immediately!
        cv2.putText(frame, "No Hand Detected", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 0, 255), 2)
        memory_buffer.clear() 
    else:
        landmarks = interpreter.get_tensor(landmark_output_index)[0]
        memory_buffer.append(landmarks.flatten().tolist())

        # ==========================================
        # 2. ABSOLUTE COOLDOWN (Return Stroke Filter)
        # ==========================================
        if time.time() - last_trigger_time < COOLDOWN_DURATION:
            cv2.putText(frame, "Cooldown Active...", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 255), 2)
            memory_buffer.clear() # Continuously flush buffer so it doesn't build up during cooldown
        else:
            cv2.putText(frame, "Tracking...", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)
            
            # --- Vector Trajectory Logic ---
            if len(memory_buffer) == 15:
                wrist_x_start, wrist_y_start = memory_buffer[0][0], memory_buffer[0][1]
                wrist_x_now, wrist_y_now = memory_buffer[14][0], memory_buffer[14][1]
                
                delta_x = wrist_x_now - wrist_x_start
                delta_y = wrist_y_now - wrist_y_start
                
                trigger_detected = False
                current_intent = ""
                
                if abs(delta_x) > abs(delta_y) and abs(delta_x) > 40.0:
                    current_intent = "No" if delta_x > 0 else "Yes"
                    trigger_detected = True
                elif abs(delta_y) > 30.0:
                    current_intent = "Thank you" if delta_y > 0 else "Hello"
                    trigger_detected = True

                if trigger_detected:
                    sock.sendto(current_intent.encode('utf-8'), (UDP_IP, UDP_PORT))
                    last_trigger_time = time.time() # ACTIVATE ABSOLUTE COOLDOWN!
                    memory_buffer.clear()
                else:
                    sock.sendto(b"Static Gesture", (UDP_IP, UDP_PORT))

        # Draw 21 Landmarks
        for i in range(21):
            x = int((landmarks[i * 3] / 256.0) * roi_w)
            y = int((landmarks[i * 3 + 1] / 256.0) * roi_h)
            cv2.circle(frame, (roi_x + x, roi_y + y), 5, (0, 0, 255), -1)

    cTime = time.time()
    fps = 1 / (cTime - pTime) if (cTime - pTime) > 0 else 0
    pTime = cTime
    
    # Display FPS and Network Confidence Score
    cv2.putText(frame, f"FPS: {int(fps)} | Score: {hand_score:.2f}", (10, 30), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 0, 0), 2)

    cv2.imshow("Python TFLite Brain", frame)
    if cv2.waitKey(5) == 27:
        break

cap.release()
cv2.destroyAllWindows()