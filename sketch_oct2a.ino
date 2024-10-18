#include <SoftwareSerial.h>
#include <ArduinoJson.h>

const int MQ2_PIN = A5;
const int MQ135_PIN = A2;
SoftwareSerial gsm(8, 9);
String phoneNumber = "+573138133118";

const int ARRAY_SIZE = 10;
String sensorDataArray[ARRAY_SIZE];

// GSM setup function
void setupGSM() {
  gsm.begin(9600);
  delay(1000);
  Serial.println("Initializing GSM module...");
  gsm.println("AT");
  delay(1000);
  gsm.println("AT+CMGF=1"); // Set SMS mode
  delay(1000);
  gsm.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""); // Set GPRS mode
  delay(1000);
  gsm.println("AT+SAPBR=3,1,\"APN\",\"internet.comcel.com.co\""); // Replace with your APN
  delay(1000);
  gsm.println("AT+SAPBR=1,1"); // Activate GPRS context
  delay(3000); // Wait for the connection to establish
}

// Send SMS function
void sendSMS(String message) {
  gsm.print("AT+CMGS=\"");
  gsm.print(phoneNumber);
  gsm.println("\"");
  delay(1000);
  gsm.print(message);
  delay(1000);
  gsm.write(26); // ASCII code for Ctrl+Z to send SMS
  delay(1000);
  Serial.println("SMS sent: " + message);
}

// Read sensor function
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

// Get GPS coordinates function
void getCoordinates(float &latitude, float &longitude) {
  gsm.println("AT+CGNSINF"); // Get GNSS information
  delay(1000);
  String gpsData = "";
  while (gsm.available()) {
    char c = gsm.read();
    gpsData += c;
  }
  Serial.println("GPS Data: " + gpsData);

  int latStart = gpsData.indexOf(',') + 3;
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

// Function to send POST request
void sendPostRequest(float mq2PPM, float mq135PPM, float latitude, float longitude) {
  // Start HTTP session
  gsm.println("AT+HTTPINIT");
  delay(1000);

  // Set the URL for POST
  gsm.println("AT+HTTPPARA=\"URL\",\"https://gbuitrago.com/backend/app/service/V1/api.php\"");
  delay(1000);

  // Set the content type to application/json
  gsm.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  delay(1000);

  // Build JSON data
  String jsonBody = "[{\"sensor_id\":1,\"value_sensor\":" + String(mq2PPM) + ",\"feature_sensor\":\"Parts per million\",\"created_by\":1,\"updated_by\":1}]";
  
  // Set POST data size
  gsm.println("AT+HTTPDATA=" + String(jsonBody.length()) + ",10000");
  delay(1000);

  // Send POST data
  gsm.print(jsonBody);
  delay(5000); // Wait for the data to be uploaded

  // Send POST request
  gsm.println("AT+HTTPACTION=1");
  delay(5000); // Wait for response

  // Read response
  gsm.println("AT+HTTPREAD");
  delay(1000);
  while (gsm.available()) {
    char c = gsm.read();
    Serial.print(c);
  }

  // End HTTP session
  gsm.println("AT+HTTPTERM");
  delay(1000);
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

  // Store data in JSON format
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

  // Send the POST request
  sendPostRequest(mq2PPM, mq135PPM, latitude, longitude);

  delay(5000); // Wait 5 seconds before reading again
}

