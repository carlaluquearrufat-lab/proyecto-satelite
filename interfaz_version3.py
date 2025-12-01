from tkinter import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial, threading, time, math


import sys
import matplotlib.pyplot as plt
import re
import matplotlib

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

# Matplotlib
canvas_temp = None
fig_temp = None
ax_temp = None

canvas_radar = None
fig_radar = None
ax_radar = None

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
                            part = linea.split('T:')[-1]
                            val = part.split()[0]
                            temperaturas.append(float(val))
                            eje_x.append(len(temperaturas)-1)
                        except: pass
                    # Lectura de humedad
                    if 'H:' in linea:
                        try:
                            part = linea.split('H:')[-1]
                            val = part.split()[0]
                            humedades.append(float(val))
                        except: pass
                    # Lectura de datos radar (Arduino envía DATA angulo,distancia)
                    if linea.startswith('DATA'):
                        try:
                            _, valores = linea.split()
                            ang_str, dist_str = valores.split(',')
                            ang = float(ang_str)
                            dist = float(dist_str)
                            angulos.append(ang)
                            distancias.append(dist)
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
    # Convertir valor del slider a float
    val = float(val)
    # Mapear -90..90 del slider a 0..180
    angulo = int(90 + val)
    if angulo < 0: angulo = 0
    if angulo > 180: angulo = 180
    # Enviar comando al Arduino
    if ser is not None:
        comando = f"DIR:{angulo}\n"
        ser.write(comando.encode())
        print("Enviado:", comando)

# ---------------- BOTONES ----------------
def TEMPClick():
    global grafica_temp
    grafica_temp = True
    init_grafica_temp()
    threading.Thread(target=plot_temp, daemon=True).start()
    if ser is not None:
        ser.write(b"IniciarT\n")

def HUMClick():
    global grafica_hum
    grafica_hum = True
    init_grafica_temp()
    threading.Thread(target=plot_hum, daemon=True).start()
    if ser is not None:
        ser.write(b"IniciarH\n")

def STOPTClick():
    global grafica_temp
    grafica_temp = False
    if ser is not None:
        ser.write(b"PararT\n")

def STOPHClick():
    global grafica_hum
    grafica_hum = False
    if ser is not None:
        ser.write(b"PararH\n")

def RADARClick():
    init_radar()

def RADARMClick():
    radar_manual()
    init_radar()
    if ser is not None:
        ser.write(b"RadarManual\n")




# Use TkAgg backend for interactive plotting
matplotlib.use('TkAgg')

# Regular expression to extract the X, Y, and Z coordinates from the input
regex = re.compile(r"Position: \(X: ([\d\.-]+) m, Y: ([\d\.-]+) m, Z: ([\d\.-]+) m\)")

# Initialize lists to store the X, Y coordinates for plotting
x_vals = []
y_vals = []

# Constants
R_EARTH = 6371000  # Radius of Earth in meters

# Set up the plot
plt.ion()  # Turn on interactive mode for dynamic updates
fig, ax = plt.subplots()
orbit_plot, = ax.plot([], [], 'bo-', label='Satellite Orbit', markersize=2)  # Line for the orbit with smaller markers
last_point_plot = ax.scatter([], [], color='red', s=50, label='Last Point')  # Scatter plot for the last point

# Draw the Earth's surface as a circle
earth_circle = plt.Circle((0, 0), R_EARTH, color='orange', fill=False, label='Earth Surface')
ax.add_artist(earth_circle)

# Set initial plot limits
ax.set_xlim(-7e6, 7e6)
ax.set_ylim(-7e6, 7e6)
ax.set_aspect('equal', 'box')
ax.set_xlabel('X (meters)')
ax.set_ylabel('Y (meters)')
ax.set_title('Satellite Equatorial Orbit (View from North Pole)')
ax.grid(True)
ax.legend()

# Flag to indicate if the window is closed
window_closed = False

# Function to handle window close event
def on_close(event):
    global window_closed
    print("Window closed")
    plt.close(fig)
    window_closed = True
    sys.exit(0)

# Connect the close event to the handler
fig.canvas.mpl_connect('close_event', on_close)

# Function to draw the Earth's slice at a given Z coordinate
def draw_earth_slice(z):
    slice_radius = (R_EARTH**2 - z**2)**0.5 if abs(z) <= R_EARTH else 0
    earth_slice = plt.Circle((0, 0), slice_radius, color='orange', fill=False, linestyle='--', label='Earth Slice at Z')
    return earth_slice

# Initialize the Earth's slice
earth_slice = draw_earth_slice(0)
ax.add_artist(earth_slice)

# Read form serial port in real-time
import serial
ser = serial.Serial('COM4:', 9600, timeout=1)

while not window_closed:
    if ser.in_waiting <= 0:
        continue
    
    line = ser.readline().decode('utf-8').rstrip()

# Read from standard input in real-time
#for line in sys.stdin:
    if window_closed:
        break

    # Search for the line containing the satellite's position
    match = regex.search(line)
    if match:
        x = float(match.group(1))
        y = float(match.group(2))
        z = float(match.group(3))

        print(f"X: {x}, Y: {y}, Z: {z}")

        # Append the new position to the lists
        x_vals.append(x)
        y_vals.append(y)

        # Update the plot
        orbit_plot.set_data(x_vals, y_vals)
        last_point_plot.set_offsets([[x_vals[-1], y_vals[-1]]])  # Update the last point

        # Remove the old Earth's slice and add the new one
        earth_slice.remove()
        earth_slice = draw_earth_slice(z)
        ax.add_artist(earth_slice)

        # Check if the new point is outside the current limits and update limits if necessary
        xlim = ax.get_xlim()
        ylim = ax.get_ylim()
        if abs(x) > max(abs(xlim[0]), abs(xlim[1])) or abs(y) > max(abs(ylim[0]), abs(ylim[1])):
            new_xlim = max(abs(xlim[0]), abs(xlim[1]), abs(x)) * 1.1
            new_ylim = max(abs(ylim[0]), abs(ylim[1]), abs(y)) * 1.1
            ax.set_xlim(-new_xlim, new_xlim)
            ax.set_ylim(-new_ylim, new_ylim)
            # Debugging information
            print(f"Updated plot limits: xlim={ax.get_xlim()}, ylim={ax.get_ylim()}")
    
        plt.draw()
        fig.canvas.flush_events()  # Force a redraw of the plot

# Show the final plot when the input ends
plt.ioff()
plt.show()





# ---------------- INTERFAZ ----------------
window = Tk()
window.geometry("1600x900")
window.title("INTERFAZ SATELITE")
window.rowconfigure([0,1,2,3,4,5,6], weight=1)
window.columnconfigure([0,1,2,3,4,5], weight=1)

# Titulo
Label(window, text="VERSION 2", font=("Times New Roman", 20, "bold")).grid(row=0,column=0,columnspan=6,sticky=N+S+E+W)

# Botones
botones = {'width':12,'height':2,'font':("Arial",11,"bold"),'relief':'raised','bd':3}

Button(window,text="TEMP", command=TEMPClick, bg='blue',fg='white',**botones).grid(row=2,column=0,sticky=N+S+E+W)
Button(window,text="STOPTEMP", command=STOPTClick, bg='blue',fg='white',**botones).grid(row=5,column=0,sticky=N+S+E+W)
Button(window,text="HUM", command=HUMClick, bg='red',fg='white',**botones).grid(row=2,column=1,sticky=N+S+E+W)
Button(window,text="STOPHUM", command=STOPHClick, bg='red',fg='white',**botones).grid(row=5,column=1,sticky=N+S+E+W)
Button(window,text="RADAR", command=RADARClick, bg='green',fg='white',**botones).grid(row=2,column=3,sticky=N+S+E+W)
Button(window,text="RADAR MANUAL", command=RADARMClick, bg='green',fg='white',**botones).grid(row=2,column=4,sticky=N+S+E+W)

# Frames
plot_frame = Frame(window, bd=2, relief='groove')
plot_frame.grid(row=4,column=0,columnspan=3,sticky=N+S+E+W,padx=5,pady=5)

radar_frame = Frame(window, bd=2, relief='groove')
radar_frame.grid(row=4,column=3,columnspan=3,sticky=N+S+E+W,padx=5,pady=5)

# ---------------- INICIAR HILO SERIAL ----------------
if ser is not None:
    threading.Thread(target=lector_serial, daemon=True).start()
else:
    print("Puerto serial no abierto. No se recibirán datos.")

window.mainloop()
