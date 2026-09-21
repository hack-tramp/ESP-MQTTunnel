# ESP32 HTTPS Proxy over MQTT

A working proof-of-concept that turns an ESP32 into a $3 hardware proxy using an ESP32 that tunnels traffic over MQTT. Because it connects out to a broker, you can bypass censorship without open ports or a public IP.

---

## What it does

Firefox → **local Python proxy** → MQTT → **ESP32** → real server

The ESP32 is the actual TCP endpoint. The Python script acts as a local proxy on the laptop. MQTT is just the pipe between them.

---

## Tested with

| Component | Version / Notes |
|---|---|
| ESP32 | ESP32-S3 Dev Module |
| MQTT broker | HiveMQ Cloud (free tier) |
| Laptop | Windows 10 |
| Browser | Firefox |
| Proxy | Python 3 |

> ⚠️ The MQTT broker must be reachable from whichever country the laptop is in.

---

## Limitations

The ESP32 has **limited resources and no multithreading**, so it can't handle too many simultaneous connections.

Images and video *do* work, but to save bandwidth (especially on a free MQTT account) and improve speed, consider using a content blocker such as **Block Image Reloaded** in Firefox. Trying to load pages in multiple tabs will not work.

---

## Confirmed working

- ✅ YouTube
- ✅ Gmail
- ✅ Twitter / X
- ✅ News sites
- ✅ Reddit

## Known issues

- ❌ Instagram gets stuck

---

## How it works

1. The **ESP32** connects to your Wi-Fi and subscribes to the MQTT `req` topic.
2. The **Python script** runs a local HTTP proxy on the laptop (e.g. `127.0.0.1:8080`).
3. Firefox is configured to use that local proxy.
4. When Firefox requests a site, the Python proxy:
   - Parses the `CONNECT host:port` line.
   - Publishes an `open` message over MQTT.
   - Streams the raw TLS bytes as `data` messages.
5. The ESP32 receives those messages, opens a real TCP socket to the target host, and forwards the bytes.
6. Responses from the real server come back through MQTT (`res` topic), are received by the Python proxy, and written back to Firefox.

---

## Notes

- All traffic is relayed **raw** — no TLS interception, no decryption.
- This is a PoC, not a hardened proxy. Expect rough edges under heavy parallel loads.
