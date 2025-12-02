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



# Objetivos a cumplir para la versión 4: 
- 
