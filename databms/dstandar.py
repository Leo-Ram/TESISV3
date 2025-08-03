
from datetime import datetime
import os
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import numpy as np

from pandas import json_normalize
os.system('cls' if os.name == 'nt' else 'clear')  

direccion = "datossd/2024-10-20-data1p"

df = pd.read_csv(direccion+".csv")
df = df[df['I'] > -200]
#print(df.head(10))


df['TIME'] = pd.to_datetime(df['TIME'])


def analyze_battery_stats(df):

    
    # Crear columna de estado (carga/descarga)
    df['Estado'] = np.where(df['I'] > 0, 'Carga', 'Descarga')
    
    # Función para calcular la desviación estándar en ventanas de tiempo
    def calculate_rolling_std(group):
        # Ordenar por tiempo
        group = group.sort_values('TIME')
        
        # Calcular la diferencia de voltaje respecto al valor anterior
        voltage_cols = [f'B{i}' for i in range(1, 7)]
        diff_cols = {}
        
        for col in voltage_cols:
            # Calcular diferencias respecto al valor anterior
            diffs = group[col].diff()
            # Calcular desviación estándar de las diferencias
            std_diff = diffs.std()
            diff_cols[f'{col}_std'] = std_diff
            
        # Añadir desviación estándar del voltaje total
        diff_cols['VT_std'] = group['VT'].diff().std()
        # Añadir desviación estándar de la corriente
        diff_cols['I_std'] = group['I'].diff().std()
        
        # Añadir estadísticas adicionales
        diff_cols['duracion_minutos'] = (group['TIME'].max() - group['TIME'].min()).total_seconds() / 60
        diff_cols['muestras'] = len(group)
        diff_cols['corriente_promedio'] = group['I'].mean()
        
        return pd.Series(diff_cols)

    # Calcular estadísticas por estado
    stats_by_state = df.groupby('Estado').apply(calculate_rolling_std)
    
    return stats_by_state

# Ejemplo de uso con tus datos
# Asumiendo que los datos están en un DataFrame llamado 'df'
def print_analysis(stats):
    """
    Imprime el análisis de manera formateada
    """
    for estado in stats.index:
        print(f"\n=== Estadísticas para {estado} ===")
        print(f"Duración: {stats.loc[estado, 'duracion_minutos']:.2f} minutos")
        print(f"Número de muestras: {stats.loc[estado, 'muestras']}")
        print(f"Corriente promedio: {stats.loc[estado, 'corriente_promedio']:.2f} A")
        print("\nDesviación estándar de cambios en voltajes:")
        
        for i in range(1, 7):
            print(f"Batería {i}: {stats.loc[estado, f'B{i}_std']:.4f} V")
            
        print(f"Voltaje Total: {stats.loc[estado, 'VT_std']:.4f} V")
        print(f"Corriente: {stats.loc[estado, 'I_std']:.4f} A")

# Uso del código:
# Asumiendo que tus datos están en un DataFrame llamado 'df':
stats = analyze_battery_stats(df)
print_analysis(stats)