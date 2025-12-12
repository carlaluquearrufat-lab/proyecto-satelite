// ----- MEDIA DE TEMPERATURA -----
bool modoMedia = false;
float bufferTemp[10];
int idxTemp = 0;
bool bufferLleno = false; esto lo cambiamos por esto: 
if (leertemperatura && ahora - tiempoTemp >= INTERVALO_TEMP) {
    tiempoTemp = ahora;
    float t = dht.readTemperature();
    if (isnan(t)) {
        ISNANT = true;
        LoRaSerial.println("1!");
    } else {
        TEMPERATURA = t;
        ISNANT = false;

        // ----- Guardar en buffer para media (si está activado) -----
        if (modoMedia) {
            bufferTemp[idxTemp] = TEMPERATURA;
            idxTemp++;

            if (idxTemp >= 10) {
                idxTemp = 0;
                bufferLleno = true;
            }
        }
    }
}

if (ahora - tiempoEnvio >= INTERVALO_ENVIO) {
    tiempoEnvio = ahora;

    String radar = "DATA " + String(anguloActual) + "," + String(DISTANCIA,1);
    LoRaSerial.println(radar);

    String mensaje = "#:" + String(numeroEnvio++) +
                     " 1:" + String(TEMPERATURA,1) +
                     " 2:" + String(HUMEDAD,1) +
                     " 3:" + String(DISTANCIA,1) +
                     " 4:" + String(anguloActual);
    LoRaSerial.println(mensaje);

    parpadeoLed(ledExito, tiempoLedExito, ahora);
} justo debajo añadir:
// ----- Enviar la media si está activada -----
if (modoMedia && bufferLleno) {
    float suma = 0;
    for (int i = 0; i < 10; i++) suma += bufferTemp[i];

    float media = suma / 10.0;

    LoRaSerial.print("MEDIA:");
    LoRaSerial.println(media, 1);
} 

Al final de tu función procesarComando, sin modificar nada, añade esto:

// ====== COMANDOS NUEVOS PARA MEDIA ======

// Activar lectura normal
if (cmd == "IniciarT") {
    leertemperatura = true;
    modoMedia = false;
}

// Detener lectura
if (cmd == "PararT") {
    leertemperatura = false;
    modoMedia = false;
}

// Activar modo media desde la interfaz
if (cmd == "IniciarT_ARDUINO") {
    leertemperatura = true;
    modoMedia = true;

    // Resetear buffer
    idxTemp = 0;
    bufferLleno = false;
}
