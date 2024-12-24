import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

from pandas import json_normalize
os.system('cls' if os.name == 'nt' else 'clear')  

direccion = "databms/datos2p"

df = pd.read_csv(direccion+".csv")
df = df[df['I'] > -200]
print(df.head(10))


df['TIME'] = pd.to_datetime(df['TIME'])

start_time = pd.to_datetime('2024-01-01 00:00:00')
end_time = pd.to_datetime('2024-01-01 04:06:00')
#mask = (df['TIME'] >= start_time) & (df['TIME'] <= end_time)
#df = df.loc[mask]

fig, axs = plt.subplots(3, 1, figsize=(10,40), sharex="col")
fig.suptitle("Cliclo de carga controlado a 500mA",fontsize=20)


axs[0].plot(df['TIME'],df["VT"])
axs[0].set_ylabel("Voltaje[V]",fontsize=20)
axs[0].grid(True)

axs[1].plot(df['TIME'],df["I"])
axs[1].set_ylabel("Corriente[mA]",fontsize=20)
axs[1].grid(True)

axs[2].plot(df['TIME'],df["PVT"])
axs[2].set_ylabel("Potencia[W]",fontsize=20)
axs[2].grid(True)
axs[2].xaxis.set_major_formatter(mdates.DateFormatter('%H:%M'))
axs[2].xaxis.set_major_locator(mdates.AutoDateLocator())
plt.setp(axs[2].xaxis.get_majorticklabels(), rotation=45, ha='right')
axs[2].set_xlabel('Tiempo (HH:MM)',fontsize=20)

# Ajustar el espacio entre subplots
plt.subplots_adjust(left=0.14, right=0.94, top=0.928, bottom=0.11, hspace=0.03)

plt.show()