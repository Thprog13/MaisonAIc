import os
import csv
import time
from datetime import datetime
import threading
import re

import cv2
import mediapipe as mp
from PIL import Image
import imagehash
import serial

#-- Hua Thu Khoa, Antonin Fournier Lequin --#
CAMERA_INDEX = 0
SAVE_DIR = os.path.join("data", "faces")
DB_CSV = os.path.join(SAVE_DIR, "faces_db.csv")

HASH_THRESHOLD = 9
STABILITY_FRAMES = 12

os.makedirs(SAVE_DIR, exist_ok=True)
mp_fd = mp.solutions.face_detection

arduino = None
try:
    arduino = serial.Serial("COM5", 9600, timeout=1)
    time.sleep(2)
    print("Arduino connecté.")
except Exception:
    print("Impossible de se connecter à l’Arduino.")

door_state = "inconnu"
door_pattern = re.compile(r"(DOOR_OPENED|DOOR_CLOSED|SYSTEM_RESET)")

door_callback = None

def on_door_change(cb):
    global door_callback
    door_callback = cb

def get_door_state():
    return door_state

def set_door(state: bool):
    global door_state
    if not arduino:
        return
    try:
        if state:
            arduino.write(b"OPEN\n")
            door_state = "ouvert"
        else:
            arduino.write(b"CLOSE\n")
            door_state = "ferme"
        if door_callback:
            door_callback(door_state)
    except Exception:
        pass

#-- Mike Ducasse Dudley --#
def read_from_arduino():
    global door_state
    while True:
        if not arduino:
            time.sleep(0.2)
            continue
        try:
            line = arduino.readline().decode(errors="ignore").strip()
            if not line:
                continue
            match = door_pattern.search(line)
            if match:
                msg = match.group(1)
                if msg == "DOOR_OPENED":
                    door_state = "ouvert"
                elif msg == "DOOR_CLOSED":
                    door_state = "ferme"
                elif msg == "SYSTEM_RESET":
                    door_state = "ferme"
                if door_callback:
                    door_callback(door_state)
        except Exception:
            pass
        time.sleep(0.05)

threading.Thread(target=read_from_arduino, daemon=True).start()

def load_db():
    if not os.path.exists(DB_CSV):
        return {}
    db = {}
    with open(DB_CSV, "r", encoding="utf-8") as f:
        reader = csv.reader(f)
        for row in reader:
            if not row:
                continue
            first = row[0].strip().lower()
            if first in ("face_id", "phash_hex", "id"):
                continue
            if len(row) < 2:
                continue
            try:
                fid = int(row[0])
            except Exception:
                continue
            ph_hex = row[1].strip()
            try:
                ph = imagehash.hex_to_hash(ph_hex)
            except Exception:
                continue
            db.setdefault(fid, []).append(ph)
    return db

def append_db(face_id, phash):
    if not os.path.exists(DB_CSV):
        with open(DB_CSV, "w", newline="", encoding="utf-8") as f:
            writer = csv.writer(f)
            writer.writerow(["face_id", "phash_hex", "created_at"])
    with open(DB_CSV, "a", newline="", encoding="utf-8") as f:
        csv.writer(f).writerow([face_id, str(phash), datetime.now().isoformat(timespec="seconds")])

def compute_phash(bgr):
    if bgr is None or bgr.size == 0:
        return None
    rgb = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB)
    return imagehash.phash(Image.fromarray(rgb))

def match_face(ph, db):
    best_fid, best_dist = None, 99
    for fid, phashes in db.items():
        for ref in phashes:
            try:
                dist = ph - ref
            except Exception:
                continue
            if dist < best_dist:
                best_dist, best_fid = dist, fid
    if best_dist <= HASH_THRESHOLD:
        return best_fid, best_dist
    return None, None

#-- Hua Thu Khoa, Antonin Fournier Lequin & Mike Ducasse Dudley --#
def start_faceid():
    global door_state
    db = load_db()

    cap = cv2.VideoCapture(CAMERA_INDEX, cv2.CAP_DSHOW)
    cap.set(3, 1280)
    cap.set(4, 720)

    if not cap.isOpened():
        print("Caméra non détectée.")
        return

    stability = 0
    progress = 0

    with mp_fd.FaceDetection(model_selection=0, min_detection_confidence=0.6) as fd:
        while True:
            ok, frame = cap.read()
            if not ok or frame is None:
                continue

            h, w = frame.shape[:2]
            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            res = fd.process(rgb)

            rect = None
            face_crop = None
            matched_id = None
            ph = None

            if res.detections:
                best_score = 0
                for det in res.detections:
                    try:
                        score = det.score[0]
                        bbox = det.location_data.relative_bounding_box
                    except Exception:
                        continue
                    x1 = int(bbox.xmin * w)
                    y1 = int(bbox.ymin * h)
                    x2 = int((bbox.xmin + bbox.width) * w)
                    y2 = int((bbox.ymin + bbox.height) * h)

                    if score > best_score:
                        best_score = score
                        rect = (x1, y1, x2, y2)
                        face_crop = frame[y1:y2, x1:x2]

            if rect is None:
                cv2.imshow("FaceID", frame)
                if cv2.waitKey(1) & 0xFF == 27:
                    break
                continue

            ph = compute_phash(face_crop)
            if ph is not None:
                matched_id, dist = match_face(ph, db)

            x1, y1, x2, y2 = rect
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0,255,0), 2)

            if matched_id is not None:
                stability += 1
            else:
                stability = max(0, stability - 2)
                progress = max(0, progress - 0.1)

            progress = min(1, stability / STABILITY_FRAMES)

            if progress >= 1 and matched_id is not None:
                cv2.putText(frame, "UNLOCK", (x1, y1 - 20), 0, 1, (0,255,0), 2)
                if arduino:
                    try:
                        arduino.write(b"UNLOCK\n")
                    except Exception:
                        pass
                door_state = "ouvert"
                if door_callback:
                    door_callback(door_state)
            else:
                cv2.putText(frame, "Identifying...", (x1, y1 - 20), 0, 1, (255,255,255), 2)

            cv2.imshow("FaceID", frame)
            key = cv2.waitKey(1) & 0xFF

            if key == ord("n") and ph is not None:
                new_id = max(db.keys(), default=0) + 1
                append_db(new_id, ph)
                db.setdefault(new_id, []).append(ph)

            if key == 27:
                break

    cap.release()
    cv2.destroyAllWindows()