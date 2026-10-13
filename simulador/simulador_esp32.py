#!/usr/bin/env python3
"""
Simulador de respaldo del ESP32 (mismo JSON, mismos topicos, mismo LWT).
Util si Wokwi no puede alcanzar tu Mosquitto local o para repetir pruebas rapido.

Instalacion:  pip install paho-mqtt
Ejemplos:
  python simulador_esp32.py --escenario normal
  python simulador_esp32.py --escenario advertencia
  python simulador_esp32.py --escenario critico
  python simulador_esp32.py --escenario luz
  python simulador_esp32.py --escenario invalido
  python simulador_esp32.py --escenario ciclo --duracion 300      # recorre todos
  python simulador_esp32.py --escenario normal --pausa-en 20 --pausa-seg 30   # prueba (e)
"""
import argparse, json, random, time
import paho.mqtt.client as mqtt

TOPIC_TEL = "dcandino/sala01/telemetria"
TOPIC_ESTADO = "dcandino/sala01/estado"
DEVICE_ID = "esp32-sala01"


def lectura(escenario, t0):
    """Devuelve (temp, hum, luz_pct) segun el escenario."""
    ruido = random.uniform(-0.3, 0.3)
    hum = round(random.uniform(40, 50), 1)
    if escenario == "normal":
        return round(25 + ruido, 1), hum, random.randint(40, 60)
    if escenario == "advertencia":
        return round(28.8 + ruido, 1), hum, random.randint(40, 60)
    if escenario == "critico":
        return round(31.5 + ruido, 1), hum, random.randint(40, 60)
    if escenario == "luz":
        luz = 15 if int((time.time() - t0) // 20) % 2 == 0 else 85   # alterna cada 20 s
        return round(25 + ruido, 1), hum, luz
    if escenario == "invalido":
        return None, None, random.randint(40, 60)                    # DHT22 fallando
    if escenario == "ciclo":
        fase = int((time.time() - t0) // 30) % 4
        return lectura(["normal", "advertencia", "critico", "luz"][fase], t0)
    raise SystemExit(f"Escenario desconocido: {escenario}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="localhost")
    ap.add_argument("--port", type=int, default=1883)
    ap.add_argument("--user", default=None)
    ap.add_argument("--password", default=None)
    ap.add_argument("--escenario", default="normal",
                    choices=["normal", "advertencia", "critico", "luz", "invalido", "ciclo"])
    ap.add_argument("--intervalo", type=float, default=5.0)
    ap.add_argument("--duracion", type=float, default=0, help="segundos (0 = infinito)")
    ap.add_argument("--pausa-en", type=int, default=0, help="enviar pausa tras N mensajes (prueba e)")
    ap.add_argument("--pausa-seg", type=float, default=30)
    a = ap.parse_args()

    try:
        c = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=DEVICE_ID)   # paho-mqtt 2.x
    except AttributeError:
        c = mqtt.Client(client_id=DEVICE_ID)                                      # paho-mqtt 1.x
    if a.user:
        c.username_pw_set(a.user, a.password)
    c.will_set(TOPIC_ESTADO, "offline", qos=1, retain=True)
    c.connect(a.host, a.port, keepalive=15)
    c.loop_start()
    c.publish(TOPIC_ESTADO, "online", qos=1, retain=True)

    t0, seq = time.time(), 0
    print(f"Publicando en {TOPIC_TEL} ({a.escenario}) cada {a.intervalo}s. Ctrl+C para salir.")
    try:
        while a.duracion == 0 or time.time() - t0 < a.duracion:
            temp, hum, luz = lectura(a.escenario, t0)
            payload = {"device_id": DEVICE_ID, "seq": seq, "ts": int(time.time() * 1000),
                       "temp": temp, "hum": hum, "luz": int((100 - luz) * 40.95), "luz_pct": luz}
            c.publish(TOPIC_TEL, json.dumps(payload), qos=1)
            print("PUB", payload)
            seq += 1
            if a.pausa_en and seq == a.pausa_en:
                print(f"--- PAUSA de {a.pausa_seg}s (simula interrupcion) ---")
                time.sleep(a.pausa_seg)
            time.sleep(a.intervalo)
    except KeyboardInterrupt:
        pass
    finally:
        c.publish(TOPIC_ESTADO, "offline", qos=1, retain=True).wait_for_publish()
        c.loop_stop()
        c.disconnect()


if __name__ == "__main__":
    main()
