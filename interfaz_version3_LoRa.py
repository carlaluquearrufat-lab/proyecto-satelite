from tkinter import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial, threading, time, math
import sys
import re
import matplotlib

# IMPORTANT: set backend before importing pyplot
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

# ---------------- SERIAL ----------------
device = 'COM7'
try:
    ser = serial.Serial(device, 9600, timeout=1)
    print("Serial abierto en", device)
except Exception as e:
    print("No se pudo abrir serial:", e)
    ser = None

# ---------------- VARIABLES ----------------
data_lock = threading.Lock()

temperaturas = []
humedades = []
distancias = []
angulos = []
eje_x = []


# Máxima cantidad de puntos a mostrar en las gráficas en tiempo real
MAX_POINTS_RADAR = 5    # número de lecturas visibles en el radar (ventana deslizante)
MAX_POINTS_ORBIT = 10   # número de puntos visibles en la órbita

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
# Regex robusta: acepta notación científica, espacios variables, opcional 'm'
regex = re.compile(
    r"Position:\s*\(X:\s*([-\d.eE]+)\s*m?,\s*Y:\s*([-\d.eE]+)\s*m?,\s*Z:\s*([-\d.eE]+)\s*m?\)"
)
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

                # Debug minimal: print raw position lines to help see format (comment if noisy)
                # print("RAW serial:", repr(linea))

                with data_lock:
                    # Lectura de temperatura
                    if '1:' in linea:
                        try:
                            val = float(linea.split('1:')[-1].split()[0])
                            temperaturas.append(val)
                            eje_x.append(len(temperaturas)-1)
                        except Exception:
                            pass
                    # Lectura de humedad
                    if '2:' in linea:
                        try:
                            val = float(linea.split('2:')[-1].split()[0])
                            humedades.append(val)
                        except Exception:
                            pass
                    # Lectura de datos radar (Arduino envía DATA angulo,distancia)
                    if linea.startswith('DATA'):
                        try:
                            _, valores = linea.split()
                            ang, dist = map(float, valores.split(','))
                            angulos.append(ang)
                            distancias.append(dist)
                        except Exception:
                            pass
                    # Lectura de posición de órbita: use regex robusta
                    # This replaces the previous fragile manual split approach
                    if 'Position' in linea:
                        try:
                            m = regex.search(linea)
                            if m:
                                x = float(m.group(1))
                                y = float(m.group(2))
                                z = float(m.group(3))
                                x_vals.append(x)
                                y_vals.append(y)
                                z_vals.append(z)
                            else:
                                # If regex doesn't match, optional: try a fallback manual parse (silently ignore)
                                pass
                        except Exception:
                            pass
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

# Variables globales para las líneas
linea_temp = None
linea_media = None
linea_hum = None

def plot_temp():
    global grafica_temp, linea_temp, linea_media
    MAX_POINTS_TEMP = 50
    init_grafica_temp()
    
    with data_lock:
        ys = temperaturas[-MAX_POINTS_TEMP:]
        xs = list(range(len(ys)))

    # Inicializar líneas si no existen
    if linea_temp is None:
        linea_temp, = ax_temp.plot(xs, ys, color='blue', label='TEMPERATURA')
        linea_media, = ax_temp.plot(xs, ys, color='green', label='MEDIA 10')
        ax_temp.set_xlabel('Últimas lecturas')
        ax_temp.set_ylabel('Temperatura (°C)')
        ax_temp.legend()
        ax_temp.set_xlim(0, MAX_POINTS_TEMP)
        ax_temp.set_ylim(0, max(ys + [30]))
    
    while grafica_temp:
        with data_lock:
            ys_all = list(temperaturas)
        if not ys_all:
            time.sleep(0.1)
            continue

        ys = ys_all[-MAX_POINTS_TEMP:]
        xs = list(range(len(ys)))

        # Actualizar líneas sin limpiar el eje
        linea_temp.set_data(xs, ys)
        if len(ys) >= 10:
            media = [sum(ys[-10:])/10.0]*len(ys)
            linea_media.set_data(xs, media)

        # Ajustar límites dinámicamente si quieres
        ymin = min(ys) - 1
        ymax = max(ys) + 1
        ax_temp.set_ylim(ymin, ymax)

        canvas_temp.draw_idle()
        time.sleep(0.1)

def plot_hum():
    global grafica_hum, linea_hum
    MAX_POINTS_TEMP = 50
    init_grafica_temp()
    
    if linea_hum is None:
        with data_lock:
            ys = humedades[-MAX_POINTS_TEMP:]
            xs = list(range(len(ys)))
        linea_hum, = ax_temp.plot(xs, ys, color='red', label='HUMEDAD')
        ax_temp.legend()

    while grafica_hum:
        with data_lock:
            ys_all = list(humedades)
        if not ys_all:
            time.sleep(0.1)
            continue

        ys = ys_all[-MAX_POINTS_TEMP:]
        xs = list(range(len(ys)))

        linea_hum.set_data(xs, ys)
        ymin = min(ys) - 1
        ymax = max(ys) + 1
        ax_temp.set_ylim(ymin, ymax)

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
        ser.write(b"R3\n")

def actualizar_radar():
    global radarEncendido
    while radarEncendido:
        try:
            with data_lock:
                angs_all = list(angulos)
                dists_all = list(distancias)
            # Take only the last MAX_POINTS_RADAR points for plotting (sliding window)
            if not angs_all or not dists_all or len(angs_all) != len(dists_all):
                time.sleep(0.1)
                continue
            angs = angs_all[-MAX_POINTS_RADAR:]
            dists = dists_all[-MAX_POINTS_RADAR:]

            ax_radar.clear()
            ax_radar.set_theta_zero_location("E")
            ax_radar.set_theta_direction(-1)
            ax_radar.set_ylim(0, max(dists)*1.1 if dists else 1)
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
        ser.write(f"RM:{angulo}\n".encode())
        print("Enviado:", angulo)

# ---------------- ORBITA (INTEGRADA EN LA INTERFAZ) ----------------
def init_orbita():
    global canvas_orbita, fig_orbita, ax_orbita, orbitaEncendida

    if orbitaEncendida:
        return
    orbitaEncendida = True

    # Create figure embedded in orbit_frame
    fig_orbita = Figure(figsize=(6,6), dpi=100)
    ax_orbita = fig_orbita.add_subplot(111)
    ax_orbita.set_aspect('equal', 'box')
    ax_orbita.set_xlabel('X (metros)')
    ax_orbita.set_ylabel('Y (metros)')
    ax_orbita.set_title('Órbita Ecuatorial del Satélite (Vista desde el Polo Norte)')
    ax_orbita.grid(True)

    # Ensure initial limits include Earth
    ax_orbita.set_xlim(-7e6, 7e6)
    ax_orbita.set_ylim(-7e6, 7e6)

    # Initial artists
    orbit_plot, = ax_orbita.plot([], [], 'bo-', label='Satellite Orbit', markersize=2)
    last_point_plot = ax_orbita.scatter([], [], color='red', s=50, label='Last Point')
    earth_circle = plt.Circle((0, 0), R_EARTH, color='orange', fill=False, label='Earth Surface')
    ax_orbita.add_artist(earth_circle)
    earth_slice = plt.Circle((0, 0), 0, color='orange', fill=False, linestyle='--', label='Earth Slice at Z')
    ax_orbita.add_artist(earth_slice)
    ax_orbita.legend()

    canvas_orbita = FigureCanvasTkAgg(fig_orbita, master=orbit_frame)
    canvas_orbita.get_tk_widget().pack(fill='both', expand=True)

    # Thread that updates the orbit plot using x_vals,y_vals,z_vals
    def actualizar_orbita():
        nonlocal earth_slice, orbit_plot, last_point_plot
        while orbitaEncendida:
            try:
                with data_lock:
                    xs_all = list(x_vals)
                    ys_all = list(y_vals)
                    zs_all = list(z_vals)
                # Use a sliding window for plotting to avoid too many points
                if not xs_all or not ys_all:
                    time.sleep(0.15)
                    continue
                xs = xs_all[-MAX_POINTS_ORBIT:]
                ys = ys_all[-MAX_POINTS_ORBIT:]
                zs = zs_all[-MAX_POINTS_ORBIT:]

                # Update orbit line and last point
                orbit_plot.set_data(xs, ys)
                last_point_plot.set_offsets([[xs[-1], ys[-1]]])

                # Update earth slice based on last z
                z = zs[-1] if zs else 0.0
                slice_radius = (R_EARTH**2 - z**2)**0.5 if abs(z) <= R_EARTH else 0.0
                # remove and re-add artist
                try:
                    earth_slice.remove()
                except Exception:
                    pass
                earth_slice = plt.Circle((0, 0), slice_radius, color='orange', fill=False, linestyle='--', label='Earth Slice at Z')
                ax_orbita.add_artist(earth_slice)

                # Auto-scale if needed
                xlim = ax_orbita.get_xlim()
                ylim = ax_orbita.get_ylim()
                max_x = max(abs(x) for x in xs) if xs else 0
                max_y = max(abs(y) for y in ys) if ys else 0
                max_existing = max(abs(xlim[0]), abs(xlim[1]), abs(ylim[0]), abs(ylim[1]), 1.0)
                needed = max(max_x, max_y, R_EARTH) * 1.1
                if needed > max_existing:
                    new_lim = max(needed, max_existing)
                    ax_orbita.set_xlim(-new_lim, new_lim)
                    ax_orbita.set_ylim(-new_lim, new_lim)

                canvas_orbita.draw_idle()
            except Exception as e:
                print("Error actualizar_orbita:", e)
            time.sleep(0.2)

    threading.Thread(target=actualizar_orbita, daemon=True).start()

# ---------------- BOTONES ----------------
def TEMPClick():
    global grafica_temp
    grafica_temp = True
    init_grafica_temp()
    threading.Thread(target=plot_temp, daemon=True).start()
    if ser: ser.write(b"R1\n")

def HUMClick():
    global grafica_hum
    grafica_hum = True
    init_grafica_temp()
    threading.Thread(target=plot_hum, daemon=True).start()
    if ser: ser.write(b"R2\n")

def STOPTClick():
    global grafica_temp
    grafica_temp = False
    if ser: ser.write(b"S1\n")

def STOPHClick():
    global grafica_hum
    grafica_hum = False
    if ser: ser.write(b"S2\n")

def RADARClick():
    init_radar()

def RADARMClick():
    radar_manual()
    init_radar()

def ORBITClick():
    init_orbita()

# ---------------- INTERFAZ ----------------
window = Tk()
window.geometry("1800x1000")
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
