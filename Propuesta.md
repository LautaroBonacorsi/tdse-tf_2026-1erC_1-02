<img width="2027" height="647" alt="image" src="https://github.com/user-attachments/assets/8aec164a-7a4a-470e-9991-086afb1a2929" />

# Muestreo Multiparamétrico de Signos Vitales (MMSV)

**Autores: Bonacorsi, Lautaro Quimey; Bonfiglio, Guido Martin; Talarico, Jonatan Axel

**Padrón: 110115; 104884; 89396

**1er cuatrimestre 2026

## 1. Selección del proyecto a implementar
El proyecto consiste en un monitor de signos vitales portátil orientado al seguimiento de pacientes en tiempo real. El dispositivo medirá la saturación de oxígeno en sangre (SpO2), la frecuencia cardíaca y la temperatura corporal superficial. Contará con un sistema de alarmas médicas configurables, visualización local en un display LCD, y transmisión de telemetría constante hacia una estación central (App móvil) mediante Bluetooth.

### 1.1 Hardware a utilizar
* *Hardware Obligatorio:*
    * *Dip Switchs:* Selección del perfil de paciente al encender (ej. Adulto, Pediátrico, Geriátrico), lo que pre-carga distintos umbrales de alarma.
    * *Buttons:* Interacción con el menú local y silenciador de alarmas (Acknowledge).
    * *Sensor Analógico (MAX30102):* Sensor De Pulso Cardiaco Y Oxigeno Max30102.
    * *Leds y Buzzer:* Indicadores de alarmas médicas (amarillo para advertencia, rojo para estado crítico).
    * *Módulo HM-10:* Transmisión BLE continua de signos vitales a la App y recepción de configuraciones.
    * *Memoria E2PROM (I2C):* Almacenamiento del SET_UP (umbrales de alarma personalizados) y log de los últimos eventos críticos.
    * *Soporte:* Placa experimental perforada con componentes y conectores soldados, interconectados mediante alambre telefónico.
* *Hardware Adicional:*
    * *Display LCD 16x2 con módulo I2C:* Visualización del menú interactivo y signos vitales en tiempo real.

### 1.2 Programación a implementar
* *Arquitectura:* Bare Metal, Event-Triggered System (Super-Loop). Garantía de 1 vuelta < 1mS para no perder muestreos del sensor óptico.
* *Base de tiempo:* Tick de 1mS (Systick con Callbacks) para gestionar el refresco del LCD (no bloqueante, ej. cada 500ms) y el parpadeo de LEDs.
* *Máquina de Estados (Modos):*
    * INICIALIZACION: Chequeo del bus I2C y carga de umbrales desde E2PROM.
    * NORMAL: Muestreo de sensores, actualización de LCD y envío por BT.
    * SET_UP: Menú para ajustar límites de SpO2/BPM/Temp (vía LCD+Botones o Bluetooth).
    * ALARMA_MEDICA: Activación de rutinas de Buzzer/LED según prioridad (Tópico de no-bloqueo).
    * FALLA: Desconexión de sonda detectada (I2C timeout o lectura analógica a GND/VCC).
* *Gestión de Periféricos:*
    * El ADC para la temperatura funcionará por *DMA* o interrupciones. 
    * El sensor MAX30102 se gestionará a través de su pin de interrupción (INT), avisando al microcontrolador cuándo hay un nuevo dato en el FIFO del sensor, evitando el polling bloqueante por I2C.

---

## 2. Elicitación de requisitos y casos de uso

### Tabla de Requisitos

| ID  | Tipo | Descripción | Prioridad |
| --- | --- | --- | --- |
| REQ-01 | Funcional | El sistema debe medir SpO2, Frecuencia Cardíaca y Temperatura de forma continua. | Alta |
| REQ-02 | Funcional | El LCD debe mostrar los valores actuales y actualizarse sin frenar la lectura de los sensores. | Alta |
| REQ-03 | Funcional | Si el SpO2 cae por debajo del umbral guardado en la E2PROM, el sistema debe disparar una alarma crítica (LED rojo + Buzzer). | Alta |
| REQ-04 | Funcional | El usuario debe poder modificar los umbrales de alarma desde la App vía Bluetooth (HM-10). | Media |
| REQ-05 | No Funcional | El sistema debe manejar múltiples dispositivos esclavos (LCD, E2PROM, MAX30102) sobre el mismo bus I2C sin colisiones. | Alta |
| REQ-06 | Funcional | Si el bus I2C no detecta al MAX30102, el sistema debe entrar en modo FALLA y notificar el error en el LCD. | Alta |

### Casos de Uso

#### Caso de Uso 1: Detección y alerta de Hipoxia (SpO2 bajo)
* *Actor:* Sistema (Automático)
* *Precondición:* El sistema se encuentra en modo NORMAL monitoreando a un paciente.
* *Flujo principal:*
    1. El sensor MAX30102 calcula un valor de SpO2 del 88%.
    2. El super-loop compara este valor con el umbral mínimo (ej. 92%) almacenado en la E2PROM.
    3. El sistema detecta la anomalía y transiciona al modo ALARMA_MEDICA.
    4. Se inicia una secuencia de pitidos rápidos no bloqueantes en el Buzzer y parpadea un LED rojo.
    5. Se envía una trama de "ALERTA_CRITICA" por el HM-10 hacia la aplicación móvil.
    6. El sistema se mantiene en este estado hasta que el valor se normalice o un enfermero presione un botón de "Acknowledge" (silenciar).

#### Caso de Uso 2: Configuración de paciente y umbrales (SET_UP)
* *Actor:* Médico / Enfermero
* *Precondición:* El sistema está encendido y el HM-10 está emparejado.
* *Flujo principal:*
    1. El profesional de la salud envía un comando de configuración desde la App móvil.
    2. El sistema recibe el comando por UART e ingresa al modo SET_UP.
    3. La App envía los nuevos umbrales máximos y mínimos para Frecuencia Cardíaca y SpO2.
    4. El sistema valida los datos y los graba en la memoria E2PROM externa por I2C.
    5. El sistema actualiza el LCD confirmando la configuración y retorna al modo NORMAL.

#### Caso de Uso 3: Falla por desconexión de sensor (Sensor Off)
* *Actor:* Sistema (Automático)
* *Precondición:* El equipo está en modo NORMAL.
* *Flujo principal:*
    1. El paciente retira el dedo del sensor óptico bruscamente, o un cable I2C se desconecta.
    2. La rutina de lectura de registros por I2C falla (recibe un NACK o la lectura del sensor arroja valores nulos constantes).
    3. El sistema detecta el error y transiciona al modo FALLA.
    4. El LCD muestra el mensaje "ERR: SENSOR DESCONECTADO".
    5. Se activa un patrón de alarma técnica (LED amarillo fijo, pitido intermitente lento).
