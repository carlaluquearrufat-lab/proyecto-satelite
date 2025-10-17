int x=5;
void setup() {
  pinMode (x, OUTPUT);

}

void loop() {
  digitalWrite(x, HIGH);
  delay(1000);
  digitalWrite(x, LOW);
  delay(1000);

}
