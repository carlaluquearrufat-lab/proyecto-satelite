from tkinter import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial, threading, time, math
import sys
import re
import matplotlib
import datetime

# --- NUEVOS IMPORTS PARA 3D ---
from mpl_toolkits.mplot3d import Axes3D  # Necesario para la proyección 3D
import numpy as np  # Necesario para generar la esfera de la Tierra
# ------------------------------

LOG_FILE = "eventos.log"

# ... (SE MANTIENE EL CÓDIGO DE EVENTOS Y LOGS IGUAL) ...
def registrar_evento(codigo, tipo, mensaje):
    fecha = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    linea = f"{fecha}\t{codigo}\t{tipo}\t{mensaje}\n"
    with open(LOG_FILE, "a") as f:
        f.write(linea)
    print("Evento registrado:", linea.strip())
eventos = {
    # COMANDOS
    201: ("COMANDO", "Parar envío de temperatura (S1)"),
    202: ("COMANDO", "Parar envío de humedad (S2)"),
    203: ("COMANDO", "Parar envío de distancia (S3)"),
    204: ("COMANDO", "Reanudar temperatura (R1)"),
    205: ("COMANDO", "Reanudar humedad (R2)"),
    206: ("COMANDO", "Reanudar distancia (R3)"),
    207: ("COMANDO", "Movimiento manual servo (RM:x)"),
    208: ("COMANDO", "Parar todos los sensores (S)"),
    209: ("COMANDO", "Reanudar todos los sensores (R)"),
    210: ("COMANDO", "Iniciar temperatura normal (R1)"),
    211: ("COMANDO", "Parar temperatura desde interfaz (S1)"),

    # ALARMAS
    301: ("ALARMA", "Error de lectura de temperatura (1!)"),
    302: ("ALARMA", "Error de lectura de humedad (2!)"),
    303: ("ALARMA", "Error combinado TEMP+HUM (1!2)"),
    304: ("ALARMA", "Mensaje corrupto recibido (trama LoRa corrupta)"),
    305: ("ALARMA", "Distancia fuera de rango"),
    306: ("ALARMA", "Eco no recibido (No Echo)"),

    # USUARIO
    401: ("USUARIO", "Observación del usuario (texto libre)"),
    402: ("USUARIO", "Nota importante marcada por el usuario"),
    403: ("USUARIO", "Usuario reporta posible fallo físico"),
    404: ("USUARIO", "Usuario reinicia la interfaz"),
    405: ("USUARIO", "Usuario reinicia el satélite")
}

def registrar_evento_por_codigo(codigo):
    if codigo in eventos:
        tipo, mensaje = eventos[codigo]
        registrar_evento(codigo, tipo, mensaje)
    else:
        print("Código de evento no definido:", codigo)

# IMPORTANT: set backend before importing pyplot
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt

from tkinter import filedialog

# ... (SE MANTIENE ABRIR FICHERO IGUAL) ...
def abrir_fichero_comandos():
    ruta = filedialog.askopenfilename(
        title="Abrir fichero de comandos",
        filetypes=(("Archivos de texto","*.txt"), ("Todos los archivos","*.*"))
    )
    if ruta:
        print("Fichero seleccionado:", ruta)
        with open(ruta, "r") as f:
            lineas = f.readlines()
        for linea in lineas:
            print("Comando:", linea.strip())

# ---------------- SERIAL ----------------
# AVISO: Cambia 'COM7' por tu puerto real si es necesario
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
medias_arduino = []

# Máxima cantidad de puntos a mostrar en las gráficas en tiempo real
MAX_POINTS_RADAR = 5
# SUGERENCIA: Aumentar este valor para ver una estela de órbita más larga en 3D
MAX_POINTS_ORBIT = 50  # Cambiado de 10 a 50 para mejor visualización 3D

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

# ---------------- LECTOR SERIAL (IGUAL QUE EL ORIGINAL) ----------------
def lector_serial():
    if ser is None:
        return
    while True:
        try:
            if ser.in_waiting > 0:
                linea = ser.readline().decode('utf-8', errors='ignore').strip()

                with data_lock:
                    # -------- TEMP --------
                    if '1:' in linea:
                        try:
                            temperaturas.append(float(linea.split('1:')[1].split()[0]))
                        except: pass
                    # -------- HUM --------
                    if ' 2:' in linea:
                        try:
                            humedades.append(float(linea.split(' 2:')[1].split()[0]))
                        except: pass
                    # -------- DIST --------
                    if ' 3:' in linea:
                        try:
                            distancias.append(float(linea.split(' 3:')[1].split()[0]))
                        except: pass
                    # -------- ANGULO --------
                    if ' 4:' in linea:
                        try:
                            angulos.append(float(linea.split(' 4:')[1].split()[0]))
                        except: pass

                    # -------- LECTURA ORBITA --------
                    if linea.startswith("POS"):
                        try:
                            valores = re.findall(r"[-+]?\d*\.\d+|[-+]?\d+", linea)
                            if len(valores) >= 3:
                                x = float(valores[0])
                                y = float(valores[1])
                                z = float(valores[2])

                                x_vals.append(x)
                                y_vals.append(y)
                                z_vals.append(z)

                                if len(x_vals) > 200:
                                    x_vals.pop(0)
                                    y_vals.pop(0)
                                    z_vals.pop(0)
                            else:
                                print("POS recibida incompleta:", linea)
                        except Exception as e:
                            print("Error leyendo POS:", linea, e)

        except Exception as e:
            print("Error lector_serial:", e)

        time.sleep(0.01)

# ---------------- FUNCIONES GRAFICAS TEMP/HUM (IGUAL QUE EL ORIGINAL) ----------------
def init_grafica_temp():
    global canvas_temp, fig_temp, ax_temp
    if canvas_temp is None:
        fig_temp = Figure(figsize=(8,4), dpi=100)
        ax_temp = fig_temp.add_subplot(111)
        ax_temp.set_facecolor('#FFFFFF')
        canvas_temp = FigureCanvasTkAgg(fig_temp, master=plot_frame)
        canvas_temp.get_tk_widget().pack(fill='both', expand=True)

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

        linea_temp.set_data(xs, ys)
        if len(ys) >= 10:
            media = [sum(ys[-10:])/10.0]*len(ys)
            linea_media.set_data(xs, media)

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


# ---------------- RADAR (IGUAL QUE EL ORIGINAL) ----------------
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

def actualizar_radar():
    global radarEncendido
    while radarEncendido:
        try:
            with data_lock:
                angs_all = list(angulos)
                dists_all = list(distancias)
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

# ---------------- RADAR MANUAL (IGUAL QUE EL ORIGINAL) ----------------
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

# =============================================================================
# ---------------- ORBITA 3D (MODIFICADA) ----------------
# =============================================================================
def init_orbita():
    global canvas_orbita, fig_orbita, ax_orbita, orbitaEncendida

    if orbitaEncendida:
        return
    orbitaEncendida = True

    # Crear figura
    fig_orbita = Figure(figsize=(6,6), dpi=100)
    # IMPORTANTE: Usar projection='3d'
    ax_orbita = fig_orbita.add_subplot(111, projection='3d')
    
    # Configuración de los ejes 3D
    ax_orbita.set_xlabel('X (m)')
    ax_orbita.set_ylabel('Y (m)')
    ax_orbita.set_zlabel('Z (m)')
    ax_orbita.set_title('Órbita Satelital 3D')
    
    # --- DIBUJAR LA TIERRA EN 3D ---
    # Generar datos para una esfera
    u = np.linspace(0, 2 * np.pi, 30) # Longitud
    v = np.linspace(0, np.pi, 30)     # Latitud
    x_earth = R_EARTH * np.outer(np.cos(u), np.sin(v))
    y_earth = R_EARTH * np.outer(np.sin(u), np.sin(v))
    z_earth = R_EARTH * np.outer(np.ones(np.size(u)), np.cos(v))

    # Dibujar la superficie de la Tierra como malla de alambre (wireframe) naranja transparente
    ax_orbita.plot_wireframe(x_earth, y_earth, z_earth, color='orange', alpha=0.3, rstride=2, cstride=2)

    # Dibujar Ecuador (Círculo Verde en Z=0)
    theta = np.linspace(0, 2*np.pi, 100)
    ax_orbita.plot(R_EARTH*np.cos(theta), R_EARTH*np.sin(theta), np.zeros_like(theta), color='green', linestyle='--', linewidth=1.5, label='Ecuador')

    # Dibujar Meridiano de Greenwich aproximado (Círculo Amarillo en el plano X-Z, asumiendo Y=0 como cruce)
    phi = np.linspace(0, 2*np.pi, 100)
    ax_orbita.plot(R_EARTH*np.sin(phi), np.zeros_like(phi), R_EARTH*np.cos(phi), color='yellow', linestyle='-', linewidth=1.5, label='Meridiano')

    # Configurar límites iniciales para que se vea la Tierra completa
    limit = R_EARTH * 1.5
    ax_orbita.set_xlim(-limit, limit)
    ax_orbita.set_ylim(-limit, limit)
    ax_orbita.set_zlim(-limit, limit)
    
    # Intentar hacer que los ejes sean iguales para que la esfera parezca redonda
    try:
        ax_orbita.set_box_aspect([1,1,1])
    except: pass # Versiones antiguas de matplotlib podrían no soportarlo

    # Inicializar las gráficas vacías para la órbita y el último punto en 3D
    # Nota: En 3D, plot toma 3 argumentos iniciales (x, y, z)
    orbit_plot, = ax_orbita.plot([], [], [], 'b-', linewidth=2, label='Órbita Satélite')
    last_point_plot = ax_orbita.scatter([], [], [], color='red', s=50, label='Última Posición')
    
    ax_orbita.legend()

    # Vista inicial de la cámara (elevación, azimut)
    ax_orbita.view_init(elev=30, azim=45)

    canvas_orbita = FigureCanvasTkAgg(fig_orbita, master=orbit_frame)
    canvas_orbita.get_tk_widget().pack(fill='both', expand=True)

    # --- HILO DE ACTUALIZACIÓN 3D ---
    def actualizar_orbita():
        nonlocal orbit_plot, last_point_plot
        while orbitaEncendida:
            try:
                with data_lock:
                    xs_all = list(x_vals)
                    ys_all = list(y_vals)
                    zs_all = list(z_vals)

                if not xs_all or not ys_all or not zs_all:
                    time.sleep(0.15)
                    continue
                
                # Usar ventana deslizante para los puntos
                xs = xs_all[-MAX_POINTS_ORBIT:]
                ys = ys_all[-MAX_POINTS_ORBIT:]
                zs = zs_all[-MAX_POINTS_ORBIT:]

                # ACTUALIZAR LA LÍNEA DE ÓRBITA EN 3D
                # Se deben establecer los datos X e Y primero, y luego los Z
                orbit_plot.set_data(xs, ys)
                orbit_plot.set_3d_properties(zs)

                # ACTUALIZAR EL ÚLTIMO PUNTO (SCATTER) EN 3D
                # Scatter 3D usa _offsets3d internamente
                last_point_plot._offsets3d = ([xs[-1]], [ys[-1]], [zs[-1]])

                # Auto-escalado dinámico de los ejes 3D si el satélite se aleja mucho
                max_val = R_EARTH
                if xs: max_val = max(max_val, np.max(np.abs(xs)))
                if ys: max_val = max(max_val, np.max(np.abs(ys)))
                if zs: max_val = max(max_val, np.max(np.abs(zs)))

                current_limit = ax_orbita.get_xlim()[1]
                if max_val * 1.1 > current_limit:
                     new_limit = max_val * 1.2
                     ax_orbita.set_xlim(-new_limit, new_limit)
                     ax_orbita.set_ylim(-new_limit, new_limit)
                     ax_orbita.set_zlim(-new_limit, new_limit)

                canvas_orbita.draw_idle()
            except Exception as e:
                print("Error actualizar_orbita 3D:", e)
            time.sleep(0.2)

    threading.Thread(target=actualizar_orbita, daemon=True).start()
# =============================================================================


# ---------------- BOTONES Y GUI (IGUAL QUE EL ORIGINAL) ----------------
def TEMPClick():
    global grafica_temp
    grafica_temp = True
    init_grafica_temp()
    threading.Thread(target=plot_temp, daemon=True).start()
    if ser: ser.write(b"R1\n")
    registrar_evento_por_codigo(204)

def MEDIAClick():
    if temperaturas:
        media_python = sum(temperaturas[-10:]) / min(len(temperaturas), 10)
        print("Media calculada en Python:", media_python)
        registrar_evento_por_codigo(210)
    if ser:
        ser.write(b"M\n")
        print("Solicitud de media enviada al satélite")

def HUMClick():
    global grafica_hum
    grafica_hum = True
    init_grafica_temp()
    threading.Thread(target=plot_hum, daemon=True).start()
    if ser: ser.write(b"R2\n")
    registrar_evento_por_codigo(205)

def STOPTClick():
    global grafica_temp
    grafica_temp = False
    if ser: ser.write(b"S1\n")
    registrar_evento_por_codigo(201)

def STOPHClick():
    global grafica_hum
    grafica_hum = False
    if ser: ser.write(b"S2\n")
    registrar_evento_por_codigo(202)

def RADARClick():
    init_radar()
    if ser: ser.write(b"R3\n")
    registrar_evento_por_codigo(207)

def RADARMClick():
    radar_manual()
    init_radar()

def ORBITClick():
    init_orbita()
    registrar_evento_por_codigo(210)

# ---------------- INTERFAZ PRINCIPAL ----------------
window = Tk()
window.geometry("1800x1000")
window.title("INTERFAZ SATELITE 3D") # Título actualizado
window.rowconfigure([0,1,2,3,4,5,6], weight=1)
window.columnconfigure([0,1,2,3,4,5], weight=1)

Label(window, text="VERSION 3D", font=("Times New Roman", 20, "bold")).grid(row=0,column=0,columnspan=6,sticky=N+S+E+W)

botones = {'width':12,'height':2,'font':("Arial",11,"bold"),'relief':'raised','bd':3}
Button(window,text="TEMP", command=TEMPClick, bg='blue',fg='white',**botones).grid(row=2,column=0,sticky=N+S+E+W)
Button(window,text="STOPTEMP", command=STOPTClick, bg='blue',fg='white',**botones).grid(row=5,column=0,sticky=N+S+E+W)
Button(window,text="HUM", command=HUMClick, bg='red',fg='white',**botones).grid(row=2,column=1,sticky=N+S+E+W)
Button(window,text="STOPHUM", command=STOPHClick, bg='red',fg='white',**botones).grid(row=5,column=1,sticky=N+S+E+W)
Button(window,text="RADAR", command=RADARClick, bg='green',fg='white',**botones).grid(row=2,column=3,sticky=N+S+E+W)
Button(window,text="RADAR MANUAL", command=RADARMClick, bg='green',fg='white',**botones).grid(row=2,column=4,sticky=N+S+E+W)
Button(window,text="ORBITA 3D", command=ORBITClick, bg='orange',fg='white',**botones).grid(row=2,column=5,sticky=N+S+E+W)
Button(window, text="MEDIA ARDUINO", command=MEDIAClick, bg='purple', fg='white', **botones).grid(row=3, column=0, sticky=N+S+E+W)
Button(window, text="ABRIR COMANDOS", command=abrir_fichero_comandos, bg='gray', fg='white', **botones).grid(row=3, column=1, sticky=N+S+E+W)

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
