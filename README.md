# Alcohol Measurement System

This project implements an alcohol measurement system using an MQ-3 gas sensor connected to an ESP32 microcontroller. The system includes a web server interface for user interaction and data display, along with integration with the Sentilo platform for centralized data storage.

## Features
- Calibration of the MQ-3 sensor for accurate alcohol measurements.
- Web server interface for easy user interaction and initiation of alcohol measurements.
- Real-time display of measurement countdown and results on the web interface.
- Integration with the Sentilo platform for centralized storage and potential data analysis.

## Hardware Components
- ESP32 microcontroller.
- MQ-3 gas sensor.
- WiFi module for connectivity.
- Web server for user interface.

## Setup Instructions
1. Connect the MQ-3 sensor and ESP32 following the provided hardware diagram.
2. Update WiFi credentials, Sentilo server details, and sensor information in the code.
3. Upload the code to the ESP32 using the Arduino IDE.
4. Open the Serial Monitor to view calibration and connection details.
5. Access the web interface by connecting to the ESP32's IP address.

## Usage
1. Connect to the web interface.
2. Click the "Start Test" button to initiate a 5-second countdown.
3. Blow into the sensor during the countdown.
4. View real-time results on the web interface.
5. Results are also sent to the Sentilo platform for centralized storage.


## Collaboratos
- Gerard Escardó Cabrerizo
- Pol Sedó i Mota
- Oriol Garcia Vila
