from tkinter import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial, threading
import math

device = 'COM7'
ser = serial.Serial(device, 9600)

temperaturas = []
humedades = []
eje_x = []
eje_x2 = []
i = 0
n = 0
grafica = False
grafica2 = False
grafica3 = False

# Canvas y figura para matplotlib integrados en Tkinter
canvas = None
fig = None
ax = None

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

def HUMClick():
    global grafica2
    grafica2 = True
    init_grafica_if_needed()
    threading.Thread(target=iniciar_grafica2, daemon=True).start()

def STOPClick():
    global grafica, grafica2
    grafica = False
    grafica2 = False
    grafica3 = False

def iniciar_grafica():
    global i
    while grafica:
        if ser.in_waiting > 0:
            linea = ser.readline().decode().strip()
            trozos = linea.split('T:')
            if len(trozos) < 2: 
                continue
            try:
                temperatura = float(trozos[1])
            except:
                continue
            temperaturas.append(temperatura)
            eje_x.append(i)
            i += 1
            # actualizar gráfica
            ax.clear()
            ax.plot(eje_x, temperaturas, color='blue', label='TEMPERATURA')
            if len(temperaturas) >= 10:
                media_linea = [sum(temperaturas[-10:])/10.0]*len(temperaturas)
                ax.plot(eje_x, media_linea, color='green', label='MEDIA 10')
            ax.legend()
            canvas.draw_idle()

def iniciar_grafica2():
    global n
    while grafica2:
        if ser.in_waiting > 0:
            linea = ser.readline().decode().strip()
            trozos = linea.split('H:')
            if len(trozos) < 2: 
                continue
            try:
                humedad = float(trozos[1])
            except:
                continue
            humedades.append(humedad)
            eje_x2.append(n)
            n += 1
            # actualizar gráfica
            ax.clear()
            ax.plot(eje_x2, humedades, color='red', label='HUMEDAD')
            ax.legend()
            canvas.draw_idle()

def GRAFClick():
    global grafica3
    grafica3 = True
    init_grafica_if_needed()  # inicializa gráfica solo la primera vez
    threading.Thread(target=iniciar_grafica3, daemon=True).start()

def iniciar_grafica3():
    global n, grafica3
    while grafica3:
        if ser.in_waiting > 0:
            linea = ser.readline().decode().strip()
            trozos_h = linea.split('H:')
            if len(trozos_h) >= 2:
                try:
                    humedad = float(trozos_h[1])
                    humedades.append(humedad)
                    eje_x2.append(n)
                    n += 1
                except:
                    pass

            linea = ser.readline().decode().strip()
            trozos_t = linea.split('T:')
            if len(trozos_t) >= 2:
                try:
                    temperatura = float(trozos_t[1])
                    temperaturas.append(temperatura)
                    eje_x.append(len(temperaturas)-1)
                except:
                    pass

            # actualizar gráfica integrada
            ax.clear()
            ax.plot(eje_x, temperaturas, color='blue', label='TEMPERATURA')
            ax.plot(eje_x2, humedades, color='red', label='HUMEDAD')
            if len(temperaturas) >= 10:
                media_linea = [sum(temperaturas[-10:])/10.0]*len(temperaturas)
                ax.plot(eje_x, media_linea, color='green', label='MEDIA 10')
            ax.legend()
            canvas.draw_idle()


window = Tk()
window.geometry("1280x720")
window.rowconfigure(0, weight=1)
window.rowconfigure(1, weight=1)
window.rowconfigure(2, weight=1)
window.rowconfigure(3, weight=6)
window.columnconfigure(0, weight=1)
window.columnconfigure(1, weight=1)
window.columnconfigure(2, weight=1)
window.columnconfigure(3, weight=1)

tituloLabel = Label(window, text="VERSION 2", font=("Times New Roman", 20, "bold"))
tituloLabel.grid(row=0, column=0, columnspan=5, padx=5, pady=5, sticky=N+S+E+W)

# Formato botones
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

plot_frame = Frame(window)
plot_frame.grid(row=3, column=0, columnspan=4, sticky=N+S+E+W, padx=5, pady=5)

window.mainloop()

