This is currently a working PoC for a ESP32 HTTPS (TCP)proxy via MQTT server. 

Tested with:
- ESP32-S3
- Hive MQTT
- Win10 + Firefox + Python 3 server

The ESP32 has limited resources, and no multithreading. This means it can't handle too many connections at once!
Images and videos work but to save bandwidth (especially if you're using a free MQTT account!) and make things faster please consider plugins like "Block Image Reloaded". 

Confirmed working:
Youtube
Gmail
Twitter
News sites
Reddit

Got stuck:
Instagram

How it works:

On the one side you have the ESP which connects to the wifi to be shared. 

The python script starts a local proxy on a laptop, and both devices will use MQTT as an intermediary.

Obviously the MQTT server will need to be accessible from whatever country the laptop is in.

