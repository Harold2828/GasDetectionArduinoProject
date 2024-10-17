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
