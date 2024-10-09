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

float readSensor(int pin, String sensorType, String &gasType) { 
    int sensorValue = analogRead(pin); 
    float voltage = sensorValue * (5.0 / 1023.0); 
    
    // Determine the type of gas based on the voltage range
    if (sensorType == "MQ2") {
        if (voltage > 0.3 && voltage < 0.5) { 
            gasType = "Carbon Monoxide (CO)";
            sendSMS("MQ2 detecta: Monóxido de carbono (CO)");
        } else if (voltage > 0.5 && voltage < 0.7) { 
            gasType = "Liquefied Petroleum Gas (LPG)";
            sendSMS("MQ2 detecta: Gas Licuado de Petróleo (LPG)");
        }
    } else if (sensorType == "MQ135") { 
        if (voltage > 0.2 && voltage < 0.4) { 
            gasType = "Carbon Dioxide (CO2)";
            sendSMS("MQ135 detecta: Dióxido de carbono (CO2)");
        } else if (voltage > 0.4 && voltage < 0.6) { 
            gasType = "Ammonia (NH3)";
            sendSMS("MQ135 detecta: Amoníaco (NH3)");
        }
    }
    
    return voltage * 100;  // Return the parts per million (PPM)
}

// Function to get coordinates using AT+CGNSINF
void getCoordinates(float &latitude, float &longitude) { 
    gsm.println("AT+CGNSINF");  // Send command to get GNSS information 
    delay(1000); 
    String gpsData = ""; 
    while (gsm.available()) { 
        char c = gsm.read(); 
        gpsData += c; 
    }
    
    Serial.println("GPS Data: " + gpsData); 
    
    // Parse the response 
    int latStart = gpsData.indexOf(',') + 3;  // Skip first two commas 
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
    float latitude, longitude; 
    getCoordinates(latitude, longitude);
    
    String mq2GasType = "";
    float mq2PPM = readSensor(MQ2_PIN, "MQ2", mq2GasType);
    
    String mq135GasType = "";
    float mq135PPM = readSensor(MQ135_PIN, "MQ135", mq135GasType);
    
    // Create JSON for MQ2
    StaticJsonDocument<200> doc1;
    doc1["type_of_sensor"] = "MQ2";
    doc1["value_of_sensor"] = mq2PPM / 100;  // Convert back to voltage for the JSON
    doc1["latitude"] = latitude;
    doc1["longitude"] = longitude;
    doc1
