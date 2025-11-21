#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#define NUM_SENSORES 4
#define LECTURAS_POR_SENSOR 1000  // Número de transacciones por sensor

// Tipos de sensores
typedef enum {
    TEMPERATURA,
    HUMEDAD,
    RUIDO,
    LUMINOSIDAD
} TipoSensor;

// Estructura para pasar datos al hilo
typedef struct {
    TipoSensor tipo;
    int id;
} SensorArgs;

// Variable global compartida: conteo de alertas (protegida por mutex)
int alertas_totales = 0;
pthread_mutex_t mutex_alertas = PTHREAD_MUTEX_INITIALIZER;

// Archivo global para log (opcional: también protegido si se escribe en paralelo)
FILE *log_file;
pthread_mutex_t mutex_log = PTHREAD_MUTEX_INITIALIZER;
// Genera un número aleatorio en [min, max]
double rand_range(double min, double max) {
    return min + ((double)rand() / RAND_MAX) * (max - min);
}

// Verifica si un valor activa una alerta
int verificar_alerta(TipoSensor tipo, double valor) {
    switch (tipo) {
        case TEMPERATURA: return valor > 35.0;
        case HUMEDAD:     return valor < 30.0 || valor > 80.0;
        case RUIDO:       return valor > 90.0;
        case LUMINOSIDAD: return valor < 150.0;
        default:          return 0;
    }
}

// btiene el nombre del sensor
const char* nombre_sensor(TipoSensor tipo) {
    switch (tipo) {
        case TEMPERATURA: return "Temperatura";
        case HUMEDAD:     return "Humedad";
        case RUIDO:       return "Ruido";
        case LUMINOSIDAD: return "Luminosidad";
        default:          return "Desconocido";
    }
}


void* hilo_sensor(void* arg) {
    SensorArgs* args = (SensorArgs*)arg;
    TipoSensor tipo = args->tipo;
    int id = args->id;

    int alertas_locales = 0;

    for (int i = 0; i < LECTURAS_POR_SENSOR; i++) {
        double valor;
        switch (tipo) {
            case TEMPERATURA: valor = rand_range(15.0, 45.0); break;
            case HUMEDAD:     valor = rand_range(20.0, 95.0); break;
            case RUIDO:       valor = rand_range(30.0, 120.0); break;
            case LUMINOSIDAD: valor = rand_range(100.0, 1000.0); break;
            default: valor = 0;
        }

        if (verificar_alerta(tipo, valor)) {
            alertas_locales++;

            // Obtener timestamp preciso
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);

            // Escribir en log con protección de mutex
            pthread_mutex_lock(&mutex_log);
            fprintf(log_file, "[ALERTA] %s (Hilo %d): %.2f @ %ld.%09ld\n",
                    nombre_sensor(tipo), id, valor, ts.tv_sec, ts.tv_nsec);
            fflush(log_file);
            pthread_mutex_unlock(&mutex_log);
        }

        usleep(100); // Simular procesamiento
    }

    // Actualizar contador global de alertas
   // pthread_mutex_lock(&mutex_alertas);
    alertas_totales += alertas_locales;
   // pthread_mutex_unlock(&mutex_alertas);

    printf("[Sensor %s %d] Finalizado. Alertas: %d\n", nombre_sensor(tipo), id, alertas_locales);
    return NULL;
} 


void ejecutar_secuencial() {
    srand(12345); // Semilla fija para comparabilidad

    FILE *log_sec = fopen("alertas_secuencial.log", "w");
    if (!log_sec) {
        perror("No se pudo crear alertas_secuencial.log");
        return;
    }

    int alertas_totales_sec = 0;

    TipoSensor tipos[] = {TEMPERATURA, HUMEDAD, RUIDO, LUMINOSIDAD};
    const char* nombres[] = {"Temperatura", "Humedad", "Ruido", "Luminosidad"};

    for (int s = 0; s < NUM_SENSORES; s++) {
        int alertas_sensor = 0;
        for (int i = 0; i < LECTURAS_POR_SENSOR; i++) {
            double valor;
            switch (tipos[s]) {
                case TEMPERATURA: valor = rand_range(15.0, 45.0); break;
                case HUMEDAD:     valor = rand_range(20.0, 95.0); break;
                case RUIDO:       valor = rand_range(30.0, 120.0); break;
                case LUMINOSIDAD: valor = rand_range(100.0, 1000.0); break;
                default: valor = 0;
            }

            if (verificar_alerta(tipos[s], valor)) {
                alertas_sensor++;
                fprintf(log_sec, "[ALERTA SEC] %s: %.2f\n", nombres[s], valor);
            }
            usleep(100); // Simular mismo retardo que en paralelo
        }
        alertas_totales_sec += alertas_sensor;
        printf("[Secuencial] %s finalizado. Alertas: %d\n", nombres[s], alertas_sensor);
    }

    fclose(log_sec);
    printf("\n[Secuencial] Alertas totales: %d\n", alertas_totales_sec);
}


int main(int argc, char *argv[]) {
    srand(time(NULL));

    log_file = fopen("alertas.log", "w");
    if (!log_file) {
        perror("No se pudo crear alertas.log");
        return 1;
    }

    struct timespec inicio, fin;
    double tiempo_paralelo, tiempo_secuencial;

    // === Medición: Versión secuencial ===
    printf("Ejecutando versión secuencial...\n");
    clock_gettime(CLOCK_MONOTONIC, &inicio);
    ejecutar_secuencial(); // Esta función ya imprime su tiempo interno, pero la envolvemos
    clock_gettime(CLOCK_MONOTONIC, &fin);
    tiempo_secuencial = (fin.tv_sec - inicio.tv_sec) + (fin.tv_nsec - inicio.tv_nsec) / 1e9;

    // Reiniciar semilla y archivo para paralelo (opcional, pero consistente)
    srand(time(NULL));
    fclose(log_file);
    log_file = fopen("alertas.log", "w");

    // === Medición: Versión paralela ===
    printf("\nEjecutando versión paralela...\n");
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    pthread_t threads[NUM_SENSORES];
    SensorArgs args[NUM_SENSORES] = {
        {TEMPERATURA, 0},
        {HUMEDAD, 1},
        {RUIDO, 2},
        {LUMINOSIDAD, 3}
    };

    for (int i = 0; i < NUM_SENSORES; i++) {
        pthread_create(&threads[i], NULL, hilo_sensor, &args[i]);
    }
    for (int i = 0; i < NUM_SENSORES; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &fin);
    tiempo_paralelo = (fin.tv_sec - inicio.tv_sec) + (fin.tv_nsec - inicio.tv_nsec) / 1e9;

    // === Resultados finales ===
    printf("==================================================\n");
    printf("RESULTADOS DE TIEMPO:\n");
    printf("Secuencial: %.4f segundos\n", tiempo_secuencial);
    printf("Paralelo:   %.4f segundos\n", tiempo_paralelo);
    printf("Aceleración: %.2fx\n", tiempo_secuencial / tiempo_paralelo);
    printf("Eficiencia: %.2f%%\n", (tiempo_secuencial / tiempo_paralelo) / NUM_SENSORES * 100);
    printf("==================================================\n");

    fclose(log_file);
    pthread_mutex_destroy(&mutex_alertas);
    pthread_mutex_destroy(&mutex_log);

    return 0;
}
