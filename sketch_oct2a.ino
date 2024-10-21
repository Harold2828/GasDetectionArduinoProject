#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// Pines de los sensores
const int MQ2_PIN = A5;
const int MQ135_PIN = A2;

// Configuración de GSM
SoftwareSerial gsm(8, 9); // RX, TX para el módulo GSM
String phoneNumber = "+573138133118";

// Tamaño del array de datos de los sensores
const int ARRAY_SIZE = 10;
String sensorDataArray[ARRAY_SIZE];

// Configuración del módulo GSM
// Configuración del módulo GSM
void setupGSM() {
  // Start GPRS setup
  gsm.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\""); // Set connection type to GPRS
  delay(1000);

  // Set the APN
  gsm.println("AT+SAPBR=3,1,\"APN\",\"internet.comcel.com.co\""); // Set the APN for Claro
  delay(1000);

  // Set username
  gsm.println("AT+SAPBR=3,1,\"USER\",\"comcel\""); // Set the username
  delay(1000);

  // Set password
  gsm.println("AT+SAPBR=3,1,\"PWD\",\"comcel\""); // Set the password
  delay(1000);

  // Open the bearer
  gsm.println("AT+SAPBR=1,1"); // Open GPRS context
  delay(3000); // Give time for GPRS connection

  // Check if bearer is opened successfully
  gsm.println("AT+SAPBR=2,1"); // Check bearer status
  delay(1000);

  // Get IP address (optional, but useful for debugging)
  gsm.println("AT+SAPBR=3,1,\"IPADDR\""); // Get IP address
  delay(1000);
  
  // Llama a la función para registrar cualquier respuesta adicional
  logGSMResponse();
}


// Enviar SMS a un número específico
void sendSMS(String message) {
  gsm.print("AT+CMGS=\"");
  gsm.print(phoneNumber);
  gsm.println("\"");
  delay(1000);
  gsm.print(message);
  delay(1000);
  gsm.write(26); // Ctrl+Z para enviar el mensaje
  delay(1000);
  Serial.println("SMS sent: " + message);
}

// Leer datos del sensor
float readSensor(int pin, String sensorType) {
  int sensorValue = analogRead(pin);
  float voltage = sensorValue * (5.0 / 1023.0); // Convierte a voltaje

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
  return voltage * 100; // Retorna el valor en partes por millón (PPM)
}

// Registrar respuesta del módulo GSM
void logGSMResponse() {
  while (gsm.available()) {
    String response = gsm.readString();
    Serial.println("GSM response: " + response);
  }
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
  delay(5000);  // Wait for the server response

  // Read and print server response
  gsm.println("AT+HTTPREAD");
  delay(1000);

  while (gsm.available()) {
    char c = gsm.read();
    Serial.write(c);  // Print server response to Serial Monitor
  }

  // Terminate HTTP session
  gsm.println("AT+HTTPTERM");
  delay(1000);
}

// Verificar conexión a Internet
bool isConnectedToInternet() {
  // Intentar abrir un contexto GPRS
  gsm.println("AT+SAPBR=1,1"); // Abrir contexto GPRS
  delay(3000);

  // Hacer una solicitud HTTP a Google
  gsm.println("AT+HTTPINIT"); // Inicializa la sesión HTTP
  delay(1000);
  gsm.println("AT+HTTPPARA=\"URL\",\"http://www.google.com\""); // URL de Google
  delay(1000);
  
  gsm.println("AT+HTTPACTION=0"); // Realiza la solicitud HTTP GET
  delay(5000); // Espera la respuesta

  // Leer la respuesta del servidor
  String response = "";
  while (gsm.available()) {
    response += gsm.readString();
  }

  // Termina la sesión HTTP
  gsm.println("AT+HTTPTERM");
  delay(1000);

  // Verifica si la respuesta contiene el código de éxito
  if (response.indexOf("200 OK") != -1) {
    Serial.println("Conectado a Internet");
    return true;
  } else {
    Serial.println("No hay conexión a Internet");
    return false;
  }
}

// Obtener coordenadas GPS
void getCoordinates(float &latitude, float &longitude) {
  gsm.println("AT+CGNSINF"); // Comando para obtener información GNSS
  delay(1000);
  String gpsData = "";
  while (gsm.available()) {
    char c = gsm.read();
    gpsData += c;
  }

  // Analizar la respuesta para extraer latitud y longitud
  int latStart = gpsData.indexOf(',') + 3; // Saltar los primeros dos comas
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

// Configuración inicial
void setup() {
  Serial.println("Starting...");
  Serial.begin(9600);
  setupGSM();
  for (int i = 0; i < ARRAY_SIZE; i++) {
    sensorDataArray[i] = "";
  }
}

// Bucle principal
void loop() {
  if (isConnectedToInternet()) {
    float mq2PPM = readSensor(MQ2_PIN, "MQ2");
    float mq135PPM = readSensor(MQ135_PIN, "MQ135");
    float latitude, longitude;
    getCoordinates(latitude, longitude);

    // Preparar el cuerpo JSON para enviar
    String jsonBody = "[{\"sensor_id\":1,\"value_sensor\":" + String(mq2PPM) + ",\"feature_sensor\":\"Parts per million\",\"created_by\":1,\"updated_by\":1}]";

    // Enviar el cuerpo JSON al servidor
    postToServer(jsonBody);
  } else {
    Serial.println("Esperando conexión a Internet...");
  }

  delay(60000); // Esperar 60 segundos antes de enviar de nuevo
}
