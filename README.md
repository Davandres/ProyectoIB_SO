# Sistema de Sensores Inteligentes – Procesamiento Paralelo con Hilos en C

Este proyecto simula un **sistema de monitoreo ambiental** mediante sensores inteligentes que procesan datos de forma **concurrente** usando hilos (`pthread`) en C. Fue desarrollado como parte de la asignatura **Sistemas Operativos** en la Escuela Politécnica Nacional.

## 📌 Descripción

El programa modela cuatro sensores ambientales:
- **Temperatura** (15°C – 45°C) → Alerta si > 35°C  
- **Humedad** (20% – 95%) → Alerta si < 30% o > 80%  
- **Ruido** (30 dB – 120 dB) → Alerta si > 90 dB  
- **Luminosidad** (100 – 1000 lux) → Alerta si < 150 lux  

Cada sensor se ejecuta en su **propio hilo**, generando **1000 lecturas simuladas** (transacciones) y registrando alertas en un archivo con **timestamp preciso**.

## ✅ Características

- Procesamiento paralelo con `pthread`
- Sincronización mediante **mutex** para:
  - Proteger el contador global de alertas
  - Evitar corrupción en el archivo de log
- Medición de tiempos con `clock_gettime()` (precisión nanosegundos)
- Comparación de rendimiento: **versión paralela vs secuencial**
- Simulación realista de un sistema IoT concurrente

## 📊 Resultados esperados

| Versión       | Tiempo (aprox.) | Uso de CPU | Aceleración |
|---------------|------------------|------------|-------------|
| Secuencial    | ~0.42 s          | ~100%      | 1.0×        |
| Paralela (4H) | ~0.11 s          | ~380–400%  | ~3.8×       |

> Los valores exactos dependerán del hardware, pero siempre se observa una clara mejora en rendimiento.

## ▶️ Cómo compilar y ejecutar

```bash
# 1. Clonar el repositorio (si aplica)
git clone <tu-repositorio>
cd proyecto-sensores-paralelo

# 2. Compilar
gcc -o sensores proyectoIBB.c -lpthread -lrt

# 3. Ejecutar
./sensores
