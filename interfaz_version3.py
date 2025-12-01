from tkinter import *
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import serial, threading, time, re, math



import sys
import matplotlib.pyplot as plt
import re
import matplotlib

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
    ser.write((mensaje + "\n").encode('utf-8'))

def HUMClick():
    global grafica2
    grafica2 = True
    init_grafica_if_needed()
    threading.Thread(target=iniciar_grafica2, daemon=True).start()
    mensaje = "IniciarH"
    ser.write((mensaje + "\n").encode('utf-8'))

def STOPTClick():
    global grafica
    mensaje = "PararT"
    ser.write((mensaje + "\n").encode('utf-8'))
    grafica= False

def STOPHClick():
    global grafica2
    mensaje = "PararH"
    ser.write((mensaje + "\n").encode('utf-8'))
    grafica2= False

def STOPDClick():
    global grafica3
    mensaje = "PararD"
    ser.write((mensaje + "\n").encode('utf-8'))
    grafica3= False

def STOPClick():
    global grafica, grafica2, grafica3
    mensaje = "STOP"
    ser.write((mensaje + "\n").encode('utf-8'))
    grafica = False
    grafica2 = False
    grafica3 = False

def REANUDARClick():
    global grafica, grafica2, grafica3
    mensaje = "REANUDAR"
    ser.write((mensaje + "\n").encode('utf-8'))
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
    ser.write((mensaje + "\n").encode('utf-8'))

def RADARMClick():
    iniciar_radarmanual()
    RADARClick()
    mensaje= "RadarManual"
    ser.write((mensaje + "\n").encode('utf-8'))
   

def iniciar_radarmanual():
    window2 = Tk()
    window2.title("Control de Motor - Slider Izquierda/Derecha")

    tituloLabel = Label(window2, text="Mover motor", font=("Arial", 14))
    tituloLabel.grid( pady=20)

    slider = Scale(
        window2,
        from_=-90,
        to=90,
        orient="horizontal",
        length=300,
        resolution=1,
        command=on_slider
    )
    slider.set(0)  # Posición inicial (centro)
    slider.grid( pady=20)

    window2.mainloop()


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
    ser.write(comando.encode())
    print("Enviado:", comando)

def on_slider(val):
    val = float(val)

    if val < -45:
        estado = 0
    elif val > 45:
        estado = 1
    else:
        estado = 2

    enviar_direccion(estado)



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
