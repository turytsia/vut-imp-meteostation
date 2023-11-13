import paho.mqtt.client as mqtt
import time

DATA = {
    "Brno": {
        "temperature": "24.4 C",
        "humidity": "46.3 %",
        "visibility": "98.2 %"
    },
    "London": {
        "temperature": "18.1 C",
        "humidity": "84.7 %",
        "visibility": "63.1 %"
    },
    "Paris": {
        "temperature": "32.7 C",
        "humidity": "23.4 %",
        "visibility": "99.7 %"
    }
}
PREFIX_DATA = "[DATA]"
PREFIX_CITIES = "[CITIES]"
PREFIX_CITY = "[CITY]"

CITY = list(DATA.keys())[0]


def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    client.subscribe("test")

def on_message(client, userdata, msg):
    global CITY
    message: str = msg.payload.decode()
    if PREFIX_CITY in message:
        _, city = message.split(" ")
        if city in DATA.keys():
            CITY = city
        else:
            print(f"City {city} is not in {DATA.keys()}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("broker.hivemq.com", 1883, 60)

# Keep the client running
client.loop_start()  # Start the loop in the background

while not client.is_connected():
    time.sleep(1)
    
try:
    while True:
        time.sleep(5)
        # # Publish a message
        client.publish("test", f"{PREFIX_DATA} {','.join(list(DATA[CITY].values()))}\n")
        client.publish("test", f"{PREFIX_CITIES} {','.join(list(DATA.keys()))}\n")
except KeyboardInterrupt:
    print("Exiting loop.")
    # Stop the loop when done
    client.loop_stop()

# Keep the script running for a while
client.loop_stop()  # Stop the loop
