 # **Monitor Multiparamétrico de Signos Vitales (MMSV)**
 ## **Informe de Avances**

**Autor**

|Autores|Padrón|
| :--- | :--- |
| BONACORSI, Lautaro Quimey | 110115 | 
| BONFIGLIO, Guido Martin | 104884 |
| TALARICO, Jonatan Axel | 89396 |

*Fecha: 12/06/2025*

*1er cuatrimestre 2026*

---

A continuación se detalla el informe de avances del TP4 a partir de los requerimientos

| Estado | Descripción      |
|-----|---------------------|
| 🟢 | Ya implementado |
| 🟡 | En proceso de implementarse |
| 🔴 | Aún no implementado |

| Grupo | ID | Descripción | Estado |
| :---- | :---- | :---- | :--- |
| **Sensores** | 1.1 | El sistema debe medir SpO2 y Frecuencia Cardíaca de forma continua. | 🟡 |
| **Interfaz Local** | 2.1 | El LCD debe mostrar los valores actuales y actualizarse sin frenar la lectura de los sensores. | 🟡 |
|  | 2.2 | En el menu se podra cambiar los umbrales máximos y mínimos. | 🟢 | 
|  | 2.3 | El menu se controlara con cuatro botones (pulsadores). | 🟢 |
|  | 2.4 | Se cargara el perfil del paciente predeterminado (valores umbrales) según la tecla del dip switch activa. | 🟢 |
| **Seguridad** | 3.1 | Si el SpO2 cae por debajo o supera el umbral guardado en la E2PROM, el sistema debe disparar una alarma crítica (LED rojo + Buzzer). | 🟡 |
| | 3.2 | Si el bus I2C no detecta al MAX30102, el sistema debe entrar en modo FALLA de forma segura y notificar el error en el LCD. | 🟡 |
| **Comunicaciones**| 4.1 | El usuario debe poder modificar los umbrales de alarma desde la App vía Bluetooth (HM-10). | 🔴 |
| **Hardware** | 5.1 | El sistema debe manejar múltiples dispositivos esclavos (LCD, EEPROM, MAX30102) sobre el mismo bus I2C sin colisiones. | 🔴 |
|   | 5.2 | Los valores de umbrales predeterminados (pediatrico, adulto, geriatrico) estaran guardados en EEPROM | 🔴 | 
