# GRUPO 14
![IMG_2547](https://github.com/user-attachments/assets/18c81932-ab38-4fac-8f1e-7fbd5b14daf9)
Somos el Grupo 14, formado por Arnau Prat Villarroya, Claudia Ros Núñez y Carla Luque Arrufat. 

# Versión 1:
**Descripción versión 1:**  

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


# Objetivos a cumplir para la versión 3: 
- El usuario puede elegir dónde deben calcularse las medias de las 10 últimas temperaturas (si en el satélite o en tierra).
- El usuario puede cambiar el periodo de envío de datos de temperatura/humedad y de distancia.
- El usuario puede parar/reanudar el envío de los datos de humedad/temperatura y el envio de datos de distancia
- El usuario puede establecer el valor máximo de temperatura que hará que salte una alarma si se reciben tres valores medios seguidos por encima de ese valor máximo.
