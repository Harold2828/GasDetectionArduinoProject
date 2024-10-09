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

  // Determine type of gas and parts per million based on voltage range
  String typeOfGas;
  float ppm;
  if (sensorType == "MQ2") {
    if (voltage > 0.3 && voltage < 0.5) {
      typeOfGas = "Monóxido de carbono (CO)";
      ppm = voltage * 100 * 10; // Assuming 10 ppm/Volt
    } else if (voltage > 0.5 && voltage < 0.7) {
      typeOfGas = "Gas Licuado de Petróleo (LPG)";
      ppm = voltage * 100 * 20; // Assuming 20 ppm/Volt
    }
  } else if (sensorType == "MQ135") {
    if (voltage > 0.2 && voltage < 0.4) {
      typeOfGas = "Dióxido de carbono (CO2)";
      ppm = voltage * 100 * 5; // Assuming 5 ppm/Volt
    } else if (voltage > 0.4 && voltage < 0.6) {
      typeOfGas = "Amoníaco (NH3)";
      ppm = voltage * 100 * 10; // Assuming 10 ppm/Volt
    }
  }

  // Create JSON object
  DynamicJsonDocument doc(200);
  doc["type_of_sensor"] = sensorType;
  doc["value_of_sensor"] = voltage;
  doc["latitude"] = 0.0; // Initialize latitude and longitude to 0.0
  doc["longitude"] = 0.0;
  doc["parts_per_million"] = ppm;
  doc["type_of_gas"] = typeOfGas;

  String jsonString;
  serializeJson(doc, jsonString);

  return voltage * 100;
}

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

  // Update JSON objects with latitude and longitude
  DynamicJsonDocument docMq2 (200);
  docMq2["type_of_sensor"] = "MQ2";
  docMq2["value_of_sensor"] = mq2PPM / 100; // Convert back to Volts
  docMq2["latitude"] = latitude;
  docMq2["longitude"] = longitude;
  docMq2["parts_per_million"] = mq2PPM;
  String typeOfGasMq2;
  if (mq2PPM / 100 > 0.3 && mq2PPM / 100 < 0.5) {
    typeOfGasMq2 = "Monóxido de carbono (CO)";
  } else if (mq2PPM / 100 > 0.5 && mq2PPM / 100 < 0.7) {
    typeOfGasMq2 = "Gas Licuado de Petróleo (LPG)";
  }
  docMq2["type_of_gas"] = typeOfGasMq2;

  DynamicJsonDocument docMq135(200);
  docMq135["type_of_sensor"] = "MQ135";
  docMq135["value_of_sensor"] = mq135PPM / 100; // Convert back to Volts
  docMq135["latitude"] = latitude;
  docMq135["longitude"] = longitude;
  docMq135["parts_per_million"] = mq135PPM;
  String typeOfGasMq135;
  if (mq135PPM / 100 > 0.2 && mq135PPM / 100 < 0.4) {
    typeOfGasMq135 = "Dióxido de carbono (CO2)";
  } else if (mq135PPM / 100 > 0.4 && mq135PPM / 100 < 0.6) {
    typeOfGasMq135 = "Amoníaco (NH3)";
  }
  docMq135["type_of_gas"] = typeOfGasMq135;

  String jsonStringMq2;
  serializeJson(docMq2, jsonStringMq2);

  String jsonStringMq135;
  serializeJson(docMq135, jsonStringMq135);

  for (int i = 0; i < ARRAY_SIZE - 1; i++) {
    sensorDataArray[i] = sensorDataArray[i + 1];
  }
  sensorDataArray[ARRAY_SIZE - 1] = jsonStringMq2 + "," + jsonStringMq135;

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
