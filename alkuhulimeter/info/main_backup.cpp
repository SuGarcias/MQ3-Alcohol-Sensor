#include <MQUnifiedsensor.h>
#include <WiFi.h>

/************************WIFI************************************/
const char* ssid = "iPhone de Gerard"; // Rellena con el nombre de tu red WiFi
const char* password = "gere7777"; // Rellena con la contraseña de tu red WiFi

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

/***********************Software Related Macros************************************/
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

void reconnectWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Reconnecting...");
    connectToWiFi();
  }
}

void reconnectServer() {
  if (!client.connected()) {
    Serial.println("Connecting to the server...");
    while (!client.connect(host, httpPort)) {
      delay(500);
    }
    Serial.println("Connected to the server");
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


void setup() {
  Serial.begin(115200);

  //WiFi
  WiFi.mode(WIFI_STA); 
  connectToWiFi();

  // Establecer el modelo matemático para calcular la concentración en PPM y los valores de las constantes
  MQ3.setRegressionMethod(1); // _PPM =  a*ratio^b
  MQ3.setA(0.3934);
  MQ3.setB(-1.504);
  /*
    Exponential regression:
  Gas    | a      | b     
  LPG    | 44771  | -3.245
  CH4    | 2*10^31| 19.01
  CO     | 521853 | -3.821
  Alcohol| 0.3934 | -1.504
  Benzene| 4.8387 | -2.68
  Hexane | 7585.3 | -2.849
  */

  // Inicialización de MQ
  initializeMQ();

  // Configurar el pin del botón "boot" como entrada
  pinMode(bootButtonPin, INPUT);

  Serial.println("Calibration completed.");
}

void loop() {
  // Reconnect to WiFi if necessary
  reconnectWiFi();

  // Reconnect to the server if necessary
  reconnectServer();

  // Verificar si se presionó el botón "boot"
  if (digitalRead(bootButtonPin) == LOW) {
    if (!measuring) {
      // Iniciar la medición si no estamos midiendo
      measuring = true;
      startTime = millis();  // Almacenar el tiempo de inicio
      sumValues = 0;         // Reiniciar la suma de valores
      sampleCount = 0;       // Reiniciar el contador de muestras
      Serial.println("BOTON PULSADO");
    }
  } else {
    // Detener la medición si el botón no está presionado
    measuring = false;
  }

  // Update MQ sensor data
  MQ3.update();
  //MQ3.serialDebug(); // Imprimirá la tabla en el puerto serial

  float sensorValue = MQ3.readSensor();


   if (measuring) {
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

  // Esperamos un poco antes de cerrar la conexión con el servidor
  delay(200);

  // Cerramos la conexión
  
}

