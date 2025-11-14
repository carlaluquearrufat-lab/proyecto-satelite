# Código relevante: 
                
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

# Video introductorio de la versión 1:
🎥 [Ver video en YouTube](https://www.youtube.com/watch?v=pAPxgO0p6xA)

