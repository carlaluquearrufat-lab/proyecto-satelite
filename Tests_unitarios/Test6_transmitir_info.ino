//Test unitario
#include <SoftwareSerial.h>
int i;
SoftwareSerial mySerial(10,11); //RX,TX
void setup() {
  mySerial.begin(9600);
  i=1;

}

void loop() {
  delay(2000);
  mySerial.print("Envío: ");
  mySerial.println(i);
  i++;

}
