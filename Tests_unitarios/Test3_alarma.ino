int pinAlarma = 8;

void setup() {
  pinMode (pinAlarma, OUTPUT);
}

void loop() {
  tone(pinAlarma, 1000);
  delay(1000);
  noTone(pinAlarma);
  delay(1000);
}
