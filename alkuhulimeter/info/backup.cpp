#include <MQUnifiedsensor.h>

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

  // Establecer el modelo matemático para calcular la concentración en PPM y los valores de las constantes
  MQ3.setRegressionMethod(1); // _PPM =  a*ratio^b
  MQ3.setA(4.8387);
  MQ3.setB(-2.68); // Configurar la ecuación para calcular la concentración de benceno
  /*
    Regresión exponencial:
    Gas     | a      | b
    LPG     | 44771  | -3.245
    CH4     | 2*10^31| 19.01
    CO      | 521853 | -3.821
    Alcohol | 0.3934 | -1.504
    Benzene | 4.8387 | -2.68
    Hexane  | 7585.3 | -2.849
  */

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
  MQ3.update(); // Actualiza los datos, el ESP32 leerá el voltaje del pin analógico
  MQ3.readSensor(); // El sensor leerá la concentración en PPM usando el modelo, los valores a y b configurados previamente o desde la configuración
  MQ3.serialDebug(); // Imprimirá la tabla en el puerto serial
  delay(500); // Frecuencia de muestreo
}
