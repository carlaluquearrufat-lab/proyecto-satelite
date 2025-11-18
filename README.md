# GRUPO 14
![IMG_2547](https://github.com/user-attachments/assets/18c81932-ab38-4fac-8f1e-7fbd5b14daf9)
Somos el Grupo 14, formado por Arnau Prat Villarroya, Claudia Ros Núñez y Carla Luque Arrufat. 

# Versión 1:
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

# Video introductorio de la versión 2:
https://www.youtube.com/watch?v=ad9l3uBzaGk 

# Objetivos a cumplir para la versión 3: 
- El usuario puede elegir dónde deben calcularse las medias de las 10 últimas temperaturas (si en el satélite o en tierra).
- El usuario puede cambiar el periodo de envío de datos de temperatura/humedad y de distancia.
- El usuario puede parar/reanudar el envío de los datos de humedad/temperatura y el envio de datos de distancia
- El usuario puede establecer el valor máximo de temperatura que hará que salte una alarma si se reciben tres valores medios seguidos por encima de ese valor máximo.
