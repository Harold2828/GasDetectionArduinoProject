#include <SoftwareSerial.h>
#include <ArduinoJson.h>

const int MQ2_PIN = A5;
const int MQ135_PIN = A2;
SoftwareSerial gsm(8, 9); // RX, TX
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
  gsm.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""); // Set connection type to GPRS
  delay(1000);
  gsm.println("AT+SAPBR=3,1,\"APN\",\"internet.comcel.com.co\""); // Set your APN (replace "your_apn" with your SIM's APN)
  delay(1000);
  gsm.println("AT+SAPBR=1,1"); // Open GPRS context
  delay(3000);
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

void postToServer(String jsonBody) {
  // Start HTTP session
  gsm.println("AT+HTTPINIT");
  delay(1000);

  // Set HTTP parameters
  gsm.println("AT+HTTPPARA=\"CID\",1");
  delay(1000);
  gsm.println("AT+HTTPPARA=\"URL\",\"https://gbuitrago.com/backend/app/service/V1/api.php\"");
  delay(1000);
  gsm.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  delay(1000);

  // Send data length
  gsm.print("AT+HTTPDATA=");
  gsm.print(jsonBody.length());
  gsm.println(",10000");
  delay(1000);

  // Send the actual JSON data
  gsm.println(jsonBody);
  delay(1000);

  // Start HTTP POST
  gsm.println("AT+HTTPACTION=1");
  delay(5000); // Wait for server response

  // Read server response
  gsm.println("AT+HTTPREAD");
  delay(1000);
  while (gsm.available()) {
    Serial.write(gsm.read()); // Print server response to Serial monitor
  }

  // Terminate HTTP session
  gsm.println("AT+HTTPTERM");
  delay(1000);
}

void getCoordinates(float &latitude, float &longitude) {
  gsm.println("AT+CGNSINF"); // Send command to get GNSS information
  delay(1000);
  String gpsData = "";
  while (gsm.available()) {
    char c = gsm.read();
    gpsData += c;
  }

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

  // Prepare JSON body to be sent
  String jsonBody = "[{\"sensor_id\":1,\"value_sensor\":100,\"feature_sensor\":\"Parts per millioin\",\"created_by\":1,\"updated_by\":1}]";
  
  // Post the JSON body to the server
  postToServer(jsonBody);

  delay(60000); // Wait 60 seconds before posting again
}
