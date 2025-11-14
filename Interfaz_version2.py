from tkinter import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial, threading, time, re, math
device = 'COM4'
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

canvas = None
fig = None
ax = None

radar_abierto = None
canvas2 = None
fig2 = None
ax2 = None
radar = False


data_lock = threading.Lock()

def lector_serial():
    global ser
    if ser is None:
        return
    while True:
        try:
            if ser.in_waiting > 0:
                linea1= ser.readline()
                if not linea1:
                    continue
                try:
                    linea = linea1.decode('utf-8', errors='ignore').strip()
                except:
                    linea = linea1.decode('latin1', errors='ignore').strip()
                
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
        except Exception as e:
            print("Error en lector_serial:", e)
            time.sleep(0.2)
        time.sleep(0.01)


def init_grafica_if_needed():
    global canvas, fig, ax
    if canvas is None:
        fig = Figure(figsize=(8,4), dpi=100)
        ax = fig.add_subplot(111)
        ax.set_facecolor('#FFFFFF')
        canvas = FigureCanvasTkAgg(fig, master=plot2)
        canvas.get_tk_widget().pack(fill='both', expand=True)

def GRAFClick():
    global grafica3
    grafica3 = True
    init_grafica_if_needed()
    threading.Thread(target=iniciar_grafica3, daemon=True).start()

def TEMPClick():
    global grafica
    grafica = True
    init_grafica_if_needed()
    threading.Thread(target=iniciar_grafica, daemon=True).start()

def HUMClick():
    global grafica2
    grafica2 = True
    init_grafica_if_needed()
    threading.Thread(target=iniciar_grafica2, daemon=True).start()

def STOPClick():
    global grafica, grafica2, grafica3, radar
    grafica = False
    grafica2 = False
    grafica3 = False
    
    radar = False

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

def iniciar_grafica3():
    global n, grafica3
    while grafica3:
        with data_lock:
            xs_t = list(eje_x)
            ys_t = list(temperaturas)
            xs_h = list(range(len(humedades)))
            ys_h = list(humedades)
        ax.clear()
        if ys_t:
            ax.plot(xs_t, ys_t, color='blue', label='TEMPERATURA')
        if ys_h:
            ax.plot(xs_h, ys_h, color='red', label='HUMEDAD')
        if len(ys_t) >= 10:
            media_linea = [sum(ys_t[-10:])/10.0]*len(ys_t)
            ax.plot(xs_t, media_linea, color='green', label='MEDIA 10')
        ax.legend()
        canvas.draw_idle()
        time.sleep(0.25)


def RADARClick():
    global radar_abierto, canvas2, fig2, ax2, radar
    if radar_abierto is not None and radar:
        return  #ya abierto 

    radar_abierto = True  #marcador: indica que el radar ya fue inicializado
    fig2 = Figure(figsize=(6,4), dpi=100)
    ax2 = fig2.add_subplot(111, polar=True)
    ax2.set_theta_zero_location("E")  # 0° a la derecha
    ax2.set_theta_direction(-1)       # grados hacia arriba
    ax2.set_title("Radar de Ultrasonido")
    
    canvas2 = FigureCanvasTkAgg(fig2, master=Sec)
    canvas2.get_tk_widget().pack(fill='both', expand=True)
    radar = True
    threading.Thread(target=actualizar_radar, daemon=True).start()

def actualizar_radar():
    global radar, ax2, canvas2, radar_abierto
    while radar:

        with data_lock:
            angs = list(angulos)
            dists = list(distancias)
        ax2.clear()
        ax2.set_theta_zero_location("E")
        ax2.set_theta_direction(-1)
        ax2.set_ylim(0, max(dists) * 1.1 if dists else 50)
        #convertir grados a radianes
        if angs and dists and len(angs) == len(dists):
            thetas = [math.radians(a) for a in angs]
            ax2.plot(thetas, dists, marker='o', linestyle='-', linewidth=2)
            #marcar último punto
            ax2.plot([thetas[-1]], [dists[-1]], marker='o', markersize=8)
        elif angs and dists:
            # si distinto largo, plotear pares disponibles por índice
            mn = min(len(angs), len(dists))
            thetas = [math.radians(a) for a in angs[:mn]]
            ax2.plot(thetas, dists[:mn], marker='o', linestyle='-', linewidth=2)
        canvas2.draw_idle()
        time.sleep(0.2)


#Interfaz 
window = Tk()
window.geometry("1600x900")
window.rowconfigure(0, weight=1)
window.rowconfigure(1, weight=1)
window.rowconfigure(2, weight=1)
window.rowconfigure(3, weight=6)
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

TEMPButton = Button(window, text="TEMP.", command=TEMPClick, bg='blue', fg='white', **botones)
TEMPButton.grid(row=2, column=0, padx=5, pady=5, sticky=N+S+E+W)
HUMButton = Button(window, text="HUM", command=HUMClick, bg='red', fg='white', **botones)
HUMButton.grid(row=2, column=2, padx=5, pady=5, sticky=N+S+E+W)
GRAFButton = Button(window, text="GRAFICA", command=GRAFClick, bg='orange', fg='black', **botones)
GRAFButton.grid(row=2, column=3, padx=5, pady=5, sticky=N+S+E+W)
STOPButton = Button(window, text="STOP", command=STOPClick, bg='purple', fg='white', **botones)
STOPButton.grid(row=2, column=1, padx=5, pady=5, sticky=N+S+E+W)

RADARButton = Button(window, text="RADAR", command=RADARClick, bg='green', fg='white', **botones)
RADARButton.grid(row=2, column=4, padx=5, pady=5, sticky=N+S+E+W)

plot2 = Frame(window, bd=2, relief='groove')
plot2.grid(row=3, column=0, columnspan=3, sticky=N+S+E+W, padx=5, pady=5)

Sec = Frame(window, bd=2, relief='groove')
Sec.grid(row=3, column=3, columnspan=3, sticky=N+S+E+W, padx=5, pady=5)

window.mainloop()
