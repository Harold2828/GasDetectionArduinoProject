#include <SoftwareSerial.h>

SoftwareSerial gsm(8, 9); // RX, TX

String phoneNumber = "+573138133118"; // Replace with the recipient's phone number

void setup() {
  Serial.begin(9600);
  gsm.begin(9600);
  
  Serial.println("Initializing GSM module...");
  delay(1000);
  
  gsm.println("AT");
  delay(1000);
  
  // Check if the module is responding
  if (gsm.find("OK")) {
    Serial.println("GSM module initialized successfully.");
  } else {
    Serial.println("GSM module initialization failed.");
    return;
  }
  
  gsm.println("AT+CMGF=1"); // Set the GSM module to SMS mode
  delay(1000);
}

void sendSMS(String message) {
  gsm.print("AT+CMGS=\"");
  gsm.print(phoneNumber);
  gsm.println("\"");
  delay(1000);
  gsm.print(message);
  delay(1000);
  gsm.write(26); // ASCII code for Ctrl+Z, which indicates the end of the message
  delay(1000);
  Serial.println("SMS sent: " + message);
}

void loop() {
  sendSMS("Hello, this is a test message from Arduino!");
  delay(60000); // Wait for 1 minute before sending the next message
}
