import os
import json
import sqlite3
from datetime import datetime
import paho.mqtt.client as mqtt

DB_PATH = "/app/data/plant_data.db"
MQTT_HOST = os.getenv("MQTT_HOST", "broker.hivemq.com")
MQTT_PORT = int(os.getenv("MQTT_PORT", 1883))
MQTT_TOPIC = os.getenv("MQTT_TOPIC", "dalarna/iot/plantcare")

def init_db():
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS sensor_readings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            temperature REAL,
            humidity REAL,
            moisture REAL,
            light REAL
        )
    """)
    conn.commit()
    conn.close()
    print("Database initialized")

def save_reading(data):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute("""
        INSERT INTO sensor_readings (timestamp, temperature, humidity, moisture, light)
        VALUES (?, ?, ?, ?, ?)
    """, (
        datetime.now().isoformat(),
        data.get("temperature"),
        data.get("humidity"),
        data.get("moisture"),
        data.get("light")
    ))
    conn.commit()
    conn.close()
    print("Saved reading to database")

def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected to MQTT broker with code: {reason_code}")
    topic = f"{MQTT_TOPIC}/sensors"
    client.subscribe(topic)
    print(f"Subscribed to {topic}")

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        print(f"Received: {payload}")
        save_reading(payload)
    except Exception as e:
        print(f"Error processing message: {e}")

if __name__ == "__main__":
    print("Starting data collector...")
    print(f"Connecting to {MQTT_HOST}:{MQTT_PORT}")
    print(f"Topic: {MQTT_TOPIC}/sensors")
    init_db()
    
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_message = on_message
    
    client.connect(MQTT_HOST, MQTT_PORT, 60)
    client.loop_forever()