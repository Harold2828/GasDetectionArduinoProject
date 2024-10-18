#include <SoftwareSerial.h>
#include <ArduinoJson.h>

const int MQ2_PIN = A5;
const int MQ135_PIN = A2;
SoftwareSerial gsm(8, 9);
String phoneNumber = "+573138133118";
const int ARRAY_SIZE = 10;
String sensorDataArray[ARRAY_SIZE];

void setupGSM() {
    gsm.begin(9600);
    delay(1000);
    Serial.println("Initializing GSM module...");

    // Initialize the GSM Module
    gsm.println("AT");
    delay(1000);
    gsm.println("AT+CMGF=1");
    delay(1000);
    gsm.println("AT+CGNSPWR=1");  // Power on GPS
    delay(1000);
    gsm.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""); // Set GPRS connection
    delay(1000);
    gsm.println("AT+SAPBR=3,1,\"APN\",\"internet.comcel.com.co\""); // Replace with your APN
    delay(1000);
    gsm.println("AT+SAPBR=1,1"); // Activate GPRS
    delay(1000);
    gsm.println("AT+HTTPINIT");  // Initialize HTTP service
    delay(1000);
}

void sendPOSTRequest() {
    gsm.println("AT+HTTPPARA=\"CID\",1"); // Set bearer profile identifier
    delay(1000);
    gsm.println("AT+HTTPPARA=\"URL\",\"https://gbuitrago.com/backend/app/service/V1/api.php\"");
    delay(1000);

    // Prepare the JSON payload
    gsm.println("AT+HTTPDATA=100,10000"); // Set the data length and timeout
    delay(1000);

    // JSON body
    String jsonData = "[{\"sensor_id\":1,\"value_sensor\":100,\"feature_sensor\":\"Parts per million\",\"created_by\":1,\"updated_by\":1}]";
    gsm.println(jsonData);  // Send the JSON data
    delay(1000);

    gsm.println("AT+HTTPACTION=1"); // Start POST action
    delay(5000); // Wait for server response

    // Read the HTTP response
    gsm.println("AT+HTTPREAD");
    delay(1000);
    while (gsm.available()) {
        char c = gsm.read();
        Serial.print(c); // Print server response
    }

    gsm.println("AT+HTTPTERM"); // Terminate HTTP service
    delay(1000);
}

void setup() {
    Serial.begin(9600);
    setupGSM();
}

void loop() {
    sendPOSTRequest();
    delay(60000); // Send data every 60 seconds
}
