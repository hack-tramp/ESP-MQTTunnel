#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* ssid = "local wifi";
const char* pass = "wifi password";
const char* broker = " url for MQTT bla1.eu.hivemq.cloud";
const char* user = "MQTT username";
const char* mpass = "MQTT password";

WiFiClientSecure netclient;
PubSubClient mqtt_client(netclient);
// --- upstream connections ---
#define MAX_CONNS 8
struct Conn {
  int      id;
  WiFiClient sock;
  bool     active;
  unsigned long last_used;
};
Conn conns[MAX_CONNS];

String current_url = "";
String current_host = "";
String current_path = "";
int current_port = 443;
unsigned long last_activity = 0;
unsigned long last_reconnect_attempt = 0;
const unsigned long IDLE_TIMEOUT = 30000;
const unsigned long RECONNECT_DELAY = 3000;
bool streaming = false;
bool headers_done = false;
int metaInt = 0;
int audioCount = 0;

// --- base64 decode (no library) ---
int b64val(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

size_t b64decode(const char* in, uint8_t* out, size_t outMax) {
  size_t outLen = 0;
  int val = 0, bits = -8;
  for (const char* p = in; *p; p++) {
    if (*p == '=') break;
    int d = b64val(*p);
    if (d < 0) continue;
    val = (val << 6) | d;
    bits += 6;
    if (bits >= 0) {
      if (outLen < outMax) out[outLen++] = (val >> bits) & 0xFF;
      bits -= 8;
    }
  }
  return outLen;
}

void print_bytes(const uint8_t* raw, size_t n) {
  size_t show = n < 80 ? n : 80;
  for (size_t i = 0; i < show; i++) {
    uint8_t b = raw[i];
    if (b >= 0x20 && b < 0x7f) Serial.print((char)b);
    else { Serial.print("\\x"); if (b < 16) Serial.print('0'); Serial.print(b, HEX); }
  }
  if (n > show) Serial.print("...");
}

void publish_res(int conn_id, const char* host, int port, const uint8_t* raw, size_t n) {
  static char out[32768];
  // base64 encode inline
  static const char* tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t oi = 0;
  const char* prefix_fmt = "{\"conn_id\":%d,\"host\":\"%s\",\"port\":%d,\"type\":\"data\",\"data\":\"";
  int pre = snprintf(out, sizeof(out), prefix_fmt, conn_id, host ? host : "", port);
  if (pre < 0 || (size_t)pre >= sizeof(out)) return;
  oi = pre;
  size_t b64len = ((n + 2) / 3) * 4;
  if (oi + b64len + 4 >= sizeof(out)) return;  // too big, skip
  for (size_t i = 0; i < n; i += 3) {
    uint32_t v = raw[i] << 16;
    if (i + 1 < n) v |= raw[i+1] << 8;
    if (i + 2 < n) v |= raw[i+2];
    out[oi++] = tbl[(v >> 18) & 0x3F];
    out[oi++] = tbl[(v >> 12) & 0x3F];
    out[oi++] = (i + 1 < n) ? tbl[(v >> 6) & 0x3F] : '=';
    out[oi++] = (i + 2 < n) ? tbl[v & 0x3F] : '=';
  }
  out[oi++] = '"'; out[oi++] = '}'; out[oi] = 0;
  mqtt_client.publish("res", out, 0);
}

Conn* find_conn(int id) {
  for (int i = 0; i < MAX_CONNS; i++)
    if (conns[i].active && conns[i].id == id) return &conns[i];
  return nullptr;
}

Conn* alloc_conn() {
  for (int i = 0; i < MAX_CONNS; i++)
    if (!conns[i].active) return &conns[i];
  return nullptr;
}

void drop_conn(Conn* c) {
  if (!c) return;
  int id = c->id;
  c->sock.stop();
  c->active = false;
  c->id = -1;
  Serial.print("  closed conn "); Serial.println(id);
}

void callback(char* t, byte* p, unsigned int l) {
  // PubSubClient payloads aren't null-terminated
  static char json[8192];
  if (l >= sizeof(json)) { Serial.println("payload too big"); return; }
  memcpy(json, p, l);
  json[l] = 0;

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) { Serial.print("json err: "); Serial.println(err.c_str()); return; }

  int conn_id       = doc["conn_id"] | -1;
  const char* host  = doc["host"] | "(null)";
  int port          = doc["port"] | 0;
  const char* type  = doc["type"] | "";
  const char* b64   = doc["data"] | "";

  // decode the base64 into a buffer
  static uint8_t raw[4096];
  size_t rawLen = b64decode(b64, raw, sizeof(raw));

  // --- your original output block ---
  Serial.println("-----------------------------");
  Serial.print("conn_id : "); Serial.println(conn_id);
  Serial.print("type    : "); Serial.println(type);
  Serial.print("host    : "); Serial.println(host);
  Serial.print("port    : "); Serial.println(port);
  Serial.print("bytes   : "); Serial.println(rawLen);
  Serial.print("data    : ");
  print_bytes(raw, rawLen);
  Serial.println();

  // --- relay to upstream ---
  if (strcmp(type, "open") == 0) {
    Conn* c = alloc_conn();
    if (!c) { Serial.println("  no free conn slot"); return; }
    c->id = conn_id;
    c->sock.setTimeout(2);
    c->sock.setNoDelay(true);
    Serial.print("  dialing "); Serial.print(host); Serial.print(":"); Serial.println(port);
    if (!c->sock.connect(host, port)) {
      Serial.println("  connect failed");
      c->active = false;
      c->id = -1;
      return;
    }
    c->active = true;
    c->last_used = millis();

  } else if (strcmp(type, "data") == 0) {
    Conn* c = find_conn(conn_id);
    if (!c) { Serial.print("  data for unknown conn "); Serial.println(conn_id); return; }
    if (rawLen > 0) c->sock.write(raw, rawLen);
    c->last_used = millis();

  } else if (strcmp(type, "close") == 0) {
    Conn* c = find_conn(conn_id);
    if (c) drop_conn(c);
  }
}

void connectMQTT() {
  if(mqtt_client.connect("esp32", user, mpass)) {
    mqtt_client.subscribe("req", 0);
    Serial.println("MQTT connected");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  while(WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("WiFi connected");
  
  netclient.setInsecure();

  for (int i = 0; i < MAX_CONNS; i++) { conns[i].active = false; conns[i].id = -1; }
  
  mqtt_client.setServer(broker, 8883);
  mqtt_client.setBufferSize(32768);
  mqtt_client.setCallback(callback);
  connectMQTT();
}

void loop() {
  if(!mqtt_client.connected()) {
    connectMQTT();
  }
  mqtt_client.loop();

  // poll upstream sockets for response bytes and print them
  static uint8_t rbuf[16384];
  for (int i = 0; i < MAX_CONNS; i++) {
    Conn* c = &conns[i];
    if (!c->active) continue;
    if (millis() - c->last_used > 8000) { drop_conn(c); continue; }
    if (!c->sock.connected()) { drop_conn(c); continue; }
    int avail = c->sock.available();
    if (avail > 0) {
      int n = c->sock.read(rbuf, avail > sizeof(rbuf) ? sizeof(rbuf) : avail);
      if (n > 0) {
        Serial.print("<< conn "); Serial.print(c->id);
        Serial.print(" "); Serial.print(n); Serial.print(" bytes: ");
        print_bytes(rbuf, n);
        Serial.println();
        c->last_used = millis();
        publish_res(c->id, "", 0, rbuf, n);   // <-- new line
      }
    }
  }

  delay(1);
}
