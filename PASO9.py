import matplotlib.pyplot as plt
import random
import serial

plt.ion()
plt.axis([0,100,0,100])

for i in range (100):
   x= range (i)
   y= range (i)
   plt.plot (x,y)
   plt.title (str(i))
   plt.draw()
   plt.pause (0.5)

plt.show (block=True)

