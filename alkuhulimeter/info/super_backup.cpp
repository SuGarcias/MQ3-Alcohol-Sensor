#include <MQUnifiedsensor.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

/************************WIFI************************************/
const char* ssid = "SuGarcias"; // Rellena con el nombre de tu red WiFi
const char* password = "sentilo1234"; // Rellena con la contraseña de tu red WiFi

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
#define Pin (32) // Cambiar a GPIO 32 para ESP32

/***********************Software Related Macros**************************** ********/
#define Type ("MQ-3") // MQ3
#define Voltage_Resolution (5)
#define ADC_Bit_Resolution (12) // Para ESP32 (12 bits)
#define RatioMQ3CleanAir (60) // RS / R0 = 60 ppm

/*****************************Globals***********************************************/
MQUnifiedsensor MQ3(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type);
const int bootButtonPin = 0;  // Pin GPIO conectado al botón boot
unsigned long startTime = 0;  // Variable para almacenar el tiempo de inicio de la medición
float sumValues = 0;          // Variable para sumar los valores durante 7 segundos
int sampleCount = 0;          // Contador de muestras durante 7 segundos
bool measuring = false;       // Bandera para indicar si estamos midiendo

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

// Nueva función para manejar la petición de obtener el resultado de la medición
void handleGetMeasurementResult(AsyncWebServerRequest *request) {
  if (measuring) {
    MQ3.update();
    float sensorValue = MQ3.readSensor();
    sumValues += sensorValue;
    sampleCount++;
  }

  float averageValue = sumValues / sampleCount;
  request->send(200, "text/plain", String(averageValue));
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

void handleStartCountdown(AsyncWebServerRequest *request) {
  // Aquí puedes iniciar la lógica relacionada con la cuenta regresiva
  // ...
  request->send(200, "text/plain", "Countdown started");
}

void handleDisplayResult(AsyncWebServerRequest *request) {
  // Aquí puedes obtener y enviar el resultado de la prueba
  // ...
  request->send(200, "text/plain", "0.08% - Nivel de alcohol en sangre elevado");
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
                font-family: 'Arial', sans-serif;
                text-align: center;
                margin: 50px;
                background-color: #222;
                color: #fff;
            }

            #countdown {
                font-size: 24px;
                margin-bottom: 20px;
            }

            #progress-bar-container {
                width: 300px;
                height: 20px;
                background-color: #555;
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
    </head>
    <body>
        <div id="countdown">Haz clic en el botón para empezar la prueba de alcohol en sangre</div>
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
                            countdownElement.textContent = 'Tiempo restante: ' + countdown + ' segundos';

                            if (countdown <= 0) {
                                clearInterval(countdownInterval);
                                countdownElement.textContent = 'Prueba completada';
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
                xhttp.open('GET', '/displayResult', true);
                xhttp.send();
            }
        </script>
    </body>
    </html>
  )HTML";

  request->send(200, "text/html", html);
}


// Nueva función para manejar la petición de inicio/detención de la medición
void handleStartStopMeasurement(AsyncWebServerRequest *request) {
  measuring = !measuring;

  if (measuring) {
    startTime = millis();
    sumValues = 0;
    sampleCount = 0;
  } else {
    MQ3.update();
    float averageValue = sumValues / sampleCount;
    sendPUTRequest(averageValue);
    client.stop();
  }

  request->send(200, "text/plain", String(measuring));
}

void startWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  // Agrega las nuevas rutas y funciones de manejo de peticiones
  server.on("/startStopMeasurement", HTTP_GET, handleStartStopMeasurement);
  server.on("/getMeasurementResult", HTTP_GET, handleGetMeasurementResult);
  server.begin();
}

void setup() {
  Serial.begin(115200);

  //WiFi
  WiFi.mode(WIFI_STA);
  connectToWiFi();

  // Iniciar el servidor web
  startWebServer();

  // Establecer el modelo matemático para calcular la concentración en PPM y los valores de las constantes
  MQ3.setRegressionMethod(1); // _PPM =  a*ratio^b
  MQ3.setA(0.3934);
  MQ3.setB(-1.504);

  // Inicialización de MQ
  initializeMQ();

  // Configurar el pin del botón "boot" como entrada
  pinMode(bootButtonPin, INPUT);

  Serial.println("Calibration completed.");

  server.on("/startCountdown", HTTP_GET, handleStartCountdown);
  server.on("/displayResult", HTTP_GET, handleDisplayResult);
  
}

void loop() {
  // Reconnect to WiFi if necessary
  reconnectWiFi();

  // Reconnect to the server if necessary
  reconnectServer();

  // Verificar si se presionó el botón "boot"
  static bool buttonState = HIGH;
  static bool lastButtonState = HIGH;
  static unsigned long lastDebounceTime = 0;
  static unsigned long debounceDelay = 50;

  int buttonReading = digitalRead(bootButtonPin);

  if (buttonReading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (buttonReading != buttonState) {
      buttonState = buttonReading;

      if (buttonState == LOW) {
        // Iniciar la medición si no estamos midiendo
        measuring = !measuring;

        if (measuring) {
          startTime = millis();  // Almacenar el tiempo de inicio
          sumValues = 0;         // Reiniciar la suma de valores
          sampleCount = 0;       // Reiniciar el contador de muestras
          Serial.println("BOTON PULSADO");
        } else {
          // Detener la medición si el botón ya no está presionado
          // Update MQ sensor data
          MQ3.update();
          // Calcular y enviar el valor promedio hasta ahora
          float averageValue = sumValues / sampleCount;
          sendPUTRequest(averageValue);
          Serial.println("Average: " + String(averageValue));
          client.stop();
        }
      }
    }
  }

  lastButtonState = buttonReading;

  // Si estamos midiendo, continuar la medición
  if (measuring) {
    // Update MQ sensor data
    MQ3.update();
    float sensorValue = MQ3.readSensor();

    // Sumar los valores durante 7 segundos
    sumValues += sensorValue;
    sampleCount++;

    // Si han pasado 7 segundos, calcular y enviar el valor promedio
    if (millis() - startTime >= 7000) {
      float averageValue = sumValues / sampleCount;
      sendPUTRequest(averageValue);
      Serial.println(averageValue);
      client.stop();
      // Reiniciar las variables para la próxima medición
      measuring = false;
    }
  }

  // Esperamos un poco antes de cerrar la conexión

  // Esperamos un poco antes de cerrar la conexión con el servidor
  delay(200);
}
