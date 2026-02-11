from aliot.aliot_obj import AliotObj
import time
import threading

import faceId as FE

#-- Mike Ducasse Dudley --#
rasp_Porte = AliotObj("rasp-Porte")

current_door_state = "inconnu"

def on_door_change(new_state):
    global current_door_state
    current_door_state = new_state
    print("État porte:", current_door_state)

FE.on_door_change(on_door_change)

def ouvrir_porte(data):
    FE.set_door(True)

def fermer_porte(data):
    FE.set_door(False)

def start():
    threading.Thread(target=FE.start_faceid, daemon=True).start()

    while True:
        rasp_Porte.update_doc({
            "/doc/door_state": current_door_state
        })
        print("Porte:", current_door_state)
        print("--------------------------")
        time.sleep(1)


rasp_Porte.on_start(start)
rasp_Porte.on_action_recv("ouvrir_porte", ouvrir_porte)
rasp_Porte.on_action_recv("fermer_porte", fermer_porte)
rasp_Porte.run()
