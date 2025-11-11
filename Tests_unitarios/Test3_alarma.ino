const int pinAlarma = 8;

void setup() {
  pinMode (pinAlarma, OUTPUT);
}

void loop() {
  digitalWrite(pinAlarma, HIGH);
  delay(2000);
  digitalWrite(pinAlarma, LOW);
  delay(2000);
}
