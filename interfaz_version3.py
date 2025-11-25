from tkinter import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial, threading, time, re, math

device = 'COM5'
try:
    ser = serial.Serial(device, 9600, timeout=1)
    print("Serial abierto en", device)
except Exception:
    print("No se pudo abrir serial:")
    ser = None

temperaturas = []
humedades = []
distancias = []
angulos = []

eje_x = []
eje_x2 = []
i = 0
n = 0

grafica = False
grafica2 = False
grafica3 = False
radarmanual= False

canvas = None
fig = None
ax = None


radar = None       
sonar = None
fig2 = None
ax2 = None
radarEncendido = False

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


def init_grafica_if_needed():
    global canvas, fig, ax
    if canvas is None:
        fig = Figure(figsize=(8,4), dpi=100)
        ax = fig.add_subplot(111)
        ax.set_facecolor('#FFFFFF')
        canvas = FigureCanvasTkAgg(fig, master=plot_frame)
        canvas.get_tk_widget().pack(fill='both', expand=True)


def TEMPClick():
    global grafica
    grafica = True
    init_grafica_if_needed()
    threading.Thread(target=iniciar_grafica, daemon=True).start()
    mensaje = "IniciarT"
    mySerial.write(mensaje.encode('utf-8'))

def HUMClick():
    global grafica2
    grafica2 = True
    init_grafica_if_needed()
    threading.Thread(target=iniciar_grafica2, daemon=True).start()
    mensaje = "IniciarH"
    mySerial.write(mensaje.encode('utf-8'))

def STOPTClick():
    global grafica
    mensaje = "PararT"
    mySerial.write(mensaje.encode('utf-8'))
    grafica= False

def STOPHClick():
    global grafica2
    mensaje = "PararH"
    mySerial.write(mensaje.encode('utf-8'))
    grafica2= False

def STOPDClick():
    global grafica3
    mensaje = "PararD"
    mySerial.write(mensaje.encode('utf-8'))
    grafica3= False

def STOPClick():
    global grafica, grafica2, grafica3
    mensaje = "STOP"
    mySerial.write(mensaje.encode('utf-8'))
    grafica = False
    grafica2 = False
    grafica3 = False

def REANUDARClick():
    global grafica, grafica2, grafica3
    mensaje = "REANUDAR"
    mySerial.write(mensaje.encode('utf-8'))
    grafica = True
    grafica2 = True
    grafica3 = True

def iniciar_grafica():
    global i
    while grafica:
        with data_lock:
            xs = list(eje_x)
            ys = list(temperaturas)
        if not ys:
            time.sleep(0.1)
            continue
        ax.clear()
        ax.plot(xs, ys, color='blue', label='TEMPERATURA')
        if len(ys) >= 10:
            media_linea = [sum(ys[-10:])/10.0]*len(ys)
            ax.plot(xs, media_linea, color='green', label='MEDIA 10')
        ax.legend()
        canvas.draw_idle()
        time.sleep(0.25)

def iniciar_grafica2():
    global n
    while grafica2:
        with data_lock:
            xs = list(range(len(humedades)))
            ys = list(humedades)
        if not ys:
            time.sleep(0.1)
            continue
        ax.clear()
        ax.plot(xs, ys, color='red', label='HUMEDAD')
        ax.legend()
        canvas.draw_idle()
        time.sleep(0.25)



def RADARClick():
    global radar, sonar, fig2, ax2, radarEncendido, radarmanual 
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
    mensaje = "IniciarD"
    mySerial.write(mensaje.encode('utf-8'))

def RADARMClick():
    iniciar_radarmanual()
    RADARClick()
    mensaje= "RadarManual"
    mySerial.write(mensaje.encode('utf-8'))
   

def iniciar_radarmanual():
    window = Tk()
    window.title("Control de Motor - Slider Izquierda/Derecha")

    tituloLabel = Label(window, text="Mover motor", font=("Arial", 14))
    tituloLabel.grid( pady=20)

    slider = Scale(
        window,
        from_=-1,
        to=1,
        orient="horizontal",
        length=300,
        resolution=0.1,
        command=on_slider
    )
    slider.set(0)  # Posición inicial (centro)
    slider.grid( pady=20)

    window.mainloop()


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

def enviar_direccion(valor):
    comando = f"DIR:{valor}\n"
    arduino.write(comando.encode())
    print("Enviado:", comando)

def on_slider(val):
    val = float(val)
    print("Valor real del slider:", val)

    if val < -0.5:
        estado = 0
    elif val > 0.5:
        estado = 1
    else:
        estado = 2

    enviar_direccion(estado)


window = Tk()
window.geometry("1600x900")
window.rowconfigure(0, weight=1)
window.rowconfigure(1, weight=1)
window.rowconfigure(2, weight=1)
window.rowconfigure(3, weight=1)
window.rowconfigure(4, weight=6)
window.rowconfigure(5, weight=1)
window.rowconfigure(6, weight=1)
for c in range(6):
    window.columnconfigure(c, weight=1)

tituloLabel = Label(window, text="VERSION 2", font=("Times New Roman", 20, "bold"))
tituloLabel.grid(row=0, column=0, columnspan=6, padx=5, pady=5, sticky=N+S+E+W)

botones = {
    'width': 12,
    'height': 2,
    'font': ("Arial", 11, "bold"),
    'relief': 'raised',
    'bd': 3
}

TEMPButton = Button(window, text="TEMP", command=TEMPClick, bg='blue', fg='white', **botones)
TEMPButton.grid(row=2, column=0, padx=5, pady=5, sticky=N+S+E+W)
STOPTEMPButton = Button(window, text="STOPTEMP", command=STOPTClick, bg='blue', fg='white', **botones)
STOPTEMPButton.grid(row=5, column=0, padx=5, pady=5, sticky=N+S+E+W)

HUMButton = Button(window, text="HUM", command=HUMClick, bg='red', fg='white', **botones)
HUMButton.grid(row=2, column=1, padx=5, pady=5, sticky=N+S+E+W)
STOPHUMButton = Button(window, text="STOPHUM", command=STOPHClick, bg='red', fg='white', **botones)
STOPHUMButton.grid(row=5, column=1, padx=5, pady=5, sticky=N+S+E+W)

REANUDARButton = Button(window, text="REANUDAR", command=REANUDARClick, bg='purple', fg='white', **botones)
REANUDARButton.grid(row=2, column=2, padx=5, pady=5, sticky=N+S+E+W)
STOPButton = Button(window, text="STOP", command=STOPClick, bg='purple', fg='white', **botones)
STOPButton.grid(row=5, column=2, padx=5, pady=5, sticky=N+S+E+W)

RADARButton = Button(window, text="RADAR", command=RADARClick, bg='green', fg='white', **botones)
RADARButton.grid(row=2, column=3, padx=5, pady=5, sticky=N+S+E+W)
RADARMButton = Button(window, text="RADAR MANUAL", command=RADARMClick, bg='green', fg='white', **botones)
RADARMButton.grid(row=2, column=4, padx=5, pady=5, sticky=N+S+E+W)
STOPDButton = Button(window, text="STOPDIST", command=STOPDClick, bg='green', fg='white', **botones)
STOPDButton.grid(row=5, column=3, padx=5, pady=5, sticky=N+S+E+W)

plot_frame = Frame(window, bd=2, relief='groove')
plot_frame.grid(row=4, column=0, columnspan=3, sticky=N+S+E+W, padx=5, pady=5)

radar_frame = Frame(window, bd=2, relief='groove')
radar_frame.grid(row=4, column=3, columnspan=3, sticky=N+S+E+W, padx=5, pady=5)


if ser is not None:
    threading.Thread(target=lector_serial, daemon=True).start()
else:
    print("Advertencia: puerto serie no abierto. El programa continuará pero no recibirá datos.")

window.mainloop()
