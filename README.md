# GRUPO 14
![IMG_2547](https://github.com/user-attachments/assets/18c81932-ab38-4fac-8f1e-7fbd5b14daf9)
Somos el Grupo 14, formado por Arnau Prat Villarroya, Claudia Ros Núñez y Carla Luque Arrufat. 

# Versión 1:
**Descripción versión 1:**  

La versión 1 del proyecto representa la fase inicial del sistema, centrada en la adquisición y monitorización de variables ambientales mediante el Arduino satélite (controlador) y la estación de tierra.

En esta versión, el controlador capta correctamente los datos de temperatura y humedad y los transmite a la estación de tierra, donde se visualizan en una gráfica dinámica. El usuario puede pausar o reanudar la gráfica dinámica, ofreciendo control directo sobre la monitorización.

El sistema incluye mecanismos de detención y aviso ante fallos, de modo que el controlador notifica a la estación de tierra si no puede capturar correctamente los datos (por ejemplo, si los sensores están desconectados), y la estación detecta cualquier fallo de comunicación, alertando al usuario de manera clara. La interfaz gráfica está diseñada para que el usuario no tenga dudas sobre su funcionamiento ni sobre la interpretación de los datos y las alarmas.

**Código relevante versión 1:**  

    plt.ion()
    plt.axis([0, 100, 20, 100])
    temperaturas = []
    eje_x = []
    i = 0
    humedades = []
    eje_x2 = []
    n = 0
    grafica = False
    grafica2 = False

    def EntrarClick ():
        print ('Has introducido la frase --- ' + fraseEntry.get() + ' --- y has pulsado el botón entrar')

    def HUMClick():
        global grafica2
        grafica2 = True
        iniciar_grafica2()  

    def iniciar_grafica2():
        global n
        global grafica2
        while grafica2 == True:
            if ser.in_waiting > 0:
                linea = ser.readline().decode('utf-8').strip()
                print(linea)

                trozos2 = linea.split('H:')    
            
                 # Evitar errores si la línea no tiene el formato
                if len(trozos2) < 2:
                    print("Línea ignorada (mal formato):", linea)
                    continue
                try:
                    humedad = float(trozos2[1])
                except ValueError:
                    print("Error al convertir a número:", trozos2[1])
                    continue

                # Usar eje_x2 y aumentar n (antes no se aumentaba)
                humedades.append(humedad)
                eje_x2.append(n)
                n += 1

                # Evitar plot con ejes de distinto tamaño
                if len(eje_x2) == len(humedades):
                    plt.figure(1)  # aseguramos figura (opcional)
                    plt.plot(eje_x2, humedades)
                    plt.draw()
                    plt.pause(0.5)
                else:
                    # por seguridad, dibujamos solo los datos disponibles
                    plt.figure(1)
                    plt.plot(eje_x2, humedades)
                    plt.draw()
                    plt.pause(0.5)

    def TEMPClick():
        global grafica
        grafica = True
        iniciar_grafica()  

    def STOPClick():
        global grafica2
        global grafica
        grafica = False
        grafica2 = False
        print("Gráficas detenida")

    def iniciar_grafica():
        global i
        while grafica == True:
            if ser.in_waiting > 0:
                linea = ser.readline().decode('utf-8').strip()
                print(linea)

                # Evitar errores si la línea no tiene el formato 
                trozos = linea.split('T:')    
            
                if len(trozos) < 2:
                    print("Línea ignorada (mal formato):", linea)
                    continue
                try:
                    temperatura = float(trozos[1])
                except ValueError:
                    print("Error al convertir a número:", trozos[1])
                    continue

                eje_x.append(i)
                temperaturas.append(temperatura)
                plt.plot(eje_x, temperaturas)
                plt.title(str(i))
                i += 1

                plt.draw()
                plt.pause(0.5)

**Video introductorio de la versión 1:**
https://youtu.be/pAPxgO0p6xA?si=Zm1WQsLXPUr7lVKI

# Versión 2:

**Descripción versión 2:**  

En la versión 2 del proyecto se ha incorporado un nuevo sensor de distancia montado sobre un motor, lo que permite su orientación controlada y dinámica. Para ello, se diseñó una estructura unificada que sostiene el sensor y el motor, garantizando que el motor pueda girar el sensor de manera precisa.

El sensor puede manejarse desde la interfaz gráfica en Python, de forma manual, mediante un control tipo joystick, o de forma automática, ofreciendo flexibilidad en su operación. Asimismo, se implementaron alarmas y notificaciones, que advierten al usuario en caso de fallos de los sensores o del sistema, mejorando la seguridad y confiabilidad.

Esta versión mantiene e integra todas las funcionalidades de la versión anterior, permitiendo que la interfaz gráfica en Python muestre gráficas dinámicas de temperatura, humedad, distancia y ángulo del motor, con la posibilidad de pausar o reanudar la visualización de manera interactiva. Adicionalmente, se ha implementado la opción de calcular la media de las últimas 10 temperaturas, ya sea en el satélite o en la estación de tierra, siguiendo los objetivos de mejora en análisis de datos.

El Arduino de tierra recibe la información del controlador y la transmite a Python, asegurando que los datos se presenten de forma fiable y en tiempo real. El código mantiene una estructura clara y comentada, con protocolos definidos y funciones documentadas, facilitando su comprensión y mantenimiento.

Finalmente, se ha añadido control total sobre el periodo de envío de datos y la posibilidad de detener o reanudar la captura de variables individuales (temperatura, humedad, distancia), cumpliendo con los objetivos de control y supervisión planteados para esta versión.

**Código relevante versión 2:**  

    data_lock = threading.Lock()

    def lector_serial():
    global ser
    if ser is None:
        return
    while True:
        try:
            if ser.in_waiting > 0:
                linea_raw = ser.readline()
                if not linea_raw:
                    continue
                try:
                    linea = linea_raw.decode('utf-8', errors='ignore').strip()
                except:
                    linea = linea_raw.decode('latin1', errors='ignore').strip()
                
                try:
                    if 'H:' in linea:
                        part = linea.split('H:')[-1]
                        val = part.split()[0]
                        hum = float(val)
                        with data_lock:
                            humedades.append(hum)
                    if 'T:' in linea:
                        part = linea.split('T:')[-1]
                        val = part.split()[0]
                        temp = float(val)
                        with data_lock:
                            temperaturas.append(temp)
                            eje_x.append(len(temperaturas)-1)
                    if 'Angulo:' in linea:
                        part = linea.split('Angulo:')[-1]
                        val = part.split()[0]
                        ang = float(val)
                        with data_lock:
                            angulos.append(ang)
                    if 'Dist' in linea and 'cm' in linea:
                        try:
                            part = linea.split('Dist (cm):')[-1]
                        except:
                            part = linea.split('Dist:')[-1]
                        val = part.split()[0]
                        dist = float(val)
                        with data_lock:
                            distancias.append(dist)
                except Exception:
                    pass
        except Exception:
            print("Error en lector_serial:")
            time.sleep(0.2)
        time.sleep(0.01)


    def RADARClick():
    global radar, sonar, fig2, ax2, radarEncendido 
    if radar is not None and radarEncendido :
        return  #ya abierto 
    
    radar = True  #radar ya inicializado
    fig2 = Figure(figsize=(6,4), dpi=100)
    ax2 = fig2.add_subplot(111, polar=True)
    ax2.set_theta_zero_location("E")  # 0° a la derecha
    ax2.set_theta_direction(-1)       # grados hacia arriba
    ax2.set_title("Radar de Ultrasonido")
  
    sonar = FigureCanvasTkAgg(fig2, master=radar_frame)
    sonar.get_tk_widget().pack(fill='both', expand=True)
    radarEncendido  = True
    threading.Thread(target=actualizar_radar, daemon=True).start()


    def actualizar_radar():
    global radarEncendido , ax2, sonar, radar
    while radarEncendido :
        
        try:
            if not window.winfo_exists():
                radarEncendido  = False
                break
        except:
            radarEncendido  = False
            break
        with data_lock:
            angs = list(angulos)
            dists = list(distancias)
        ax2.clear()
        ax2.set_theta_zero_location("E")
        ax2.set_theta_direction(-1)
        ax2.set_ylim(0, max(dists) * 1.1 if dists else 50)
        # Convertir grados a radianes
        if angs and dists and len(angs) == len(dists):
            thetas = [math.radians(a) for a in angs]
            ax2.plot(thetas, dists, marker='o', linestyle='-', linewidth=2)
            # marcar último punto
            ax2.plot([thetas[-1]], [dists[-1]], marker='o', markersize=8)
        elif angs and dists:
          
            mn = min(len(angs), len(dists))
            thetas = [math.radians(a) for a in angs[:mn]]
            ax2.plot(thetas, dists[:mn], marker='o', linestyle='-', linewidth=2)
        sonar.draw_idle()
        time.sleep(0.2)


**Video introductorio de la versión 2:**
https://www.youtube.com/watch?v=ad9l3uBzaGk 

# Versión 3: 
En la versión 3 del proyecto se ha incorporado la opción de girar manualmente el servomotor a través de la interfaz gráfica. También podemos parar y reanudar cualquier dato del satélite: humedad, temperatura, distancia o los tres al mismo tiempo. Además, si se producen más de cinco errores de lectura de temperatura y humedad, se activará una alarma sonora en el satélite.

Al haber completado los objetivos pendientes de la versión 2, hemos añadido algunas de las novedades de la versión 3: un simulador de órbitas —ya que no podemos enviar nuestra placa Arduino al espacio— en el que podemos ver en qué punto del planeta se encuentra “en tiempo real” a través de una gráfica en la interfaz. Asimismo, ahora la comunicación entre la placa de la estación de tierra y la placa Arduino del satélite es inalámbrica, gracias al kit LoRa.

**Código relevante de la versión 3:**

    canvas_orbita = None
    fig_orbita = None
    ax_orbita = None
    
    #Variables para la órbita
    R_EARTH = 6371000  # Radio de la Tierra en metros
    x_vals = []
    y_vals = []
    z_vals = []

    # Lectura de posición de órbita (Position: X:.. Y:.. Z:..)
        if 'Position:' in linea:
            try:
                parts = linea.replace('Position:','').replace('(','').replace(')','').split(',')
                x = float(parts[0].split(':')[1])
                y = float(parts[1].split(':')[1])
                z = float(parts[2].split(':')[1])
                x_vals.append(x)
                y_vals.append(y)
                z_vals.append(z)
                except: pass
                
    # ---------------- RADAR MANUAL ----------------              
    def radar_manual():
        win = Toplevel(window)
        win.title("Control Motor")
        Label(win, text="Mover Motor", font=("Arial",14)).pack(pady=10)
        slider = Scale(win, from_=-90, to=90, orient="horizontal", length=300, resolution=1, command=enviar_direccion)
        slider.set(0)
        slider.pack(pady=20)

    def enviar_direccion(val):
        val = float(val)
        angulo = int(90 + val)
        angulo = max(0, min(180, angulo))
        if ser is not None:
            ser.write(f"DIR:{angulo}\n".encode())
            print("Enviado:", angulo)

    # ---------------- ORBITA ----------------
    def init_orbita():
        global canvas_orbita, fig_orbita, ax_orbita, orbitaEncendida
        if orbitaEncendida:
            return
        orbitaEncendida = True
        fig_orbita = Figure(figsize=(6,6), dpi=100)
        ax_orbita = fig_orbita.add_subplot(111)
        ax_orbita.set_aspect('equal', 'box')
        ax_orbita.set_xlim(-7e6, 7e6)
        ax_orbita.set_ylim(-7e6, 7e6)
        ax_orbita.set_xlabel('X (m)')
        ax_orbita.set_ylabel('Y (m)')
        ax_orbita.set_title('Orbit View')
        canvas_orbita = FigureCanvasTkAgg(fig_orbita, master=orbit_frame)
        canvas_orbita.get_tk_widget().pack(fill='both', expand=True)
        threading.Thread(target=actualizar_orbita, daemon=True).start()

    def actualizar_orbita():
        global x_vals, y_vals
        while orbitaEncendida:
            with data_lock:
                xs = list(x_vals)
                ys = list(y_vals)
            if not xs or not ys:
                time.sleep(0.1)
                continue
            ax_orbita.clear()
            ax_orbita.set_aspect('equal', 'box')
            ax_orbita.set_xlim(-7e6,7e6)
            ax_orbita.set_ylim(-7e6,7e6)
            ax_orbita.plot(xs, ys, 'bo-', markersize=2, label='Orbit')
            canvas_orbita.draw_idle()
            time.sleep(0.25)

    def RADARMClick():
        radar_manual()
        init_radar()
        if ser: ser.write(b"RadarManual\n")

    def ORBITClick():
        init_orbita()

    Button(window,text="RADAR MANUAL", command=RADARMClick, bg='green',fg='white',**botones).grid(row=2,column=4,sticky=N+S+E+W)
    Button(window,text="ORBITA", command=ORBITClick, bg='orange',fg='white',**botones).grid(row=2,column=5,sticky=N+S+E+W)

    orbit_frame = Frame(window, bd=2, relief='groove')
    orbit_frame.grid(row=4,column=5,columnspan=1,sticky=N+S+E+W,padx=5,pady=5)

    
**Video introductorio de la versión 3:**
https://www.youtube.com/watch?v=icbPOi3H1ic

# Versión 4: Versión final
En la última versión de nuestro proyecto se mantienen todas las funcionalidades desarrolladas en versiones anteriores y se incorporan algunos detalles adicionales para hacerlo más completo e interesante.

El sistema está compuesto por tres partes principales: el satélite, la estación de tierra y la interfaz gráfica. El satélite se encarga de recoger los datos del entorno, la estación de tierra recibe esta información y la retransmite, y la interfaz gráfica permite al usuario visualizar los datos y controlar el sistema de forma sencilla.

El satélite es capaz de enviar periódicamente datos de temperatura, humedad y distancia. Para ello cuenta con un sensor de temperatura y humedad, y con un sensor de ultrasonidos montado sobre un servomotor que realiza un barrido de 180 grados. Este sistema permite medir la distancia a los objetos que rodean al satélite en diferentes direcciones.

Los datos de distancia obtenidos se envían a la estación de tierra mediante comunicación LoRa y posteriormente a la interfaz gráfica, donde se representan en una gráfica con forma de radar, facilitando la visualización del entorno del satélite.

Por otro lado, el satélite incorpora un simulador de órbita programado en Arduino, que genera datos de posición a lo largo del tiempo. Estas coordenadas se utilizan en la interfaz gráfica para representar la órbita del satélite mediante una gráfica tridimensional, permitiendo observar su movimiento alrededor de la Tierra. Además, los datos de temperatura y humedad se muestran en la interfaz mediante una gráfica temporal, donde la temperatura aparece en color azul y la humedad en color rojo. De este modo, el usuario puede supervisar fácilmente las condiciones del entorno.

Por último, esta versión del proyecto integra sensores, comunicación inalámbrica, simulación orbital y visualización gráfica en tiempo real, consiguiendo un sistema más completo, interactivo y cercano al funcionamiento de un satélite real.

**Código Relevante Versión 4**

    //INTERFAZ VERSION 4 – RESUMEN DEL CÓDIGO MÁS RELEVANTE

    //COMUNICACIÓN SERIE CON CONFIRMACIÓN (ACK)
    //Esta función envía comandos críticos al Arduino y espera
    //una confirmación "OK" para asegurar que el mensaje llegó.

    def enviar_con_ack(comando, intentos=5, espera_ack=0.8):
        for i in range(intentos):
            ser.write((comando + "\n").encode())
            t0 = time.time()
            while time.time() - t0 < espera_ack:
                if ser.in_waiting > 0:
                    respuesta = ser.readline().decode().strip()
                    if respuesta == "OK":
                        return True
            time.sleep(espera_ack)
        return False

    //SISTEMA DE REGISTRO DE EVENTOS (LOG)
    //Todo lo que ocurre (comandos, alarmas, acciones del
    //usuario) se guarda en un archivo eventos.log

    LOG_FILE = "eventos.log"

    def registrar_evento(codigo, tipo, mensaje):
        fecha = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        with open(LOG_FILE, "a") as f:
            f.write(f"{fecha}\t{codigo}\t{tipo}\t{mensaje}\n")

    //TABLA DE EVENTOS POR CÓDIGO
    //Permite registrar acciones usando solo un número,
    //manteniendo coherencia en todo el sistema

    eventos = {
        201: ("COMANDO", "Parar temperatura"),
        204: ("COMANDO", "Reanudar temperatura"),
        301: ("ALARMA", "Error lectura temperatura"),
        401: ("USUARIO", "Nota del usuario"),
        406: ("USUARIO", "Solicitud de media al Arduino")
    }

    def registrar_evento_por_codigo(codigo):
        if codigo in eventos:
            tipo, mensaje = eventos[codigo]
            registrar_evento(codigo, tipo, mensaje)

    # MULTI-IDIOMA DE LA INTERFAZ
    #  Todos los textos de botones y etiquetas dependen
    # del idioma seleccionado dinámicamente

    traducciones = {
        "es": {"temp": "TEMP", "hum": "HUM", "orbit": "ÓRBITA"},
        "en": {"temp": "TEMP", "hum": "HUM", "orbit": "ORBIT"},
        "zh": {"temp": "温度", "hum": "湿度", "orbit": "轨道"}
    }

    idioma = "es"

    # CONEXIÓN SERIAL
    # Apertura del puerto COM para comunicación con Arduino

    device = "COM7"
    ser = serial.Serial(device, 9600, timeout=1)

    # VARIABLES GLOBALES DE DATOS
    # Listas compartidas entre hilos para almacenar sensores

    temperaturas = []
    humedades = []
    distancias = []
    angulos = []

    x_vals, y_vals, z_vals = [], [], []   # posición orbital
    medias_arduino = []

    data_lock = threading.Lock()

    # LECTOR SERIAL (NÚCLEO DEL SISTEMA)
    # Lee continuamente los datos enviados por LoRa,
    # interpreta el protocolo y guarda los valores

    def lector_serial():
        global media_arduino
        while True:
            linea = ser.readline().decode(errors="ignore").strip()
            if not linea:
                continue

            with data_lock:
                if "1:" in linea:
                    temperaturas.append(float(linea.split("1:")[1]))
                if "2:" in linea:
                    humedades.append(float(linea.split("2:")[1]))
                if "3:" in linea:
                    distancias.append(float(linea.split("3:")[1]))
                if linea.startswith("POS"):
                    vals = re.findall(r"[-+]?\d*\.\d+|[-+]?\d+", linea)
                    x_vals.append(float(vals[0]))
                    y_vals.append(float(vals[1]))
                    z_vals.append(float(vals[2]))

            time.sleep(0.01)

    # GRÁFICA DE TEMPERATURA + MEDIA
    # Muestra temperatura en tiempo real y una media móvil

    def plot_temp():
        fig = Figure(figsize=(8,4))
        ax = fig.add_subplot(111)

        while True:
            with data_lock:
                ys = temperaturas[-50:]
            if ys:
                ax.clear()
                ax.plot(ys, label="Temperatura")
                if len(ys) >= 10:
                    media = sum(ys[-10:]) / 10
                    ax.plot([media]*len(ys), label="Media")
                ax.legend()
            time.sleep(0.2)

    # RADAR ULTRASÓNICO (POLAR)
    # Representa ángulo vs distancia en coordenadas polares

    def actualizar_radar(ax):
        while True:
            with data_lock:
                ang = angulos[-5:]
                dist = distancias[-5:]
            if ang and dist:
                ax.clear()
                ax.plot([math.radians(a) for a in ang], dist, marker="o")
            time.sleep(0.2)

    # CONTROL MANUAL DEL SERVO
    # Permite mover el radar manualmente desde la GUI

    def enviar_direccion(val):
        angulo = int(90 + float(val))
        ser.write(f"RM:{angulo}\n".encode())

    # ÓRBITA 3D DEL SATÉLITE
    # Visualización 3D de la trayectoria alrededor de la Tierra

    def actualizar_orbita(ax):
        while True:
            with data_lock:
                xs = x_vals[-50:]
                ys = y_vals[-50:]
                zs = z_vals[-50:]
            if xs:
                ax.clear()
                ax.plot(xs, ys, zs)
            time.sleep(0.2)

    # SOLICITUD DE MEDIA AL ARDUINO
    # Pide al satélite que calcule la media internamente

    def MEDIAClick():
        ser.write(b"M\n")
        registrar_evento_por_codigo(406)

    # INICIO DE HILOS
    # El lector serie SIEMPRE corre en segundo plano

    threading.Thread(target=lector_serial, daemon=True).start()

    # INTERFAZ GRÁFICA (Tkinter)
    # Botones, frames y control de toda la aplicación

    window = Tk()
    window.title("INTERFAZ VERSION 4")
    window.geometry("1800x800")

    Button(window, text="TEMP", command=plot_temp).pack()
    Button(window, text="MEDIA", command=MEDIAClick).pack()

    window.mainloop()


    // SATÉLITE – VERSIÓN 4 (LoRa)
    // RESUMEN DEL CÓDIGO MÁS RELEVANTE 

    // VARIABLES DE CONTROL DE TIEMPO

    unsigned long tiempoServo = 0;
    unsigned long tiempoDist = 0;
    unsigned long tiempoTemp = 0;
    unsigned long tiempoHum  = 0;
    unsigned long tiempoEnvio = 0;

    // MODO DE FUNCIONAMIENTO DEL RADAR
    
    // AUTO   → barrido automático
    // MANUAL → control desde la interfaz
    // STOP   → radar detenido

    enum RadarModo { RADAR_AUTO, RADAR_MANUAL, RADAR_STOP };
    RadarModo radarModo = RADAR_AUTO;

    // BUFFER PARA CÁLCULO DE MEDIA DE TEMPERATURA
    // Guarda las últimas N lecturas para calcular la media

    const int MAX_LECTURAS = 50;
    float bufferTemperaturas[MAX_LECTURAS];
    int numLecturasTemp = 0;
    float mediaTemperatura = 0;

    // VARIABLES DE ESTADO DEL SISTEMA

    int anguloActual = 90;
    int direccion = 1;

    float DISTANCIA = 0;
    float TEMPERATURA = 0;
    float HUMEDAD = 0;

    bool ISNANT = false;
    bool ISNANH = false;

    // FLAGS PARA ACTIVAR/DESACTIVAR SENSORES

    bool leertemperatura = true;
    bool leerhumedad = true;
    bool leerdistancia = true;

    // OBJETOS PRINCIPALES

    Servo servo;
    DHT dht(DHTPIN, DHTTYPE);
    SoftwareSerial LoRaSerial(10, 11); // RX, TX

    // FUNCIÓN: ESCRIBIR SERVO 

    // - En modo automático solo escribe si cambia el ángulo
    // - En modo manual fuerza siempre el movimiento
    // - En STOP no se mueve

    void escribirServo(int ang, bool fuerza=false) {
        static int ultimoAnguloEscrito = 90;

        if (radarModo == RADAR_MANUAL && fuerza) {
            servo.write(ang);
            ultimoAnguloEscrito = ang;
        }
        else if (radarModo == RADAR_AUTO) {
            if (ang != ultimoAnguloEscrito) {
                servo.write(ang);
                ultimoAnguloEscrito = ang;
            }
        }
    }

    // LOOP PRINCIPAL
    // Ejecuta todas las tareas sin bloquear el sistema

    void loop() {
        unsigned long ahora = millis();

        // RECEPCIÓN DE COMANDOS LoRa
        if (LoRaSerial.available()) {
            static String cmd = "";
            char c = LoRaSerial.read();
            if (c == '\n') {
                procesarComando(cmd);
                cmd = "";
            } else cmd += c;
        }

        // LECTURA TEMPERATURA
        if (leertemperatura && ahora - tiempoTemp >= INTERVALO_TEMP) {
            tiempoTemp = ahora;
            float t = dht.readTemperature();
            if (isnan(t)) ISNANT = true;
            else {
                ISNANT = false;
                TEMPERATURA = t;
                if (numLecturasTemp < MAX_LECTURAS)
                    bufferTemperaturas[numLecturasTemp++] = t;
            }
        }
        // LECTURA HUMEDAD
        if (leerhumedad && ahora - tiempoHum >= INTERVALO_HUM) {
            tiempoHum = ahora;
            float h = dht.readHumidity();
            if (isnan(h)) ISNANH = true;
            else { HUMEDAD = h; ISNANH = false; }
        }
        // MEDICIÓN DISTANCIA
        if (leerdistancia && ahora - tiempoDist >= INTERVALO_DIST) {
            tiempoDist = ahora;
            DISTANCIA = medirDistancia();
            if (DISTANCIA < 0) DISTANCIA = 0;
        }
        // ENVÍO DE DATOS POR LoRa
        if (ahora - tiempoEnvio >= INTERVALO_ENVIO) {
            tiempoEnvio = ahora;
            mediaTemperatura = calcularMediaTemperatura();

            String mensaje = "#:";
            if (leertemperatura) mensaje += " 1:" + String(TEMPERATURA,1);
            if (leerhumedad)     mensaje += " 2:" + String(HUMEDAD,1);
            if (leerdistancia)   mensaje += " 3:" + String(DISTANCIA,1);
            mensaje += " 5:" + String(mediaTemperatura,2);
            mensaje += " 4:" + String(anguloActual);

            LoRaSerial.println(mensaje);

            digitalWrite(ledExito, HIGH);
            delay(40);
            digitalWrite(ledExito, LOW);
        }

        actualizarServo();
    }

    // FUNCIONES AUXILIARES

    void actualizarServo() {
        if (radarModo != RADAR_AUTO) return;
        unsigned long ahora = millis();
        if (ahora - tiempoServo < INTERVALO_SERVO) return;

        tiempoServo = ahora;
        anguloActual += direccion;
        if (anguloActual >= 180 || anguloActual <= 0) direccion *= -1;
        escribirServo(anguloActual);
    }

    float medirDistancia() {
        digitalWrite(TRIG, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG, LOW);
        unsigned long d = pulseIn(ECO, HIGH, 25000UL);
        if (d == 0) return -1;
        return d / 58.2;
    }

    float calcularMediaTemperatura() {
        if (numLecturasTemp == 0) return TEMPERATURA;
        float suma = 0;
        for (int i = 0; i < numLecturasTemp; i++) suma += bufferTemperaturas[i];
        return suma / numLecturasTemp;
    }

    // PROCESAMIENTO DE COMANDOS
    // Permite control total desde la interfaz Tierra

    void procesarComando(String cmd) {
        cmd.trim();

        if (cmd == "S1") leertemperatura = false;
        if (cmd == "S2") leerhumedad = false;
        if (cmd == "S3") leerdistancia = false;

        if (cmd == "R1") leertemperatura = true;
        if (cmd == "R2") leerhumedad = true;
        if (cmd == "R3") leerdistancia = true;

        if (cmd == "RS") radarModo = RADAR_STOP;
        if (cmd == "RA" || cmd == "RR") radarModo = RADAR_AUTO;

        if (cmd.startsWith("RM:")) {
            radarModo = RADAR_MANUAL;
            anguloActual = constrain(cmd.substring(3).toInt(), 0, 180);
            escribirServo(anguloActual, true);
        }
    }



# Complementos añadidos en la versión 4: 
- Pantalla LCD 1602 en la estación de tierra que nos escribe algunos mensajes como "Conexión establecida".
- La órbita mostrada en la interfaz en 3D y un GroundTrack
- Poder cambiar el idioma de la interfaz. Tenemos la opción de tenerlo en castellano, inglés o en chino.
- Cajas impreses en 3d para la estación de tierra y para el satélite.


**Enlaces de los diseños de las cajas en 3D:**

  https://www.tinkercad.com/things/03zUn7bJhpA-satelite?sharecode=7c37xDYE8clnHKsiDSPyY3ky6TD5HbNhtTHXYAx4EUI
  https://www.tinkercad.com/things/28N9JQKFnjQ-tierra?sharecode=iBcDcr7fODBJKSEA2U5JFaFap1RKmUYZWhpY3S-DtNg
  
