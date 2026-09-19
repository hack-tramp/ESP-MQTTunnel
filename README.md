This is currently a working PoC for a ESP32 HTTPS (TCP)proxy via MQTT server. 
Simple text based sites like www.example.com work. 

On the one side you have the ESP which connects to the wifi to be shared. 
The python script starts a local proxy on a laptop, and both devices will use MQTT as an intermediary.
This is a blind TCP byte forwarder proxy. 
