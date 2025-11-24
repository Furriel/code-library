import paho.mqtt.client as mqtt

BROKER = "192.168.4.1"
PORT   = 1883

def on_connect(client, userdata, flags, rc):
    print("Conectado ao broker, rc =", rc)
    client.subscribe("#")  # assina tudo

def on_message(client, userdata, msg):
    print(f"[{msg.topic}] {msg.payload.decode(errors='ignore')[:200]}")

client = mqtt.Client(client_id="pc_sniffer")
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, PORT, 60)
client.loop_forever()
