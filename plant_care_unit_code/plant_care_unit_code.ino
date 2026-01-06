#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

const char* WIFI_SSID = "Telia-3A66D3";
const char* WIFI_PASS = "6wmrWdFxKfvWAcmd";
const char* MQTT_SERVER = "broker.hivemq.com";
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC = "dalarna/iot/plantcare/";

#define DHTPIN 13
#define MOIS_PIN 34
#define LIGHT_PIN 35
#define MOTOR1_PIN 27
#define MOTOR2_PIN 26
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient mqtt(espClient);
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long lastPublish = 0;
const long PUBLISH_INTERVAL = 15000;
const float MOISTURE_THRESHOLD = 20.0;
const int WATER_DURATION = 5000;
int lcdPage = 0;

void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println();
    Serial.println("Plant Care System Starting...");
    
    dht.begin();
    
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Plant Care");
    lcd.setCursor(0, 1);
    lcd.print("Starting...");
    
    pinMode(MOTOR1_PIN, OUTPUT);
    pinMode(MOTOR2_PIN, OUTPUT);
    digitalWrite(MOTOR1_PIN, LOW);
    digitalWrite(MOTOR2_PIN, LOW);

    Serial.print("Connecting to WiFi");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WiFi...");
    
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
    
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
    delay(2000);

    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setCallback(mqttCallback);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Received [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);

    String controlTopic = String(MQTT_TOPIC) + "/control/pump";
    if (String(topic) == controlTopic) {
        if (message == "ON") {
            Serial.println("Manual pump ON");
            digitalWrite(MOTOR1_PIN, HIGH);
            digitalWrite(MOTOR2_PIN, LOW);
        } else {
            Serial.println("Manual pump OFF");
            digitalWrite(MOTOR1_PIN, LOW);
            digitalWrite(MOTOR2_PIN, LOW);
        }
    }
}

void reconnectMQTT() {
    while (!mqtt.connected()) {
        Serial.print("Connecting to MQTT...");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MQTT...");
        
        String clientId = "PlantCareESP32-" + String(random(0xffff), HEX);
        if (mqtt.connect(clientId.c_str())) {
            Serial.println("connected");
            String controlTopic = String(MQTT_TOPIC) + "/control/pump";
            mqtt.subscribe(controlTopic.c_str());
            lcd.setCursor(0, 1);
            lcd.print("Connected!");
            delay(1000);
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqtt.state());
            Serial.println(". Retrying in 5 seconds...");
            lcd.setCursor(0, 1);
            lcd.print("Retry...");
            delay(5000);
        }
    }
}

void waterPlant() {
    Serial.println("Watering plant for 5 seconds...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Watering...");
    
    digitalWrite(MOTOR1_PIN, HIGH);
    digitalWrite(MOTOR2_PIN, LOW);
    delay(WATER_DURATION);
    digitalWrite(MOTOR1_PIN, LOW);
    digitalWrite(MOTOR2_PIN, LOW);
    
    Serial.println("Watering complete");
}

void updateLCD(float temperature, float humidity, float moisture, float light) {
    lcd.clear();
    
    if (lcdPage == 0) {
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        lcd.print(temperature, 1);
        lcd.print("\xDF" "C");
        
        lcd.setCursor(0, 1);
        lcd.print("Humi: ");
        lcd.print(humidity, 1);
        lcd.print("%");
    } else {
        lcd.setCursor(0, 0);
        lcd.print("Mois: ");
        lcd.print(moisture, 1);
        lcd.print("%");
        
        lcd.setCursor(0, 1);
        lcd.print("Light: ");
        lcd.print(light, 1);
        lcd.print("%");
    }
    
    lcdPage = (lcdPage + 1) % 2;
}

void loop() {
    if (!mqtt.connected()) {
        reconnectMQTT();
    }
    mqtt.loop();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    
    float rawMoisture = analogRead(MOIS_PIN);
    float moisture = constrain((rawMoisture - 2480.0) * 100.0 / (719.0 - 2480.0), 0.0, 100.0);
    
    float rawLight = analogRead(LIGHT_PIN);
    float light = constrain(rawLight * 100.0 / 4095.0, 0.0, 100.0);

    updateLCD(temperature, humidity, moisture, light);

    if (millis() - lastPublish > PUBLISH_INTERVAL) {
        if (!isnan(humidity) && !isnan(temperature)) {
            char payload[128];
            snprintf(payload, sizeof(payload),
                "{\"temperature\":%.1f,\"humidity\":%.1f,\"moisture\":%.1f,\"light\":%.1f}",
                temperature, humidity, moisture, light);
            
            String sensorTopic = String(MQTT_TOPIC) + "/sensors";
            mqtt.publish(sensorTopic.c_str(), payload);
            
            Serial.print("Published: ");
            Serial.println(payload);

            if (moisture < MOISTURE_THRESHOLD) {
                Serial.println("Moisture low! Auto-watering triggered.");
                waterPlant();
            }
        } else {
            Serial.println("Failed to read DHT sensor");
        }
        
        lastPublish = millis();
    }

    delay(1000);
}