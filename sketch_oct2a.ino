#include <SoftwareSerial.h>
#include <ArduinoJson.h>

const int MQ2_PIN = A5;  
const int MQ135_PIN = A2; 

SoftwareSerial gsm(7, 8); 

String phoneNumber = "+573138133118";
const int ARRAY_SIZE = 10;
String sensorDataArray[ARRAY_SIZE];

void setupGSM() {
  gsm.begin(9600);
  delay(1000);
  Serial.println("Initializing GSM module...");
  gsm.println("AT");
  delay(1000);
  gsm.println("AT+CMGF=1");
  delay(1000);
  gsm.println("AT+CGNSPWR=1"); // Power on GPS
  delay(1000);
}

void sendSMS(String message) {
  gsm.print("AT+CMGS=\"");
  gsm.print(phoneNumber);
  gsm.println("\"");
  delay(1000);
  gsm.print(message);
  delay(1000);
  gsm.write(26);
  delay(1000);
  Serial.println("SMS sent: " + message);
}

float readSensor(int pin, String sensorType) {
  int sensorValue = analogRead(pin);
  float voltage = sensorValue * (5.0 / 1023.0);

  if (sensorType == "MQ2") {
    if (voltage > 0.3 && voltage < 0.5) {
      sendSMS("MQ2 detecta: Monóxido de carbono (CO)");
    } else if (voltage > 0.5 && voltage < 0.7) {
      sendSMS("MQ2 detecta: Gas Licuado de Petróleo (LPG)");
    }
  } else if (sensorType == "MQ135") {
    if (voltage > 0.2 && voltage < 0.4) {
      sendSMS("MQ135 detecta: Dióxido de carbono (CO2)");
    } else if (voltage > 0.4 && voltage < 0.6) {
      sendSMS("MQ135 detecta: Amoníaco (NH3)");
    }
  }
  return voltage * 100;
}

// Function to get coordinates using AT+CGNSINF
void getCoordinates(float &latitude, float &longitude) {
  gsm.println("AT+CGNSINF"); // Send command to get GNSS information
  delay(1000);
  
  String gpsData = "";
  while (gsm.available()) {
    char c = gsm.read();
    gpsData += c;
  }

  // Example response: +CGNSINF: 1,1,20210101010101.000,37.7749,-122.4194,10.0,...
  Serial.println("GPS Data: " + gpsData);

  // Parse the response
  int latStart = gpsData.indexOf(',') + 3; // Skip first two commas
  int latEnd = gpsData.indexOf(',', latStart);
  latitude = gpsData.substring(latStart, latEnd).toFloat();

  int lonStart = latEnd + 1;
  int lonEnd = gpsData.indexOf(',', lonStart);
  longitude = gpsData.substring(lonStart, lonEnd).toFloat();

  Serial.print("Latitude: ");
  Serial.println(latitude);
  Serial.print("Longitude: ");
  Serial.println(longitude);
}

void setup() {
  Serial.begin(9600);
  setupGSM();
  
  for (int i = 0; i < ARRAY_SIZE; i++) {
    sensorDataArray[i] = "";
  }
}

void loop() {
  float mq2PPM = readSensor(MQ2_PIN, "MQ2");
  float mq135PPM = readSensor(MQ135_PIN, "MQ135");
  
  float latitude, longitude;
  getCoordinates(latitude, longitude);

  StaticJsonDocument<200> doc;
  doc["mq2"] = mq2PPM;
  doc["mq135"] = mq135PPM;
  doc["latitude"] = latitude;
  doc["longitude"] = longitude;

  String jsonString;
  serializeJson(doc, jsonString);

  for (int i = 0; i < ARRAY_SIZE - 1; i++) {
    sensorDataArray[i] = sensorDataArray[i + 1];
  }
  sensorDataArray[ARRAY_SIZE - 1] = jsonString;

  Serial.println("Sensor Data Array:");
  Serial.println("[");
  for (int i = 0; i < ARRAY_SIZE; i++) {
    Serial.println(sensorDataArray[i]);
    if (i < ARRAY_SIZE - 1) {
      Serial.println(",");
    }
  }
  Serial.println("]");
  
  delay(5000); // Wait 5 seconds before reading again
}