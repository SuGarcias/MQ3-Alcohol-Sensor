#include <MQUnifiedsensor.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

/************************WIFI************************************/
const char* ssid = "Vodafone9276";
const char* password = "ADSFU9ZT4RU52J";

AsyncWebServer server(80);

/************************SENTILO************************************/
const char* host = "147.83.83.21";
const char* token = "5829244fb43b7f0218be9077d47cb1050be18eb03bf8c8def57c6e67dd367925";
const char* provider = "grup_3-102@Provider";
const char* sensor = "MQ3_01";

WiFiClient client;
const int httpPort = 8081;

/************************Hardware Related Macros************************************/
#define Board ("ESP32")
#define Pin (32)

/***********************Software Related Macros**************************** ********/
#define Type ("MQ-3")
#define Voltage_Resolution (5)
#define ADC_Bit_Resolution (12)
#define RatioMQ3CleanAir (60)

/*****************************Globals***********************************************/
MQUnifiedsensor MQ3(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type);
unsigned long startTime = 0;
float sumValues = 0;
int sampleCount = 0;
bool measuring = false;

void connectToWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectToWiFi();
  }
}

void reconnectServer() {
  if (!client.connected()) {
    while (!client.connect(host, httpPort)) {
      delay(500);
    }
  }
}

void initializeMQ() {
  MQ3.init();

  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for (int i = 1; i <= 100; i++) {
    MQ3.update();
    calcR0 += MQ3.calibrate(RatioMQ3CleanAir);
    Serial.print(".");
  }

  MQ3.setR0(calcR0 / 100);
  Serial.println("  Calibration completed.");

  if (isinf(calcR0)) {
    Serial.println("Warning: Connection issue, R0 is infinite (open circuit detected), check your wiring and supply.");
    while (1);
  }
  if (calcR0 == 0) {
    Serial.println("Warning: Connection issue, R0 is zero (analog pin short-circuited to ground), check your wiring and supply.");
    while (1);
  }
}

String makePUTRequest(float value) {
  String request = "PUT /data/";
  request += String(provider);
  request += '/';
  request += String(sensor);
  request += '/';
  request += String(value);
  request += " HTTP/1.1\r\nIDENTITY_KEY: ";
  request += String(token);
  request += "\r\n\r\n";

  return request;
}

void sendPUTRequest(float value) {
  String request = makePUTRequest(value);
  client.print(request);
}

void resetMeasurement() {
  measuring = false;
  sumValues = 0;
  sampleCount = 0;
}

void handleGetMeasurementResult(AsyncWebServerRequest *request) {
  if (measuring) {
    MQ3.update();
    float sensorValue = MQ3.readSensor();
    sumValues += sensorValue;
    sampleCount++;
  }

  if (millis() - startTime >= 7000) {
    
    float averageValue = sumValues / sampleCount;
    reconnectServer();
    sendPUTRequest(averageValue);
    Serial.println(averageValue);
    client.stop();
    resetMeasurement();
    request->send(200, "text/plain", String(averageValue));
  } else {
    request->send(200, "text/plain", "Measuring...");
  }
}

void handleStartCountdown(AsyncWebServerRequest *request) {
  if (!measuring) {
    resetMeasurement();
    measuring = true;
    startTime = millis() + 5000;  // Iniciar la cuenta atrás de 5 segundos
    request->send(200, "text/plain", "Blow in 5 seconds...");
  } else {
    request->send(200, "text/plain", "Measurement already in progress");
  }
}

void handleRoot(AsyncWebServerRequest *request) {
  String html = R"HTML(
    <!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: 'Press Start 2P', cursive;
            text-align: center;
            margin: 50px;
            background-color: #000;
            color: #fff;
        }

        #countdown {
            font-size: 24px;
            margin-bottom: 20px;
        }

        #progress-bar-container {
            width: 300px;
            height: 20px;
            background-color: #333;
            margin: 20px auto;
            overflow: hidden;
            border-radius: 5px;
        }

        #progress-bar {
            height: 100%;
            width: 0;
            background-color: #4CAF50;
            border-radius: 5px;
            transition: width 7s ease-in-out;
        }

        #result {
            font-size: 24px;
            margin-top: 20px;
        }

        #start-button {
            padding: 10px 20px;
            font-size: 18px;
            cursor: pointer;
            background-color: #d9534f; /* Rojo intenso */
            color: #fff;
            border: none;
            border-radius: 5px;
            outline: none;
            transition: background-color 0.3s ease;
        }

        #start-button:hover {
            background-color: #c9302c; /* Rojo más oscuro al pasar el mouse */
        }
    </style>
    <link href="https://fonts.googleapis.com/css2?family=Press+Start+2P&display=swap" rel="stylesheet">
</head>
<body>
    <div id="countdown">Haz clic en el botón para empezar la prueba de alcohol en aire aspirado</div>
    <button id="start-button" onclick="startCountdown()">Iniciar prueba</button>
    <div id="progress-bar-container">
        <div id="progress-bar"></div>
    </div>
    <div id="result"></div>

    <script>
        function startCountdown() {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    var countdown = 5;
                    var countdownElement = document.getElementById('countdown');
                    var progressBar = document.getElementById('progress-bar');
                    var resultElement = document.getElementById('result');
                    var startButton = document.getElementById('start-button');

                    startButton.disabled = true;

                    var countdownInterval = setInterval(function() {
                        countdownElement.textContent = 'Empieza a soplar en ' + countdown + ' segundos';

                        if (countdown <= 0) {
                            clearInterval(countdownInterval);
                            countdownElement.textContent = 'Sopla ahora!';
                            progressBar.style.width = '100%';

                            // Simular la barra de progreso que se llena durante 7 segundos
                            setTimeout(function() {
                                progressBar.style.width = '0';
                                startButton.disabled = false;
                                displayResult();
                            }, 7000);
                        }

                        countdown--;
                    }, 1000);
                }
            };
            xhttp.open('GET', '/startCountdown', true);
            xhttp.send();
        }

        function displayResult() {
            var xhttp = new XMLHttpRequest();
            xhttp.onreadystatechange = function() {
                if (this.readyState == 4 && this.status == 200) {
                    var resultElement = document.getElementById('result');
                    resultElement.textContent = 'Resultado: ' + this.responseText;
                    resultElement.style.color = '#d9534f'; /* Rojo intenso */
                }
            };
            xhttp.open('GET', '/getMeasurementResult', true);
            xhttp.send();
        }
    </script>
</body>
</html>

  )HTML";

  request->send(200, "text/html", html);
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/startCountdown", HTTP_GET, handleStartCountdown);
  server.on("/getMeasurementResult", HTTP_GET, handleGetMeasurementResult);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  connectToWiFi();
  startWebServer();
  MQ3.setRegressionMethod(1);
  MQ3.setA(0.3934); // valors per obtenir alcohol amb el sensor MQ3
  MQ3.setB(-1.504);
  initializeMQ();
}

void loop() {
  reconnectWiFi();
  
}
