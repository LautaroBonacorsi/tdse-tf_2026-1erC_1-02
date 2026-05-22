<p align="center">
    <img width="50%" alt="logo_fiuba" src="https://www.fi.uba.ar/images/logo-fiuba.png" />
</p>

# **Monitor Multiparamétrico de Signos Vitales (MMSV)**

|Autores|Padrón|
| :--- | :--- |
| BONACORSI, Lautaro Quimey | 110115 | 
| BONFIGLIO, Guido Martin | 104884 |
| TALARICO, Jonatan Axel | 89396 |

**Fecha: 1er cuatrimestre 2026**

---

## **1. Selección del proyecto a implementar**

#### **1.1 Objetivo del proyecto y resultados esperados**
El objetivo de este proyecto es diseñar e implementar un monitor de signos vitales portátil orientado al seguimiento de pacientes en tiempo real. El dispositivo medirá la saturación de oxígeno en sangre (SpO2) y la frecuencia cardíaca. Contará con un sistema de alarmas médicas configurables, visualización local en un display LCD, y transmisión de telemetría constante hacia una estación central (App móvil) mediante Bluetooth. Dado que se trata de un dispositivo orientado a la medicina, su diseño sentará las bases conceptuales para una futura alineación con normativas exigidas por ANMAT, tales como las normas IEC 60601-1 (seguridad electromédica), ISO 80601-2-61 (pulsioximetría) e IEC 62304 (software médico seguro).

### **1.2 Proyectos Similares**
Para alcanzar los objetivos del proyecto, se plantearon tres alternativas viables de monitoreo clínico, todas basadas en una arquitectura de telemetría embebida:

1. **Monitor Base (SpO2 + Frecuencia Cardíaca):** Monitoreo continuo mediante sensado óptico (I2C) con alarmas y telemetría Bluetooth.
2. **Monitor Base + Electrocardiograma (ECG):** Se añade un front-end analógico (ej. módulo AD8232) para graficar la actividad eléctrica del corazón.
3. **Monitor Base + Presión Arterial No Invasiva (NIBP):** Se añaden bombas de aire, válvulas solenoides y sensores de presión para inflar un manguito de forma automática.

Para comparar estas alternativas, se evaluaron seis aspectos característicos ponderados del 1 al 10:
1. **Disponibilidad del hardware (10):** Accesibilidad de los componentes en el mercado regional.
2. **Impacto en el proyecto (8):** Funcionalidad y valor agregado al monitoreo del paciente.
3. **Costo (8):** Inversión requerida para el hardware.
4. **Dificultad técnica / Viabilidad (10):** Complejidad de integración manteniendo el ciclo de ejecución Bare Metal < 1ms exigido.
5. **Interés personal (8):** Motivación del equipo para desarrollar la tecnología.

La siguiente tabla (Tabla 1.2.1) muestra los valores ponderados asignados a cada proyecto considerado:

<table>
    <thead>
        <tr>
            <th rowspan="2">Criterio</th>
            <th colspan="2">1. Monitor Base (SpO2 + FC)</th>
            <th colspan="2">2. Base + ECG</th>
            <th colspan="2">3. Base + NIBP (Presión)</th>
        </tr>
        <tr>
            <th>Puntaje</th>
            <th>Puntaje Ponderado</th>
            <th>Puntaje</th>
            <th>Puntaje Ponderado</th>
            <th>Puntaje</th>
            <th>Puntaje Ponderado</th>
        </tr>
    </thead>
    <tbody>
        <tr class="header-row">
            <td align="center">Disponibilidad de Hardware <br>(peso: 10)</td>
            <td>10</td>
            <td>100</td>
            <td>6</td>
            <td>60</td>
            <td>3</td>
            <td>30</td>
        </tr>
        <tr>
            <td align="center">Impacto en el proyecto <br>(peso: 8)</td>
            <td>8</td>
            <td>64</td>
            <td>10</td>
            <td>80</td>
            <td>10</td>
            <td>80</td>
        </tr>
        <tr class="header-row">
            <td align="center">Costo (peso: 8)</td>
            <td>9</td>
            <td>72</td>
            <td>6</td>
            <td>48</td>
            <td>3</td>
            <td>24</td>
        </tr>
        <tr>
            <td align="center">Dificultad técnica / Viabilidad <br>(peso: 10)</td>
            <td>9</td>
            <td>90</td>
            <td>6</td>
            <td>60</td>
            <td>2</td>
            <td>20</td>
        </tr>
        <tr class="header-row">
            <td align="center">Interés personal <br>(peso: 8)</td>
            <td>10</td>
            <td>80</td>
            <td>9</td>
            <td>72</td>
            <td>8</td>
            <td>64</td>
        </tr>
        <tr class="highlight-green">
            <td><strong>Puntaje Total</strong></td>
            <td>-</td>
            <td><strong>406</strong></td>
            <td>-</td>
            <td>320</td>
            <td>-</td>
            <td class="highlight-red">218</td>
        </tr>
    </tbody>
</table>
<p align="center"><em>Tabla 1.2.1: Comparación de alternativas de proyecto</em></p>

#### **1.3 Selección de proyecto**
Considerando los resultados de la Tabla 1.2.1, se decide implementar el **Monitor Base (SpO2 + Frecuencia Cardíaca)**. La opción con NIBP fue descartada debido a su altísimo costo y la dificultad de implementar dispositivos neumáticos. La opción con ECG es muy interesante, pero requiere un procesamiento de señales digitales (DSP) muy demandante que pondría en riesgo el requisito fundamental de mantener el Super-Loop por debajo de 1mS de ejecución.

El proyecto Base garantiza un sistema robusto, con un tiempo de implementación acorde al cuatrimestre, excelente disponibilidad de hardware regional y el desafío sustancial de gestionar múltiples periféricos no bloqueantes en un mismo bus I2C.
    
### **1.1 Hardware y Programación a implementar**
- **Hardware Obligatorio / Adicional:**
    - **Dip Switchs:** Selección del perfil de paciente al encender (ej. Adulto, Pediátrico, Geriátrico), pre-cargando distintos umbrales de alarma.
    - **Buttons:** Interacción con el menú local y silenciador de alarmas (Acknowledge).
    - **Sensor Analógico (MAX30102):** Sensor De Pulso Cardiaco Y Oxigeno por I2C.
    - **Leds y Buzzer:** Indicadores de alarmas médicas (amarillo para advertencia, rojo para estado crítico).
    - **Módulo HM-10:** Transmisión BLE continua de signos vitales a la App y recepción de configuraciones.
    - **Memoria EEPROM (I2C):** Almacenamiento del SET_UP (umbrales de alarma personalizados) y log de los últimos eventos críticos.
    - **Display LCD 16x2 (I2C):** Visualización del menú interactivo y signos vitales en tiempo real.
- **Arquitectura de Software:**
    - Bare Metal, Event-Triggered System (Super-Loop < 1ms). Base de tiempo Systick de 1mS con Callbacks para gestionar el refresco del LCD (no bloqueante) y el parpadeo de LEDs. El sensor MAX30102 se gestionará a través de su pin de interrupción (INT), avisando al microcontrolador cuándo hay un nuevo dato en el FIFO del sensor, evitando el polling bloqueante por I2C.
- **Máquina de Estados:**
    - INICIALIZACION: Chequeo del bus I2C y carga de umbrales desde EEPROM.
    - NORMAL: Muestreo de sensores, actualización de LCD y envío por BT.
    - SET_UP: Menú para ajustar límites de SpO2/BPM (vía LCD+Botones o Bluetooth).
    - ALARMA_MEDICA: Activación de rutinas de Buzzer/LED según prioridad (Tópico de no-bloqueo).
    - FALLA: Desconexión de sonda detectada (I2C timeout).

###### **1.3.1 Diagrama en bloques**
<p align="center">
    <img width="627" height="341" alt="Image" src="https://github.com/user-attachments/assets/25a6153c-a059-418d-8077-646bc64b4445" />
</p>
<p align="center"><em>Figura 1.3.1: Diagrama en bloques del sistema</em></p>

---

## 2. Elicitación de requisitos y casos de uso

En Argentina y en el mercado general, los oxímetros de pulso (tipo pinza de dedo) son extremadamente comunes y económicos. Sin embargo, estos dispositivos comerciales suelen ser unidades aisladas sin telemetría ni un historial de eventos. Por otro lado, los monitores multiparamétricos de terapia intensiva cuestan miles de dólares. 

Este proyecto se ubica en un nicho intermedio: un dispositivo económico de sala o internación general que no solo mide, sino que centraliza la información a través de telemetría IoT (Bluetooth), alerta activamente a la guardia mediante una app, y cuenta con una arquitectura a prueba de fallos (estado de FALLA) para seguridad del paciente.

| Grupo | ID | Descripción | Prioridad |
| :---- | :---- | :---- | :--- |
| **Sensores** | 1.1 | El sistema debe medir SpO2 y Frecuencia Cardíaca de forma continua. | Alta |
| **Interfaz Local** | 2.1 | El LCD debe mostrar los valores actuales y actualizarse sin frenar la lectura de los sensores. | Alta |
| **Seguridad** | 3.1 | Si el SpO2 cae por debajo del umbral guardado en la E2PROM, el sistema debe disparar una alarma crítica (LED rojo + Buzzer). | Alta |
| | 3.2 | Si el bus I2C no detecta al MAX30102, el sistema debe entrar en modo FALLA de forma segura y notificar el error en el LCD. | Alta |
| **Comunicaciones**| 4.1 | El usuario debe poder modificar los umbrales de alarma desde la App vía Bluetooth (HM-10). | Media |
| **Hardware** | 5.1 | El sistema debe manejar múltiples dispositivos esclavos (LCD, EEPROM, MAX30102) sobre el mismo bus I2C sin colisiones. | Alta |

<p align="center"><em>Tabla 2.1: Requisitos del proyecto</em></p>

A continuación se presentan los casos de uso principales para el sistema:

| Elemento | Definición |
| :---- | :---- |
| **Disparador** | El sistema calcula que el SpO2 está por debajo del límite seguro. |
| **Precondiciones** | El sistema se encuentra en el estado NORMAL monitoreando a un paciente. |
| **Flujo principal** | 1. El sensor MAX30102 calcula un valor de SpO2 del 88%.<br>2. El super-loop compara este valor con el umbral mínimo almacenado en la E2PROM.<br>3. El sistema detecta la anomalía y transiciona al estado ALARMA_MEDICA.<br>4. Se inicia una secuencia de pitidos rápidos no bloqueantes en el Buzzer y parpadea un LED rojo.<br>5. Se envía una trama de "ALERTA_CRITICA" por el HM-10 hacia la aplicación móvil.<br>6. El sistema se mantiene en este estado hasta que el valor se normalice o un enfermero presione un botón de "Acknowledge" (silenciar). |
| **Flujos alternativos** | a. El paciente se recupera rápidamente y el SpO2 vuelve a niveles normales antes de que el enfermero presione el botón: el sistema cesa la alarma y retorna al estado NORMAL automáticamente. |

<p align="center"><em>Tabla 2.2: Caso de uso 1 - Detección y alerta de Hipoxia (SpO2 bajo)</em></p>


| Elemento | Definición |
| :---- | :---- |
| **Disparador** | El profesional de salud decide modificar los límites de alarma del equipo. |
| **Precondiciones** | El sistema está encendido y en estado NORMAL. El módulo HM-10 está emparejado con la App móvil. |
| **Flujo principal** | 1. El profesional de la salud envía un comando de configuración desde la App móvil.<br>2. El sistema recibe el comando por UART e ingresa al estado SET_UP.<br>3. La App envía los nuevos umbrales máximos y mínimos para Frecuencia Cardíaca y SpO2.<br>4. El sistema valida los datos y los graba en la memoria EEPROM externa por I2C.<br>5. El sistema actualiza el LCD confirmando la configuración y retorna al estado NORMAL. |
| **Flujos alternativos** | a. Se pierde la conexión Bluetooth durante el proceso: el sistema descarta los datos parciales, mantiene los umbrales antiguos y retorna al estado NORMAL. |

<p align="center"><em>Tabla 2.3: Caso de uso 2 - Configuración de umbrales (SET_UP)</em></p>


| Elemento | Definición |
| :---- | :---- |
| **Disparador** | Falla de hardware o desconexión del paciente. |
| **Precondiciones** | El equipo está operando en el estado NORMAL. |
| **Flujo principal** | 1. El paciente retira el dedo del sensor óptico bruscamente, o el cable I2C del sensor se desconecta.<br>2. La rutina de lectura de registros por I2C falla (se recibe un NACK o lectura nula constante).<br>3. El sistema aborta el muestreo, detecta el error crítico y transiciona al estado FALLA.<br>4. El LCD muestra el mensaje "ERR: SENSOR DESCONECTADO".<br>5. Se activa un patrón de alarma técnica (LED amarillo fijo, pitido intermitente lento). |
| **Flujos alternativos** | a. El sensor vuelve a conectarse o se reposiciona el dedo: el sistema requiere un reset físico o presionar un botón de reinicio para salir del modo de enclavamiento por seguridad. |

<p align="center"><em>Tabla 2.4: Caso de uso 3 - Falla por desconexión de sensor (Sensor Off)</em></p>
