#include <LoRa.h>

const int pinAlarma = 8;
const int pinLed = 5;

unsigned long marcaAlarma = 0;
unsigned long marcaLed = 0;
bool alarmaActiva = false;
bool ledActivo = false;

String linea = "";

void setup() {
  Serial.begin(9600);
  pinMode(pinLed, OUTPUT);
  pinMode(pinAlarma, OUTPUT);

  Serial.println("Sistema Tierra listo (LoRa)");

  // -------- INICIAR LORA --------
  if (!LoRa.begin(433E6)) {
    Serial.println("Error iniciando LoRa");
    while (1);
  }
  Serial.println("LoRa iniciado");
}

void loop() {

  // -------------------------
  // LECTURA DE PAQUETES LORA
  // -------------------------
  int packetSize = LoRa.parsePacket();
  if (packetSize) {

    while (LoRa.available()) {
      char c = LoRa.read();

      if (c == '\n' || c == '\r') {
        linea.trim();

        if (linea.length() > 0) {
          Serial.println(linea);

          if (linea.indexOf("No Echo") >= 0)
            activarAlarma(1000);

          else if (linea.indexOf("Error al leer") >= 0)
            activarAlarma(2000);

          else  
            activarLed();
        }

        linea = "";
      } 
      else {
        linea += c;
      }
    }
  }

  // -------------------------
  // ENVÍO DE COMANDOS A SATÉLITE
  // -------------------------
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');

    LoRa.beginPacket();
    LoRa.println(cmd);
    LoRa.endPacket();
  }

  // -------------------------
  // EFECTOS NO BLOQUEANTES
  // -------------------------
  actualizarAlarma();
  actualizarLed();
}

// -------- FUNCIONES --------
void activarAlarma(float freq) { 
  tone(pinAlarma, freq); 
  alarmaActiva = true; 
  marcaAlarma = millis(); 
}

void actualizarAlarma() { 
  if (alarmaActiva && millis() - marcaAlarma >= 300) { 
    noTone(pinAlarma); 
    alarmaActiva = false; 
  }
}

void activarLed() { 
  digitalWrite(pinLed, HIGH); 
  ledActivo = true; 
  marcaLed = millis(); 
}

void actualizarLed() { 
  if (ledActivo && millis() - marcaLed >= 150) { 
    digitalWrite(pinLed, LOW); 
    ledActivo = false; 
  }
}
