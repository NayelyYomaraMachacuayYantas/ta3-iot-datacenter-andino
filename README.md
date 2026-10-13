# TA3 – Monitoreo IoT de temperatura e iluminación · DataCenter Andino S.A.C.

Prototipo académico: **ESP32 (Wokwi) + DHT22 + LDR → MQTT → Mosquitto → Node-RED → InfluxDB → Grafana**.
Se ejecuta **sin contenedores**. Los umbrales (28 / 30 °C) son parámetros didácticos, no límites normativos.

```
DHT22 + LDR → ESP32 → Wi-Fi → MQTT → Mosquitto → Node-RED → InfluxDB → Grafana
                                          (valida, clasifica, alerta)
```

## 1. Requisitos
| Componente | Versión sugerida |
|---|---|
| Mosquitto | 2.x |
| Node.js + Node-RED | Node 18+ / Node-RED 3.x o 4.x |
| Paleta Node-RED | `node-red-contrib-influxdb` |
| InfluxDB | 2.x (+ CLI `influx`) |
| Grafana | 10.x o superior |
| Simulación | Cuenta en https://wokwi.com (o `simulador/simulador_esp32.py` + Python 3 + `paho-mqtt`) |

## 2. Estructura
```
esp32-wokwi/   sketch.ino, diagram.json, libraries.txt
mosquitto/     mosquitto.conf
nodered/       flows.json (flujo exportado, sin credenciales)
influxdb/      setup.md (bucket, token, modelo de datos), queries.flux
grafana/       dashboard.json (importable)
simulador/     simulador_esp32.py (respaldo / pruebas repetibles)
docs/          PRUEBAS.md (plantilla de pruebas e indicadores)
.env.example   variables de ejemplo (no subir .env)
```

## 3. Instalación y arranque (orden recomendado)

### 3.1 Mosquitto
1. Instalar Mosquitto y copiar `mosquitto/mosquitto.conf` (ver comentarios del archivo).
2. Iniciar: `mosquitto -c mosquitto.conf -v`
3. Verificar: en otra terminal `mosquitto_sub -h localhost -t "dcandino/#" -v`
   y en otra `mosquitto_pub -t dcandino/test -m hola`.

Parámetros del broker (`mosquitto.conf`): `listener 1883`, `allow_anonymous true` (solo prototipo), `persistence true` y el **puente** hacia `broker.emqx.io` (ver sección 4, opción D). Para que el puente funcione, el equipo necesita salida a Internet por el puerto 1883.

### 3.2 InfluxDB
Seguir `influxdb/setup.md` (org `dcandino`, bucket `sala_servidores`, token con permisos solo sobre el bucket).

### 3.3 Node-RED
1. `npm install -g node-red` y luego `node-red` (http://localhost:1880).
2. Menú → *Manage palette* → instalar `node-red-contrib-influxdb`.
3. Menú → *Import* → pegar `nodered/flows.json`.
4. Abrir el nodo de configuración **InfluxDB local** y pegar el **token** (se guarda como credencial, no en `flows.json`).
5. *(Opcional)* Umbrales por variable de entorno antes de iniciar:
   - Windows (PowerShell): `$env:UMBRAL_CRITICO=30; node-red`
   - Linux/macOS: `UMBRAL_CRITICO=30 node-red`
   Si no se definen, se usan 28/30 °C y luz 20/70 %.
6. *Deploy*. El nodo `Telemetria` debe mostrar “connected”.

### 3.4 Grafana
1. Data source → InfluxDB → Query language **Flux**, URL `http://localhost:8086`, organización `dcandino`, token, bucket por defecto `sala_servidores`.
2. *Dashboards → Import* → subir `grafana/dashboard.json` y elegir el data source.

### 3.5 ESP32 en Wokwi
1. Nuevo proyecto ESP32 → pegar `sketch.ino`, `diagram.json` y `libraries.txt`.
2. El sketch ya apunta a `broker.emqx.io:1883` y genera un **ID de conexión MQTT único** en cada arranque (ver sección siguiente).
3. Ejecutar y observar el monitor serie (`PUB {...}`).

## 4. Conectar Wokwi con Mosquitto
El ESP32 de Wokwi corre en la nube y **no ve `localhost`**. Opciones:
- **A. Wokwi IoT Gateway**: permite al ESP32 simulado alcanzar tu red local (revisar en la documentación de Wokwi si requiere plan de pago).
- **B. Exponer tu broker**: túnel TCP (p. ej. `ngrok tcp 1883`) o broker en una VM; usar el host/puerto resultante en `sketch.ino`. Con un broker expuesto, **activar autenticación**.
- **C. Respaldo**: `python simulador/simulador_esp32.py --escenario normal` publica los mismos mensajes. Documentar en el informe que el sensor es simulado por software.
- **D. Puente (opción usada)**: el ESP32 publica en el broker público `broker.emqx.io` y el Mosquitto local se conecta a ese broker como cliente (*bridge*) y copia los mensajes hacia el broker local.

### Opción D: cómo funciona el puente
```
Wokwi (ESP32) → broker.emqx.io:1883 ──(puente, dirección "in")──→ Mosquitto local :1883 → Node-RED → InfluxDB → Grafana
 dcandino_nayely_7342/sala01/#                                      dcandino/sala01/#
```
1. **Salida del ESP32**: `sketch.ino` usa `MQTT_HOST = "broker.emqx.io"` y publica en tópicos con prefijo propio `dcandino_nayely_7342/…`. Ese prefijo evita mezclar mensajes con otros usuarios del broker público, que es compartido y sin control de acceso.
2. **ID de conexión único**: en un broker público dos clientes con el mismo *client ID* se desconectan entre sí. El sketch genera `dcandino_nayely_esp32_<hex aleatorio>` en cada arranque.
3. **El puente**: en `mosquitto.conf`, el bloque `connection puente_publico` hace que Mosquitto se conecte a `broker.emqx.io:1883` con su propio ID (`dcandino_nayely_puente`) y aplique la regla:
   `topic sala01/# in 1 dcandino/ dcandino_nayely_7342/`
   - `sala01/#`: patrón de tópicos que se transfieren.
   - `in`: solo **remoto → local** (el broker local nunca publica hacia EMQX).
   - `1`: QoS 1.
   - `dcandino/`: prefijo **local**. `dcandino_nayely_7342/`: prefijo **remoto**.
   Resultado: `dcandino_nayely_7342/sala01/telemetria` (EMQX) llega como `dcandino/sala01/telemetria` (local), que es exactamente el tópico que usa Node-RED y el simulador de respaldo. **Node-RED no necesita cambios.**
4. **Ventajas**: no requiere túneles ni planes de pago; Node-RED, InfluxDB y Grafana siguen siendo locales.
5. **Limitaciones**: depende de Internet y de un broker público sin autenticación (no usar datos sensibles); añade latencia (ESP32 → EMQX → puente → Mosquitto), que se refleja en el indicador de latencia.

**Verificación rápida del puente**
```
mosquitto -c mosquitto/mosquitto.conf -v          # el log debe mostrar la conexión "puente_publico" a broker.emqx.io
mosquitto_sub -h localhost -t "dcandino/#" -v     # deben aparecer los mensajes del ESP32
mosquitto_sub -h broker.emqx.io -t "dcandino_nayely_7342/#" -v   # (opcional) ver el lado remoto
```

## 5. Tópicos MQTT
| Tópico (local, el que lee Node-RED) | Tópico remoto (EMQX, el que publica el ESP32) | Dirección | Contenido | QoS / retain |
|---|---|---|---|---|
| `dcandino/sala01/telemetria` | `dcandino_nayely_7342/sala01/telemetria` | ESP32 → broker | JSON de lectura | 0 (sketch) / 1 (simulador) |
| `dcandino/sala01/estado` | `dcandino_nayely_7342/sala01/estado` | ESP32 / LWT → broker | `online` / `offline` | 1 / retain |

El simulador de Python publica directamente en el tópico local (`dcandino/…`), sin pasar por el puente.

Mensaje de telemetría:
```json
{"device_id":"esp32-sala01","seq":42,"ts":1760000000000,"temp":26.5,"hum":45.0,"luz":2100,"luz_pct":48}
```
`seq` permite comparar publicados vs. recibidos; `ts` (ms UTC por NTP) permite medir latencia; `temp:null` indica fallo del DHT22.

## 6. Reglas de Node-RED
- **Validación**: temperatura numérica en −40…80 °C, humedad 0…100 %, luz 0…100 %. Si falla → `lecturas_invalidas` + aviso, no se guarda en `ambiente`.
- **Clasificación**: NORMAL < 28 °C · ADVERTENCIA ≥ 28 y < 30 · CRITICO ≥ 30. Luz: BAJA < 20 %, NORMAL 20–70 %, ALTA > 70 % (rangos justificados en el informe); el panel **Estado de iluminación** de Grafana muestra esta clasificación.
- **Alertas**: evento en `alertas` solo en transición a CRITICO (y al normalizarse) para no duplicar.
- **Watchdog**: 15 s sin telemetría → `estado_monitoreo = SIN_DATOS` + evento `sin_datos`.
- **LWT**: `offline` del broker se registra en `dispositivo`.

## 7. Estructura de datos en InfluxDB
Ver `influxdb/setup.md` (measurements `ambiente`, `estado_monitoreo`, `alertas`, `lecturas_invalidas`, `dispositivo`) y `influxdb/queries.flux`.

## 8. Pruebas
Seguir `docs/PRUEBAS.md` (P1–P5 obligatorias + P6 dato inválido) y calcular los 3 indicadores con `influxdb/queries.flux`.

## 9. Seguridad
No se suben claves, contraseñas ni tokens (`.env` y credenciales están en `.gitignore`). Para producción: TLS (8883), usuarios/ACL en Mosquitto, tokens de mínimo privilegio, alta disponibilidad y respaldos de InfluxDB.

## 10. Solución de problemas
- *Field type conflict* en InfluxDB: un campo se escribió como entero y luego como decimal. Borrar el bucket de pruebas y recrearlo.
- No llegan datos al broker local: revisar en el log de Mosquitto que el puente esté conectado a `broker.emqx.io`, que el prefijo remoto sea `dcandino_nayely_7342/` y que no haya un firewall bloqueando el puerto 1883.
- Nodo MQTT “disconnected”: revisar que Mosquitto esté activo y el puerto 1883 libre.
- Grafana sin datos: ampliar el rango de tiempo y comprobar token/bucket del data source.
- Pines del `diagram.json` no conectan: reconectar manualmente DHT22 SDA→GPIO15 y LDR AO→GPIO34.

## Limitaciones
Sensores simulados; luz no demuestra presencia de personas; sin línea base no se declaran ahorros.
