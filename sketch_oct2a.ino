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

    DynamicJsonDocument sensorData(1024);
    JsonArray dataArray = sensorData.to<JsonArray>();

    // Sensor 1: MQ2 data
    JsonObject mq2Voltage = dataArray.createNestedObject();
    mq2Voltage["sensor_id"] = 1;  // Example ID for MQ2
    mq2Voltage["value_sensor"] = mq2PPM / 100;  // Convert to voltage
    mq2Voltage["feature_sensor"] = "Voltage";
    mq2Voltage["created_by"] = 1;
    mq2Voltage["updated_by"] = 1;

    JsonObject mq2PPMData = dataArray.createNestedObject();
    mq2PPMData["sensor_id"] = 1;  // Example ID for MQ2
    mq2PPMData["value_sensor"] = mq2PPM;
    mq2PPMData["feature_sensor"] = "Parts per million";
    mq2PPMData["created_by"] = 1;
    mq2PPMData["updated_by"] = 1;

    // Add latitude and longitude for MQ2
    JsonObject mq2Lat = dataArray.createNestedObject();
    mq2Lat["sensor_id"] = 1;
    mq2Lat["value_sensor"] = latitude;
    mq2Lat["feature_sensor"] = "Latitude";
    mq2Lat["created_by"] = 1;
    mq2Lat["updated_by"] = 1;

    JsonObject mq2Long = dataArray.createNestedObject();
    mq2Long["sensor_id"] = 1;
    mq2Long["value_sensor"] = longitude;
    mq2Long["feature_sensor"] = "Longitude";
    mq2Long["created_by"] = 1;
    mq2Long["updated_by"] = 1;

    // Sensor 2: MQ135 data
    JsonObject mq135Voltage = dataArray.createNestedObject();
    mq135Voltage["sensor_id"] = 2;  // Example ID for MQ135
    mq135Voltage["value_sensor"] = mq135PPM / 100;
    mq135Voltage["feature_sensor"] = "Voltage";
    mq135Voltage["created_by"] = 1;
    mq135Voltage["updated_by"] = 1;

    JsonObject mq135PPMData = dataArray.createNestedObject();
    mq135PPMData["sensor_id"] = 2;  // Example ID for MQ135
    mq135PPMData["value_sensor"] = mq135PPM;
    mq135PPMData["feature_sensor"] = "Parts per million";
    mq135PPMData["created_by"] = 1;
    mq135PPMData["updated_by"] = 1;

    // Add latitude and longitude for MQ135
    JsonObject mq135Lat = dataArray.createNestedObject();
    mq135Lat["sensor_id"] = 2;
    mq135Lat["value_sensor"] = latitude;
    mq135Lat["feature_sensor"] = "Latitude";
    mq135Lat["created_by"] = 1;
    mq135Lat["updated_by"] = 1;

    JsonObject mq135Long = dataArray.createNestedObject();
    mq135Long["sensor_id"] = 2;
    mq135Long["value_sensor"] = longitude;
    mq135Long["feature_sensor"] = "Longitude";
    mq135Long["created_by"] = 1;
    mq135Long["updated_by"] = 1;

    // Convert array to string
    String jsonString;
    serializeJson(sensorData, jsonString);

    // Print the JSON string to verify the structure
    Serial.println(jsonString);

    // Send the JSON via SMS or any communication method you prefer
    sendSMS(jsonString);

    delay(5000);  // Wait 5 seconds before reading again
}
