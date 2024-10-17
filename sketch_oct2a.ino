#include <SoftwareSerial.h>
#include <ArduinoJson.h>

const int MQ2_PIN = A5;
const int MQ135_PIN = A2;
SoftwareSerial gsm(7, 8);
String phoneNumber = "+573138133118";

const int ARRAY_SIZE = 10;
String sensorDataArray[ARRAY_SIZE];

// API URL
String apiEndpoint = "https://gbuitrago.com/backend/app/service/V1/api.php";

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
    gsm.write(26);  // End of message
    delay(1000);
    Serial.println("SMS sent: " + message);
}

float readSensor(int pin, String sensorType) {
    int sensorValue = analogRead(pin);
    float voltage = sensorValue * (5.0 / 1023.0); // Voltage calculation
    float ppm = 0;
    String typeOfGas;

    // MQ2 gas types and ranges
    if (sensorType == "MQ2") {
        if (voltage > 0.3 && voltage < 0.5) {
            typeOfGas = "Monóxido de carbono (CO)";
            ppm = voltage * 100 * 10;
        } else if (voltage > 0.5 && voltage < 0.7) {
            typeOfGas = "Gas Licuado de Petróleo (LPG)";
            ppm = voltage * 100 * 20;
        }
    }

    // MQ135 gas types and ranges
    if (sensorType == "MQ135") {
        if (voltage > 0.2 && voltage < 0.4) {
            typeOfGas = "Dióxido de carbono (CO2)";
            ppm = voltage * 100 * 5;
        } else if (voltage > 0.4 && voltage < 0.6) {
            typeOfGas = "Amoníaco (NH3)";
            ppm = voltage * 100 * 10;
        }
    }

    // Create JSON object for sensor data
    DynamicJsonDocument doc(200);
    doc["type_of_sensor"] = sensorType;
    doc["value_of_sensor"] = voltage;
    doc["parts_per_million"] = ppm;
    doc["type_of_gas"] = typeOfGas;

    String jsonString;
    serializeJson(doc, jsonString);
    return ppm;
}

void getCoordinates(float &latitude, float &longitude) {
    gsm.println("AT+CGNSINF");
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

// Function to send POST request to API
void sendPostRequest(String jsonPayload) {
    // HTTP POST Request via GSM
    gsm.println("AT+HTTPINIT");
    delay(1000);
    gsm.println("AT+HTTPPARA=\"CID\",1");
    delay(1000);
    gsm.println("AT+HTTPPARA=\"URL\",\"" + apiEndpoint + "\"");
    delay(1000);
    gsm.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
    delay(1000);
    gsm.println("AT+HTTPDATA=" + String(jsonPayload.length()) + ",10000");
    delay(1000);
    gsm.println(jsonPayload);  // Send JSON data
    delay(1000);
    gsm.println("AT+HTTPACTION=1"); // 1 for POST request
    delay(1000);
    gsm.println("AT+HTTPREAD");
    delay(1000);

    // Read response
    String response = "";
    while (gsm.available()) {
        response += gsm.readString();
    }
    Serial.println("Response: " + response);
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

    // Create JSON payload
    DynamicJsonDocument doc(512);
    JsonArray data = doc.to<JsonArray>();

    JsonObject mq2Data = data.createNestedObject();
    mq2Data["sensor_id"] = 1;  // MQ2 sensor
    mq2Data["value_sensor"] = mq2PPM;
    mq2Data["feature_sensor"] = "Parts per million";
    mq2Data["latitude"] = latitude;
    mq2Data["longitude"] = longitude;
    mq2Data["created_by"] = 1;
    mq2Data["updated_by"] = 1;

    JsonObject mq135Data = data.createNestedObject();
    mq135Data["sensor_id"] = 2;  // MQ135 sensor
    mq135Data["value_sensor"] = mq135PPM;
    mq135Data["feature_sensor"] = "Parts per million";
    mq135Data["latitude"] = latitude;
    mq135Data["longitude"] = longitude;
    mq135Data["created_by"] = 1;
    mq135Data["updated_by"] = 1;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Send the POST request
    sendPostRequest(jsonPayload);

    delay(5000);  // Wait 5 seconds before next loop
}
