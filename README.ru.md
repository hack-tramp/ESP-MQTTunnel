[🇺🇸 English](README.md)  [🇨🇳 中文](README.zh.md)  [🇷🇺 русский](README.ru.md)  [🇮🇳 हिंदी](README.hi.md)  [🇪🇸 español](README.es.md)
<p align="center">
<img width="200" src="https://github.com/hack-tramp/WNetWrap/blob/main/logo.png?raw=true">
<h1 align="center">ESP-MQTTunnel <br>
<img align="center" src="https://img.shields.io/badge/c%2B%2B-11-blue" href="https://github.com/topics/c-plus-plus-11">
<img align="center" src="https://img.shields.io/badge/License-MIT-green.svg" href="https://opensource.org/licenses/MIT">
<img align="center" src="https://www.ardu-badge.com/badge/MQTT.svg" href="https://mqtt.org">
<img align="center" src="https://www.ardu-badge.com/badge/HttpClient.svg" href="https://arduino.cc">
</h1>
</p>





Рабочий прототип (PoC), превращающий ESP32 в аппаратный прокси-сервер стоимостью 3 доллара, который передает трафик через MQTT. Поскольку устройство инициирует исходящее соединение с брокером, можно обходить цензуру без открытия портов или наличия публичного IP-адреса.

---

## Принцип работы

ESP32 выступает в роли конечной точки TCP-соединения. Python-скрипт работает как локальный прокси-сервер на ноутбуке. MQTT служит лишь каналом связи между ними. ![Alt ​​text](esp-mqtt-diag.svg)
Проверено на: Windows 10 (Firefox / Python 3) <--> ESP32-S3 Dev Module <--> HiveMQ Cloud (бесплатный тариф)
> ⚠️ MQTT-брокер должен быть доступен из той страны, где находится ноутбук.
---

## Подтвержденная работоспособность

- ✅ YouTube
- ✅ Gmail
- ✅ Twitter / X
- ✅ Новостные сайты
- ✅ Reddit

## Известные проблемы

- ❌ Instagram зависает
---
## Принцип работы

1. **ESP32** подключается к Wi-Fi и подписывается на MQTT-топик `req`.
2. **Python-скрипт** запускает локальный HTTP-прокси на ноутбуке (например, `127.0.0.1:8080`).
3. Firefox настраивается на использование этого локального прокси.
4. Когда Firefox запрашивает сайт, Python-прокси:
- Разбирает строку `CONNECT host:port`. 
- Публикует сообщение `open` через MQTT. 
- Передает «сырые» байты TLS в виде сообщений `data`.
5. ESP32 получает эти сообщения, открывает реальный TCP-сокет к целевому хосту и пересылает байты.
6. Ответы от реального сервера возвращаются через MQTT (топик `res`), принимаются Python-прокси и передаются обратно в Firefox.

---

## Примечания

- Весь трафик передается в **«сыром» виде** — без перехвата TLS и без расшифровки.
- Это прототип (PoC), а не защищенный прокси-сервер. Возможны сбои или нестабильная работа при высокой параллельной нагрузке.
---

## Ограничения

У ESP32 **ограниченные ресурсы и отсутствует многопоточность**, поэтому он не может обрабатывать слишком много одновременных соединений. Попытка загружать страницы в нескольких вкладках работать не будет.

Изображения и видео **работают**, но для экономии трафика (особенно на бесплатном аккаунте MQTT) и повышения скорости рекомендуется использовать блокировщик контента, например **Block Image Reloaded** для Firefox.

---

## Использование

### 1. MQTT-брокер

Создайте бесплатный кластер на [HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/) (или используйте любой MQTT-брокер, доступный с обоих устройств). Запишите следующие данные:
- Имя хоста (Hostname)
- Порт (обычно `8883` для TLS)
- Имя пользователя
- Пароль

### 2. Настройка учетных данных

Отредактируйте оба файла и укажите свои учетные данные для MQTT:

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

### 3. Прошивка ESP32

Откройте файл `esp.ino` в Arduino IDE, выберите плату ESP32 и выполните загрузку (upload). Откройте монитор последовательного порта (Serial Monitor) со скоростью 115200 бод, чтобы видеть ход выполнения. Я предпочитаю использовать PuTTY, так как это позволяет копировать большие объемы выводимых данных.

### 4. Запуск прокси-сервера на Python

```
pip install paho-mqtt
python server.py
```

Вы должны увидеть следующее:

```
Listening on 127.0.0.1:8080
Set Firefox HTTPS proxy to 127.0.0.1:8080
Press Ctrl+C to stop
```

### 5. Настройка Firefox на использование прокси

В Firefox:

1. Перейдите в Настройки (Settings) → Основные (General) → Настройки сети (Network Settings) → Настроить... (Settings…)

2. Выберите «Ручная настройка прокси» (Manual proxy configuration)

3. Установите HTTPS-прокси: `127.0.0.1`, порт `8080`

4. Убедитесь, что `localhost` и `127.0.0.1` отсутствуют в списке исключений («Не использовать прокси для» / "No proxy for")

5. Нажмите OK

Посетите любой HTTPS-сайт. Трафик будет передаваться через MQTT на ESP32, а затем — на реальный сервер.

### MQTT-топики
|Топик|Направление|Назначение|
|---|---|---| |req|Python → ESP32|Байты от клиента к серверу (open, data, close)|
|res|ESP32 → Python|Байты от сервера к клиенту (data)|


### Формат сообщений

## Все сообщения MQTT представлены в формате JSON:

```json

{
"conn_id": 5,
"host": "www.example.com",
"port": 443,
"type": "data",
"data": "FgMBB2ABAAdc..."
}
```

- type: одно из значений open, data, close

- data: необработанные байты в кодировке base64 (или null для open/close)

- conn_id: идентификатор TCP-соединения (Firefox открывает несколько соединений параллельно)
