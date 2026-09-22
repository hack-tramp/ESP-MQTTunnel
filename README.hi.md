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





यह एक काम करने वाला 'प्रूफ-ऑफ-कांसेप्ट' है जो ESP32 को $3 के हार्डवेयर प्रॉक्सी में बदल देता है और MQTT के ज़रिए ट्रैफ़िक को टनल करता है। क्योंकि यह एक ब्रोकर से कनेक्ट होता है, इसलिए आप बिना ओपन पोर्ट या पब्लिक IP के सेंसरशिप को बायपास कर सकते हैं।

---

## यह क्या करता है

ESP32 असल TCP एंडपॉइंट है। Python स्क्रिप्ट लैपटॉप पर लोकल प्रॉक्सी की तरह काम करती है। MQTT इनके बीच बस एक पाइप का काम करता है। ![Alt ​​text](esp-mqtt-diag.svg)
इनके साथ टेस्ट किया गया: Windows 10 Firefox / Python 3 <--> ESP32-S3 Dev Module <--> HiveMQ Cloud (फ्री टियर)
> ⚠️ MQTT ब्रोकर उस देश से एक्सेस किया जा सकने वाला होना चाहिए जहाँ लैपटॉप मौजूद है।
---

## कन्फर्म तौर पर काम कर रहा है

- ✅ YouTube
- ✅ Gmail
- ✅ Twitter / X
- ✅ News sites
- ✅ Reddit

## ज्ञात समस्याएँ

- ❌ Instagram अटक जाता है
---
## यह कैसे काम करता है

1. **ESP32** आपके Wi-Fi से कनेक्ट होता है और MQTT `req` टॉपिक को सब्सक्राइब करता है।
2. **Python स्क्रिप्ट** लैपटॉप पर एक लोकल HTTP प्रॉक्सी चलाती है (जैसे `127.0.0.1:8080`)।
3. Firefox को उस लोकल प्रॉक्सी का इस्तेमाल करने के लिए कॉन्फ़िगर किया जाता है।
4. जब Firefox किसी साइट के लिए रिक्वेस्ट करता है, तो Python प्रॉक्सी:
- `CONNECT host:port` लाइन को पार्स करती है। 
- MQTT पर एक `open` मैसेज पब्लिश करती है। 
- रॉ TLS बाइट्स को `data` मैसेज के तौर पर स्ट्रीम करती है।
5. ESP32 उन मैसेज को रिसीव करता है, टारगेट होस्ट के लिए एक असली TCP सॉकेट खोलता है, और बाइट्स को आगे भेजता है।
6. असली सर्वर से रिस्पॉन्स MQTT (`res` टॉपिक) के ज़रिए वापस आते हैं, Python प्रॉक्सी उन्हें रिसीव करती है, और Firefox को वापस भेजती है।

---

## नोट्स

- सारा ट्रैफ़िक **रॉ (raw)** रूप में रिले किया जाता है — कोई TLS इंटरसेप्शन या डिक्रिप्शन नहीं होता।
- यह एक PoC (प्रूफ ऑफ़ कॉन्सेप्ट) है, न कि कोई मज़बूत प्रॉक्सी। ज़्यादा पैरेलल लोड होने पर इसमें कुछ कमियाँ आ सकती हैं।
---

## सीमाएँ

ESP32 में **सीमित रिसोर्स होते हैं और इसमें मल्टीथ्रेडिंग नहीं होती**, इसलिए यह एक साथ बहुत सारे कनेक्शन नहीं संभाल सकता। कई टैब में पेज लोड करने की कोशिश करने पर यह काम नहीं करेगा।

इमेज और वीडियो *काम करते हैं*, लेकिन बैंडविड्थ बचाने (खासकर फ्री MQTT अकाउंट पर) और स्पीड बेहतर करने के लिए, Firefox में **Block Image Reloaded** जैसे कंटेंट ब्लॉकर का इस्तेमाल करने पर विचार करें।

---

## इस्तेमाल

### 1. MQTT ब्रोकर

[HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/) पर एक फ्री क्लस्टर बनाएँ (या किसी ऐसे MQTT ब्रोकर का इस्तेमाल करें जिसे दोनों डिवाइस से एक्सेस किया जा सके)। इन बातों का ध्यान रखें:
- होस्टनेम
- पोर्ट (आमतौर पर TLS के लिए `8883`)
- यूज़रनेम
- पासवर्ड

### 2. क्रेडेंशियल कॉन्फ़िगर करें

दोनों फ़ाइलों को एडिट करें और अपने MQTT क्रेडेंशियल सेट करें:

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

### 3. ESP32 को फ़्लैश करें

Arduino IDE में esp.ino खोलें, अपना ESP32 बोर्ड चुनें और अपलोड करें। गतिविधि देखने के लिए 115200 बॉड पर सीरियल मॉनिटर खोलें। मैं PuTTY का इस्तेमाल करना पसंद करता हूँ ताकि मैं ज़्यादा मात्रा में आउटपुट कॉपी कर सकूँ।

### 4. Python प्रॉक्सी सर्वर चलाएँ

```
pip install paho-mqtt
python server.py
```

आपको यह दिखना चाहिए

```
Listening on 127.0.0.1:8080
Set Firefox HTTPS proxy to 127.0.0.1:8080
Press Ctrl+C to stop
```

### 5. Firefox को प्रॉक्सी पर सेट करें

Firefox में:

1. Settings → General → Network Settings → Settings… पर जाएँ

2. Manual proxy configuration चुनें

3. HTTPS Proxy को 127.0.0.1 पोर्ट 8080 पर सेट करें

4. पक्का करें कि localhost और 127.0.0.1 "No proxy for" लिस्ट में न हों

5. OK पर क्लिक करें

किसी भी HTTPS साइट पर जाएँ। ट्रैफ़िक MQTT के ज़रिए ESP32 तक और फिर असली सर्वर तक जाएगा।

### MQTT टॉपिक्स
|टॉपिक|दिशा|मकसद|
|---|---|---| |req|Python → ESP32|Client-to-server बाइट्स (open, data, close)|
|res|ESP32 → Python|Server-to-client बाइट्स (data)|


### मैसेज फ़ॉर्मैट

## सभी MQTT मैसेज JSON होते हैं:

```json

{
"conn_id": 5,
"host": "www.example.com",
"port": 443,
"type": "data",
"data": "FgMBB2ABAAdc..."
}
```

- type इनमें से कोई एक होता है: open, data, close

- data बेस64-एनकोडेड रॉ बाइट्स होते हैं, या open/close के लिए null होते हैं

- conn_id TCP कनेक्शन की पहचान करता है (फ़ायरफ़ॉक्स एक साथ कई कनेक्शन खोलता है)
