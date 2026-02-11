from aliot.aliot_obj import AliotObj
import maison_utils as M
import time
import cv2
import mediapipe as mp
import threading
import traceback

#-- Hua Thu Khoa --#
mp_hands = mp.solutions.hands
mp_draw = mp.solutions.drawing_utils

hands = mp_hands.Hands(
    max_num_hands=1,
    min_detection_confidence=0.6,
    min_tracking_confidence=0.6
)

cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("Erreur : Caméra non détectée")

def fingers_up(hand):
    fingers = []

    if hand.landmark[4].x < hand.landmark[3].x:
        fingers.append(1)
    else:
        fingers.append(0)

    for tip in [8, 12, 16, 20]:
        if hand.landmark[tip].y < hand.landmark[tip - 2].y:
            fingers.append(1)
        else:
            fingers.append(0)

    return fingers

def detect_hand_loop():
    global manual_mode_led
    print(">>> detect_hand_loop START")

    try:
        while True:
            ret, frame = cap.read()
            if not ret:
                print("Frame non lue...")
                continue

            rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            results = hands.process(rgb)

            if results.multi_hand_landmarks:
                hand_landmarks = results.multi_hand_landmarks[0]

                if not hasattr(hand_landmarks, "landmark"):
                    continue

                mp_draw.draw_landmarks(frame, hand_landmarks, mp_hands.HAND_CONNECTIONS)

                try:
                    fingers = fingers_up(hand_landmarks)
                except:
                    continue

                # LED
                if fingers == [0,0,0,0,0]:
                    manual_mode_led = True
                    M.set_led(False)
                    print("✊ Poing → LED OFF")

                elif fingers == [1,0,0,0,0]:
                    manual_mode_led = True
                    M.set_led(True)
                    print("👍 Pouce → LED ON")

                # PORTE
                elif fingers == [1,1,1,1,1]:
                    M.set_door(True)
                    print("✋ Main ouverte → PORTE OUVERTE")

                elif fingers == [0,1,0,0,0]:
                    M.set_door(False)
                    print("☝️ Index → PORTE FERMÉE")

            cv2.imshow("Hand Detection", frame)

            if cv2.waitKey(1) & 0xFF == 27:
                print(">>> ESC pressé")
                break

    except Exception as e:
        print("EXCEPTION detect_hand_loop:", e)
        traceback.print_exc()

    finally:
        cap.release()
        cv2.destroyAllWindows()
        print(">>> detect_hand_loop END")


rasp_Maison = AliotObj("rasp-Maison")

manual_mode_led = False
manual_mode_buzzer = False

#-- Hua Thu Khoa --#
def allumer_led(data):
    global manual_mode_led
    manual_mode_led = True
    M.set_led(True)

def eteindre_led(data):
    global manual_mode_led
    manual_mode_led = False
    M.set_led(False)

def allumer_buzzer(data):
    global manual_mode_buzzer
    manual_mode_buzzer = True
    M.set_buzzer(True)

def eteindre_buzzer(data):
    global manual_mode_buzzer
    manual_mode_buzzer = False
    M.set_buzzer(False)

def start():
    global manual_mode_led, manual_mode_buzzer

    M.set_led(False)
    M.set_buzzer(False)
    time.sleep(3)

    while True:
        T, H = M.get_temperature_and_humidity()
        L = M.get_luminosity()
        R = M.get_rain()
        D = M.get_door_state()

        if not M._manual_buzzer_override:
            manual_mode_buzzer = False

        print("Temp:", T, "°C | Hum:", H, "%")
        print("Lumière:", L)
        print("Pluie:", R)
        print("Porte:", D)
        print("LED =", M.get_led_state())
        print("BUZZER =", M.get_buzzer_state())
        print("--------------------------------")

        if not manual_mode_led:
            M.set_led(L >= 500)

        if not manual_mode_buzzer:
            M.set_buzzer(R > 500)

        rasp_Maison.update_doc({
            "/doc/temperature": T,
            "/doc/humidity": H,
            "/doc/luminosity": L,
            "/doc/rain": R,
            "/doc/door_state": D,
            "/doc/led_state": M.get_led_state(),
            "/doc/buzzer_state": M.get_buzzer_state(),
            "/doc/manual_mode_led": manual_mode_led,
            "/doc/manual_mode_buzzer": manual_mode_buzzer,
        })

        time.sleep(1)

#-- Mike Ducasse Dudley --#
rasp_Maison.on_start(start)
rasp_Maison.on_action_recv("allumer_led", allumer_led)
rasp_Maison.on_action_recv("eteindre_led", eteindre_led)
rasp_Maison.on_action_recv("allumer_buzzer", allumer_buzzer)
rasp_Maison.on_action_recv("eteindre_buzzer", eteindre_buzzer)

threading.Thread(target=rasp_Maison.run, daemon=True).start()

detect_hand_loop()
