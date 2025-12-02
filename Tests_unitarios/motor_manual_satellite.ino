#include <Servo.h>
#include <SoftwareSerial.h>

// Comunicación con Arduino tierra
SoftwareSerial mySerial(10, 11);  // RX, TX

String mensaje = "";
Servo servoMotor;

void setup() {
  mySerial.begin(9600);
  servoMotor.attach(13);  // Pin del servo

  servoMotor.write(90);  // Posición neutra inicial
}

void loop() {

  while (mySerial.available()) {
    char c = mySerial.read();

    if (c == '\n') {
      procesarComando(mensaje);
      mensaje = "";
    } 
    else {
      mensaje += c;
    }
  }
}

void procesarComando(String cmd) {

  if (cmd.startsWith("DIR:")) {

    int dir = cmd.substring(4).toInt();

    if (dir == 0) {
      // Izquierda
      servoMotor.write(0);
    } 
    else if (dir == 1) {
      // Derecha
      servoMotor.write(180);
    } 
    else if (dir == 2) {
      // Neutro
      servoMotor.write(90);
    }
  }
}
