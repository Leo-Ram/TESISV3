import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

from pandas import json_normalize
os.system('cls' if os.name == 'nt' else 'clear')  

direccion = "databms/datos3d"

nameColumn = ["B1", "B2", "B3", "B4", "B5", "B6", "I","T","VT","TIME"]
df = pd.read_csv(direccion+".txt",delimiter=',',names=nameColumn)



df['TIME'] = pd.to_datetime(df['TIME'])

import pandas as pd
from datetime import datetime, timedelta

def correct_time_resets(df, time_column='TIME'):
    """
    Corrige los reinicios en una columna de tiempo, creando una secuencia continua
    que respeta los intervalos originales entre registros.
    
    Args:
        df: DataFrame con los datos
        time_column: Nombre de la columna que contiene los timestamps
    
    Returns:
        DataFrame con la columna de tiempo corregida
    """
    # Convertir la columna de tiempo a datetime si no lo está ya
    df['timestamp'] = pd.to_datetime(df[time_column])
    
    # Calcular diferencias entre timestamps consecutivos
    time_diff = df['timestamp'].diff()
    
    # Detectar reinicios (cuando la diferencia es negativa)
    resets = time_diff < timedelta(0)
    
    # Calcular el offset acumulativo para cada segmento
    cumulative_offset = pd.Timedelta(0)
    corrected_times = df['timestamp'].copy()
    
    for i in range(1, len(df)):
        if resets[i]:
            # Cuando hay un reinicio, calcular cuánto tiempo debería haber pasado
            # basado en el intervalo típico entre registros
            typical_interval = time_diff[~resets].median()
            cumulative_offset += abs(time_diff[i]) + typical_interval
        
        corrected_times[i] = df['timestamp'][i] + cumulative_offset
    
    # Crear nueva columna con los tiempos corregidos
    df['corrected_time'] = corrected_times
    
    return df

# Ejemplo de uso:

# Asumiendo que tus datos están en un DataFrame llamado 'df':
df = correct_time_resets(df)
#print(df[['TIME', 'corrected_time']])  # Comparar columnas original y corregida
