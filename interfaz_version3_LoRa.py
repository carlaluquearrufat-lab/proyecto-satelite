from tkinter import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial, threading, time, math

# ---------------- SERIAL ----------------
device = 'COM7'
try:
    ser = serial.Serial(device, 9600, timeout=1)
    print("Serial abierto en", device)
except Exception as e:
    print("No se pudo abrir serial:", e)
    ser = None

# ---------------- VARIABLES ----------------
temperaturas = []
humedades = []
distancias = []
angulos = []
eje_x = []

data_lock = threading.Lock()

# Flags de graficas
grafica_temp = False
grafica_hum = False
grafica_dist = False
radarEncendido = False
orbitaEncendida = False

# Matplotlib para Tkinter
canvas_temp = None
fig_temp = None
ax_temp = None

canvas_radar = None
fig_radar = None
ax_radar = None

canvas_orbita = None
fig_orbita = None
ax_orbita = None

# Variables para la órbita
R_EARTH = 6371000  # Radio de la Tierra en metros
x_vals = []
y_vals = []
z_vals = []

# ---------------- LECTOR SERIAL ----------------
def lector_serial():
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

                with data_lock:
                    # Lectura de temperatura
                    if 'T:' in linea:
                        try:
                            val = float(linea.split('T:')[-1].split()[0])
                            temperaturas.append(val)
                            eje_x.append(len(temperaturas)-1)
                        except: pass
                    # Lectura de humedad
                    if 'H:' in linea:
                        try:
                            val = float(linea.split('H:')[-1].split()[0])
                            humedades.append(val)
                        except: pass
                    # Lectura de datos radar (Arduino envía DATA angulo,distancia)
                    if linea.startswith('DATA'):
                        try:
                            _, valores = linea.split()
                            ang, dist = map(float, valores.split(','))
                            angulos.append(ang)
                            distancias.append(dist)
                        except: pass
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
        except Exception as e:
            print("Error lector_serial:", e)
        time.sleep(0.01)

# ---------------- FUNCIONES GRAFICAS ----------------
def init_grafica_temp():
    global canvas_temp, fig_temp, ax_temp
    if canvas_temp is None:
        fig_temp = Figure(figsize=(8,4), dpi=100)
        ax_temp = fig_temp.add_subplot(111)
        ax_temp.set_facecolor('#FFFFFF')
        canvas_temp = FigureCanvasTkAgg(fig_temp, master=plot_frame)
        canvas_temp.get_tk_widget().pack(fill='both', expand=True)

def plot_temp():
    global grafica_temp
    while grafica_temp:
        with data_lock:
            xs = list(eje_x)
            ys = list(temperaturas)
        if not ys:
            time.sleep(0.1)
            continue
        ax_temp.clear()
        ax_temp.plot(xs, ys, color='blue', label='TEMPERATURA')
        if len(ys) >= 10:
            media_linea = [sum(ys[-10:])/10.0]*len(ys)
            ax_temp.plot(xs, media_linea, color='green', label='MEDIA 10')
        ax_temp.legend()
        canvas_temp.draw_idle()
        time.sleep(0.25)

def plot_hum():
    global grafica_hum
    while grafica_hum:
        with data_lock:
            xs = list(range(len(humedades)))
            ys = list(humedades)
        if not ys:
            time.sleep(0.1)
            continue
        ax_temp.clear()
        ax_temp.plot(xs, ys, color='red', label='HUMEDAD')
        ax_temp.legend()
        canvas_temp.draw_idle()
        time.sleep(0.25)

# ---------------- RADAR ----------------
def init_radar():
    global canvas_radar, fig_radar, ax_radar, radarEncendido
    if radarEncendido:
        return
    radarEncendido = True
    fig_radar = Figure(figsize=(6,4), dpi=100)
    ax_radar = fig_radar.add_subplot(111, polar=True)
    ax_radar.set_theta_zero_location("E")
    ax_radar.set_theta_direction(-1)
    ax_radar.set_title("Radar Ultrasonico")
    canvas_radar = FigureCanvasTkAgg(fig_radar, master=radar_frame)
    canvas_radar.get_tk_widget().pack(fill='both', expand=True)
    threading.Thread(target=actualizar_radar, daemon=True).start()
    if ser is not None:
        ser.write(b"IniciarD\n")

def actualizar_radar():
    global radarEncendido
    while radarEncendido:
        try:
            with data_lock:
                angs = list(angulos)
                dists = list(distancias)
            if not angs or not dists or len(angs) != len(dists):
                time.sleep(0.1)
                continue
            ax_radar.clear()
            ax_radar.set_theta_zero_location("E")
            ax_radar.set_theta_direction(-1)
            ax_radar.set_ylim(0, max(dists)*1.1)
            thetas = [math.radians(a) for a in angs]
            ax_radar.plot(thetas, dists, marker='o', linestyle='-', linewidth=2)
            ax_radar.plot([thetas[-1]], [dists[-1]], marker='o', markersize=8)
            canvas_radar.draw_idle()
        except Exception as e:
            print("Error radar:", e)
        time.sleep(0.2)

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

# ---------------- BOTONES ----------------
def TEMPClick():
    global grafica_temp
    grafica_temp = True
    init_grafica_temp()
    threading.Thread(target=plot_temp, daemon=True).start()
    if ser: ser.write(b"IniciarT\n")

def HUMClick():
    global grafica_hum
    grafica_hum = True
    init_grafica_temp()
    threading.Thread(target=plot_hum, daemon=True).start()
    if ser: ser.write(b"IniciarH\n")

def STOPTClick():
    global grafica_temp
    grafica_temp = False
    if ser: ser.write(b"PararT\n")

def STOPHClick():
    global grafica_hum
    grafica_hum = False
    if ser: ser.write(b"PararH\n")

def RADARClick():
    init_radar()

def RADARMClick():
    radar_manual()
    init_radar()
    if ser: ser.write(b"RadarManual\n")

def ORBITClick():
    init_orbita()

# ---------------- INTERFAZ ----------------
window = Tk()
window.geometry("1600x900")
window.title("INTERFAZ SATELITE")
window.rowconfigure([0,1,2,3,4,5,6], weight=1)
window.columnconfigure([0,1,2,3,4,5], weight=1)

Label(window, text="VERSION 2", font=("Times New Roman", 20, "bold")).grid(row=0,column=0,columnspan=6,sticky=N+S+E+W)

botones = {'width':12,'height':2,'font':("Arial",11,"bold"),'relief':'raised','bd':3}
Button(window,text="TEMP", command=TEMPClick, bg='blue',fg='white',**botones).grid(row=2,column=0,sticky=N+S+E+W)
Button(window,text="STOPTEMP", command=STOPTClick, bg='blue',fg='white',**botones).grid(row=5,column=0,sticky=N+S+E+W)
Button(window,text="HUM", command=HUMClick, bg='red',fg='white',**botones).grid(row=2,column=1,sticky=N+S+E+W)
Button(window,text="STOPHUM", command=STOPHClick, bg='red',fg='white',**botones).grid(row=5,column=1,sticky=N+S+E+W)
Button(window,text="RADAR", command=RADARClick, bg='green',fg='white',**botones).grid(row=2,column=3,sticky=N+S+E+W)
Button(window,text="RADAR MANUAL", command=RADARMClick, bg='green',fg='white',**botones).grid(row=2,column=4,sticky=N+S+E+W)
Button(window,text="ORBITA", command=ORBITClick, bg='orange',fg='white',**botones).grid(row=2,column=5,sticky=N+S+E+W)

plot_frame = Frame(window, bd=2, relief='groove')
plot_frame.grid(row=4,column=0,columnspan=3,sticky=N+S+E+W,padx=5,pady=5)

radar_frame = Frame(window, bd=2, relief='groove')
radar_frame.grid(row=4,column=3,columnspan=2,sticky=N+S+E+W,padx=5,pady=5)

orbit_frame = Frame(window, bd=2, relief='groove')
orbit_frame.grid(row=4,column=5,columnspan=1,sticky=N+S+E+W,padx=5,pady=5)

# ---------------- INICIAR HILO SERIAL ----------------
if ser is not None:
    threading.Thread(target=lector_serial, daemon=True).start()
else:
    print("Puerto serial no abierto. No se recibirán datos.")

window.mainloop()
