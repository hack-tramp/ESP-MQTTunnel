import socket, threading, signal, base64, json, itertools
import paho.mqtt.client as mqtt

broker = "mqtt url s1.eu.hivemq.cloud"
user = "mqtt username"
pw = "mqtt password"

running = True
conn_counter = itertools.count(1)

# conn_id -> Firefox socket, so res messages can be written back
clients = {}
clients_lock = threading.Lock()

def signal_handler(sig, frame):
    global running
    running = False

# MQTT setup
client = mqtt.Client(client_id="laptop", protocol=mqtt.MQTTv5)
client.tls_set()
client.username_pw_set(user, pw)

def on_message(mqtt_client, userdata, msg):
    try:
        m = json.loads(msg.payload)
        conn_id = m.get("conn_id")
        data = m.get("data")
        if not data:
            return
        raw = base64.b64decode(data)
        with clients_lock:
            sock = clients.get(conn_id)
        if sock is None:
            return
        try:
            sock.sendall(raw)
        except Exception as e:
            print(f"res write error for conn {conn_id}: {e}")
    except Exception as e:
        print("res parse error:", e)

client.on_message = on_message
client.connect(broker, 8883, 60)
client.subscribe("res", qos=0)
client.loop_start()

def publish(topic, conn_id, host, port, type, chunk=None):
    payload = json.dumps({
        "conn_id": conn_id,
        "host": host,
        "port": port,
        "type": type,
        "data": base64.b64encode(chunk).decode("ascii") if chunk is not None else None,
    })
    client.publish(topic, payload, qos=0)

def parse_first_block(buf):
    """Return (host, port, is_connect) from the first HTTP block, or (None,None,False)."""
    if b"\r\n\r\n" not in buf:
        return None, None, False
    head = buf.split(b"\r\n\r\n", 1)[0]
    first = head.split(b"\r\n", 1)[0]
    if first.startswith(b"CONNECT "):
        target = first.split(b" ", 2)[1].decode("ascii", "replace")
        h, _, p = target.partition(":")
        return h, (int(p) if p.isdigit() else None), True
    for line in head.split(b"\r\n")[1:]:
        if line.lower().startswith(b"host:"):
            val = line.split(b":", 1)[1].strip().decode("ascii", "replace")
            h, _, p = val.partition(":")
            return h, (int(p) if p.isdigit() else 80), False
    return None, None, False

def handle_client(sock, addr, conn_id):
    host = None
    port = None
    with clients_lock:
        clients[conn_id] = sock
    try:
        sock.settimeout(1)
        buf = b""
        # phase 1: wait for the CONNECT (or plain HTTP) header block
        while running and b"\r\n\r\n" not in buf:
            try:
                chunk = sock.recv(4096)
                print(f"--- {len(chunk)} bytes ---")
                print(repr(chunk[:100]))
            except socket.timeout:
                continue
            if not chunk:
                return
            buf += chunk
            if host is None:
                h, p, is_connect = parse_first_block(buf)
                if h is not None:
                    host, port = h, p
                    if is_connect:
                        sock.sendall(b"HTTP/1.1 200 Connection established\r\n\r\n")
                        # do NOT forward CONNECT bytes — signal "open" instead
                        publish("req", conn_id, host, port, "open")
                        buf = b""  # drop what we consumed
                        continue
                    else:
                        # plain HTTP: header block is forwardable as-is
                        publish("req", conn_id, host, port, "open")
            publish("req", conn_id, host, port, "data", chunk)
        # phase 2: pump the rest (TLS ClientHello etc.) as raw data
        while running:
            try:
                chunk = sock.recv(4096)
                print(f"--- {len(chunk)} bytes ---")
                print(repr(chunk[:100]))
            except socket.timeout:
                continue
            if not chunk:
                break
            publish("req", conn_id, host, port, "data", chunk)
    except Exception as e:
        print("client error:", e)
    finally:
        with clients_lock:
            clients.pop(conn_id, None)
        publish("req", conn_id, host, port, "close")
        sock.close()

# TCP server
server = socket.socket()
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("127.0.0.1", 8080))
server.listen()
server.settimeout(1)
print("Listening on 127.0.0.1:8080")
print("Set Firefox HTTPS proxy to 127.0.0.1:8080")
print("Press Ctrl+C to stop")

signal.signal(signal.SIGINT, signal_handler)

try:
    while running:
        try:
            client_sock, addr = server.accept()
            cid = next(conn_counter)
            print("connection", cid)
            threading.Thread(target=handle_client, args=(client_sock, addr, cid), daemon=True).start()
        except socket.timeout:
            continue
finally:
    server.close()
    client.loop_stop()
    print("\nStopped")
