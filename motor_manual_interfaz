import serial
import time
import tkinter as tk


arduino = serial.Serial('COM7', 9600)  # Cambiar COM según tu PC
time.sleep(2)


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
# --------------------------
# INTERFAZ GRÁFICA
# --------------------------
ventana = tk.Tk()
ventana.title("Control de Motor - Slider Izquierda/Derecha")

label = tk.Label(ventana, text="Mover motor", font=("Arial", 14))
label.pack(pady=10)

slider = tk.Scale(
    ventana,
    from_=-1,
    to=1,
    orient="horizontal",
    length=300,
    resolution=0.1,
    command=on_slider
)
slider.set(0)  # Posición inicial (centro)
slider.pack(pady=20)

ventana.mainloop()
