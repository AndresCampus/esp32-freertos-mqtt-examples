# esp32-freertos-mqtt-examples
Ejemplos de ESP32 con FreeRTOS y MQTT: publicación periódica, temporizadores y colas para controlar un LED.

Este repositorio contiene **tres versiones progresivas** de un mismo ejercicio:  
la conexión de un ESP32 a WiFi y a un broker MQTT, la publicación periódica de mensajes y la recepción de órdenes para encender/apagar el LED integrado.  

El objetivo es **aprender cómo se usan las primitivas de FreeRTOS** (tareas, temporizadores, semáforos y colas) para estructurar aplicaciones IoT.

---

## 🔹 Requisitos

- **Placa ESP32** compatible con Arduino IDE.  
- **Broker MQTT** accesible (en los ejemplos se usa `iot.ac.uma.es`).  
- Librerías:
  - `WiFi.h`
  - [`PubSubClient`](https://pubsubclient.knolleary.net/)  

---

## 🔹 Versiones

### 1. Publicación periódica con `vTaskDelay`
- Se crean dos tareas principales:
  - `taskMQTTService`: conecta a WiFi y al broker, mantiene la conexión.
  - `taskPublisher`: espera al semáforo y publica un mensaje cada 30 segundos.
- La periodicidad se controla con `vTaskDelay()`.  
- **Conceptos FreeRTOS**: tareas, prioridades, semáforo binario.

👉 Es la versión más simple y sirve como introducción.

---

### 2. Publicación con Timer FreeRTOS
- Se reemplaza el bucle con `vTaskDelay()` por un **temporizador software**.  
- El callback del temporizador se ejecuta cada 30 segundos y publica el mensaje.  
- La tarea `taskPublisher` solo se encarga de crear el timer y luego se elimina.  
- **Conceptos FreeRTOS**: temporizadores, tarea `Tmr Svc`, callbacks.

👉 Buena para explicar la diferencia entre tareas periódicas y temporizadores.

---

### 3. Control de LED mediante cola
- El **callback de MQTT** no accede al hardware directamente.  
- En su lugar, produce un mensaje (`0`/`1`) en una **cola FreeRTOS**.  
- Una tarea dedicada (`taskLed`) recibe los mensajes y controla el LED.  
- **Conceptos FreeRTOS**: colas, productor/consumidor, desacoplo de responsabilidades.

👉 Es la versión más cercana a la filosofía de diseño de sistemas en tiempo real.

---

## 🔹 Comparativa rápida

| Versión | Periodicidad       | Sincronización inicial | Control del LED        | Concepto principal         |
|---------|-------------------|------------------------|------------------------|----------------------------|
| 1       | `vTaskDelay`      | Semáforo binario       | Desde callback MQTT    | Tareas + semáforo          |
| 2       | Timer FreeRTOS    | Semáforo binario       | Desde callback MQTT    | Temporizadores + Tmr Svc   |
| 3       | `vTaskDelay`      | Semáforo binario       | Cola + tarea dedicada  | Comunicación con colas     |

---

## 🔹 Ejecución

1. Abre cada sketch (`mqtt_FreeRTOS_v1.ino`, `mqtt_FreeRTOS_v2_timer.ino`, `mqtt_FreeRTOS_v3_cola.ino`) en Arduino IDE.  
2. Configura las credenciales WiFi y MQTT si fuera necesario.  
3. Carga en el ESP32 y abre el monitor serie (`115200 baud`).  

---

## 🔹 Objetivos de aprendizaje

- **Crear y gestionar tareas** en FreeRTOS.  
- **Sincronizar tareas** con semáforos.  
- **Usar temporizadores software** para tareas periódicas.  
- **Comunicar tareas** mediante colas.  
- Entender cómo estructurar aplicaciones IoT **robustas y mantenibles**.  

---

## 📂 Estructura del repo
/

├── mqtt_FreeRTOS.ino       # Versión con vTaskDelay

├── mqtt_FreeRTOS_timer.ino # Versión con timer

├── mqtt_FreeRTOS_cola.ino  # Versión con cola

└── README.md

