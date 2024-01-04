#include <MQUnifiedsensor.h>
#include <WiFi.h>

/************************WIFI************************************/
const char* ssid = "SuGarcias"; // Rellena con el nombre de tu red WiFi
const char* password = "sentilo1234"; // Rellena con la contraseña de tu red WiFi

/************************SENTILO************************************/
const char* host = "147.83.74.98";
const char* token = "5829244fb43b7f0218be9077d47cb1050be18eb03bf8c8def57c6e67dd367925";
const char* provider = "grup_3-102@Provider/";
const char* sensor = "MQ3_01/";

WiFiClient client;
const int httpPort = 8081;

void reconnect_wifi() 
{
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

void reconnect_client() 
{
  if (!client.connected()) {
    Serial.println("Connecting to the server...");
    while (!client.connect(host, httpPort)) {
      delay(500);
    }
  }
}

String make_put_request(float value) //passar el nivell ppm
{
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
void send_PUT_request(float value)
{
  String request = make_put_request(value);
  client.print(request);
}

/************************Hardware Related Macros************************************/
#define         Board                   ("ESP32")
#define         Pin                     (12) // Cambiar a GPIO 12 para ESP32
/***********************Software Related Macros************************************/
#define         Type                    ("MQ-3") // MQ3
#define         Voltage_Resolution      (5)
#define         ADC_Bit_Resolution      (12) // Para ESP32 (12 bits)
#define         RatioMQ3CleanAir        (60) // RS / R0 = 60 ppm

/*****************************Globals***********************************************/
// Declare Sensor
MQUnifiedsensor MQ3(Board, Voltage_Resolution, ADC_Bit_Resolution, Pin, Type);

void setup() {
  // Iniciar la comunicación por puerto serial para depuración
  Serial.begin(115200);

  //WiFi
  WiFi.mode(WIFI_STA); 
  reconnect_wifi();

  // Establecer el modelo matemático para calcular la concentración en PPM y los valores de las constantes
  MQ3.setRegressionMethod(1); // _PPM =  a*ratio^b
  MQ3.setA(4.8387);
  MQ3.setB(-2.68); 

  /*****************************  Inicialización de MQ ********************************************/
  MQ3.init();
  
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for (int i = 1; i <= 100; i++) {
    MQ3.update(); // Actualiza los datos, el ESP32 leerá el voltaje del pin analógico
    calcR0 += MQ3.calibrate(RatioMQ3CleanAir);
    Serial.print(".");
  }
  MQ3.setR0(calcR0 / 100);
  Serial.println("  done!.");

  if (isinf(calcR0)) { Serial.println("Warning: Problema de conexión, R0 es infinito (se detecta circuito abierto), verifica tu cableado y suministro."); while (1); }
  if (calcR0 == 0) { Serial.println("Warning: Problema de conexión encontrada, R0 es cero (el pin analógico se cortocircuita a tierra), verifica tu cableado y suministro."); while (1); }
  /*****************************  Calibración de MQ ********************************************/
  MQ3.serialDebug(true);
}

void loop() {
  reconnect_wifi(); // Reconnect to WiFi
  reconnect_client();

  MQ3.update(); // Actualiza los datos, el ESP32 leerá el voltaje del pin analógico
  MQ3.serialDebug(); // Imprimirá la tabla en el puerto serial
  send_PUT_request(MQ3.readSensor()); // El sensor leerá la concentración en PPM usando el modelo, los valores a y b configurados previamente o desde la configuración

  // si ha respondido esperamos un poco para cerrar la conexion con el servidor
  unsigned long timeout = millis();
  while (millis() - timeout < 200){}
  // Cerramos la conexion
  client.stop();

  delay(500); // Frecuencia de muestreo
}
