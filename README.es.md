[🇺🇸 Inglés](README.md)  [🇨🇳 Chino](README.zh.md)  [🇷🇺 Ruso](README.ru.md)  [🇮🇳 Hindi](README.hi.md)  [🇪🇸 Español](README.es.md)
<p align="center">
<img width="200" src="https://github.com/hack-tramp/WNetWrap/blob/main/logo.png?raw=true">
<h1 align="center">ESP-MQTTunnel <br>
<img align="center" src="https://img.shields.io/badge/c%2B%2B-11-blue" href="https://github.com/topics/c-plus-plus-11">
<img align="center" src="https://img.shields.io/badge/License-MIT-green.svg" href="https://opensource.org/licenses/MIT">
<img align="center" src="https://www.ardu-badge.com/badge/MQTT.svg" href="https://mqtt.org">
<img align="center" src="https://www.ardu-badge.com/badge/HttpClient.svg" href="https://arduino.cc">
</h1>
</p>





Una prueba de concepto funcional que convierte un ESP32 en un proxy de hardware de 3 dólares capaz de tunelizar tráfico a través de MQTT. Dado que se conecta a un *broker* (intermediario) externo, permite eludir la censura sin necesidad de abrir puertos ni contar con una dirección IP pública.

---

## ¿Qué hace?

El ESP32 actúa como el punto final TCP real. El script de Python funciona como un proxy local en el ordenador portátil. MQTT sirve simplemente como el canal de comunicación entre ambos. ![Texto alternativo](esp-mqtt-diag.svg)
Probado con: Windows 10 Firefox / Python 3 <--> ESP32-S3 Dev Module <--> HiveMQ Cloud (nivel gratuito)
> ⚠️ El *broker* MQTT debe ser accesible desde el país donde se encuentre el portátil.
---

## Funcionamiento confirmado

- ✅ YouTube
- ✅ Gmail
- ✅ Twitter / X
- ✅ Sitios de noticias
- ✅ Reddit

## Problemas conocidos

- ❌ Instagram se bloquea
---
## Cómo funciona

1. El **ESP32** se conecta a tu Wi-Fi y se suscribe al tema (topic) MQTT `req`.
2. El **script de Python** ejecuta un proxy HTTP local en el portátil (p. ej., `127.0.0.1:8080`).
3. Firefox se configura para utilizar dicho proxy local.
4. Cuando Firefox solicita un sitio, el proxy de Python:
- Analiza la línea `CONNECT host:port`. 
- Publica un mensaje `open` a través de MQTT. 
- Transmite los bytes TLS sin procesar (*raw*) como mensajes `data`.
5. El ESP32 recibe esos mensajes, abre un socket TCP real hacia el host de destino y reenvía los bytes.
6. Las respuestas del servidor real regresan a través de MQTT (tema `res`), son recibidas por el proxy de Python y se envían de vuelta a Firefox.

---

## Notas

- Todo el tráfico se retransmite en bruto (*raw*): sin interceptación TLS ni descifrado.
- Esto es una prueba de concepto (PoC), no un proxy robusto o preparado para producción. Cabe esperar inestabilidad bajo cargas paralelas intensas.
---

## Limitaciones

El ESP32 tiene **recursos limitados y carece de multihilo** (*multithreading*), por lo que no puede gestionar demasiadas conexiones simultáneas. Intentar cargar páginas en varias pestañas no funcionará.

Las imágenes y los vídeos *sí* funcionan, pero para ahorrar ancho de banda (especialmente en una cuenta MQTT gratuita) y mejorar la velocidad, considera utilizar un bloqueador de contenido como **Block Image Reloaded** en Firefox.

---

## Uso

### 1. Broker MQTT

Crea un clúster gratuito en [HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/) (o utiliza cualquier *broker* MQTT accesible desde ambos dispositivos). Toma nota de lo siguiente:
- Nombre de host (Hostname)
- Puerto (normalmente `8883` para TLS)
- Nombre de usuario
- Contraseña

### 2. Configurar las credenciales

Edita ambos archivos y establece tus credenciales MQTT:

**`server.py`**
```python
broker = "your-cluster-id.s1.eu.hivemq.cloud"
user   = "your-username"
pw     = "your-password"
```
**`esp.ino`**
```c
const char* ssid    = "your-wifi-ssid";
const char* pass    = "your-wifi-password";
const char* broker  = "your-cluster-id.s1.eu.hivemq.cloud";
const char* user    = "your-username";
const char* mpass   = "your-password";
```

### 3. Cargar el código en el ESP32

Abre `esp.ino` en el IDE de Arduino, selecciona tu placa ESP32 y carga el programa. Abre el Monitor Serie a 115200 baudios para ver la actividad. Prefiero usar PuTTY para poder copiar grandes cantidades de salida.

### 4. Ejecutar el servidor proxy en Python

```
pip install paho-mqtt
python server.py
```

Deberías ver:

```
Listening on 127.0.0.1:8080
Set Firefox HTTPS proxy to 127.0.0.1:8080
Press Ctrl+C to stop
```

### 5. Configurar Firefox para usar el proxy

En Firefox:

1. Ve a Configuración → General → Configuración de red → Configuración...

2. Selecciona "Configuración manual del proxy"

3. Establece el proxy HTTPS en `127.0.0.1`, puerto `8080`

4. Asegúrate de que `localhost` y `127.0.0.1` no estén en la lista de excepciones ("No usar proxy para")

5. Haz clic en Aceptar

Visita cualquier sitio HTTPS. El tráfico se transmitirá a través de MQTT hacia el ESP32 y de ahí al servidor real.

### Temas MQTT
|Tema (Topic)|Dirección|Propósito|
|---|---|---| |req|Python → ESP32|Bytes de cliente a servidor (open, data, close)|
|res|ESP32 → Python|Bytes de servidor a cliente (data)|


### Formato del mensaje

## Todos los mensajes MQTT están en formato JSON:

```json

{
"conn_id": 5,
"host": "www.example.com",
"port": 443,
"type": "data",
"data": "FgMBB2ABAAdc..."
}
```

- type puede ser: open, data o close

- data son bytes sin procesar codificados en base64, o null para open/close

- conn_id identifica la conexión TCP (Firefox abre varias en paralelo)
