void setup() {
  Serial.begin(9600);  // Comunicación con satélite
  Serial.println("Estacion de tierra lista");
}

void loop() {
  // --- 1️⃣ Leer datos desde la estación (PC) ---
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // leer línea completa
    input.trim(); // limpiar espacios

    int angulo = input.toInt(); // convertir a número
    if (angulo >= 0 && angulo <= 180) {
      // --- 2️⃣ Enviar al satélite ---
      Serial.println(angulo); // enviar el ángulo al satélite
      Serial.print("Enviado al satelite: ");
      Serial.println(angulo);
    } else {
      Serial.println("Angulo invalido, ingrese 0-180");
    }
  }

  // --- 3️⃣ Recibir datos del satélite ---
  if (Serial.available() > 0) {
    String datos = Serial.readStringUntil('\n');
    datos.trim();
    if (datos.length() > 0) {
      Serial.print("Datos recibidos del satelite: ");
      Serial.println(datos);
    }
  }
}

