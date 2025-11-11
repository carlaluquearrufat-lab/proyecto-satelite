#include <Servo.h>

Servo servo;

void setup() {
  servo.attach(9);    
}

void loop() {
  // Mover de 0° a 180°
  for (int angulo = 0; angulo <= 180; angulo += 10) {
    servo.write(angulo);
    delay(300);             
  }

  // Regresar de 180° a 0°
  for (int angulo = 180; angulo >= 0; angulo -= 10) {
    servo.write(angulo);
    delay(300);
  }
}
