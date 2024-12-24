import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

from pandas import json_normalize
os.system('cls' if os.name == 'nt' else 'clear')  

direccion = "datarp/lecturas2p"

df = pd.read_csv(direccion+".csv")

print(df.head(10))


df['TIME'] = pd.to_datetime(df['TIME'])

# Definir el intervalo de tiempo (ajusta estos valores según tus necesidades)
#start_time = pd.to_datetime('2024-01-01 00:35:00')
#end_time = pd.to_datetime('2024-01-01 00:40:00')

# Filtrar el DataFrame por el intervalo de tiempo especificado
#mask = (df['TIME'] >= start_time) & (df['TIME'] <= end_time)
#df = df.loc[mask]


# Crear una figura con un arreglo de 3 filas y 1 columna
fig, axs = plt.subplots(2, 1, figsize=(10, 40), sharex=True)
fig.suptitle('Corriente y Potencia vs Tiempo', fontsize=20)

# Primera gráfica: Voltajes VF, VB, VC contra tiempo
#axs[0].plot(df['TIME'], df['VF'], label='Fuente')
#axs[0].plot(df['TIME'], df['VB'], label='Baterias')
#axs[0].plot(df['TIME'], df['VC'], label='Carga')
#axs[0].set_ylabel('Voltaje [V]',fontsize=20)
#axs[0].legend(loc='upper right')
#axs[0].grid(True)

# Segunda gráfica: Corrientes IF, IB, IC contra tiempo
axs[0].plot(df['TIME'], df['IMPPT'], label='Fuente')
axs[0].plot(df['TIME'], df['IL'], label='Carga')
axs[0].plot(df['TIME'], df['IBAT'], label='Baterías')
axs[0].set_ylabel('Corriente [mA]',fontsize=20)
axs[0].legend(loc='upper right')
axs[0].grid(True)

# Tercera gráfica: Potencias PVF, PVB, PVC contra tiempo
axs[1].plot(df['TIME'], df['PMPPT'], label='Fuente')
axs[1].plot(df['TIME'], df['PL'], label='Carga')
axs[1].plot(df['TIME'], df['PBAT'], label='Baterías')
axs[1].set_ylabel('Potencia [W]',fontsize=20)
axs[1].legend(loc='upper right')
axs[1].grid(True)

# Formatear el eje X (Tiempo)
axs[1].xaxis.set_major_formatter(mdates.DateFormatter('%H:%M'))
axs[1].xaxis.set_major_locator(mdates.AutoDateLocator())
plt.setp(axs[1].xaxis.get_majorticklabels(), rotation=45, ha='right')
axs[1].set_xlabel('Tiempo (HH:MM)',fontsize=20)

# Ajustar el espacio entre subplots
plt.subplots_adjust(left=0.14, right=0.94, top=0.928, bottom=0.11, hspace=0.03)

# Mostrar las gráficas
plt.show()
