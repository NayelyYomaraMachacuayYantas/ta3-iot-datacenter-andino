# Guía de ejecución integrada y pruebas (pasos 2 y 3)

## Paso 2 – Levantar todo junto (≈ 30 min)

Orden recomendado, cada uno en su propia terminal/pestaña:

| # | Componente | Comando / acción | Comprobación |
|---|---|---|---|
| 1 | InfluxDB | `influxd` (primera vez: `influxdb/setup.md`) | http://localhost:8086 abre; existe el bucket `sala_servidores` |
| 2 | Mosquitto | `mosquitto -c mosquitto/mosquitto.conf -v` | El log muestra `Connecting bridge ... puente_publico (broker.emqx.io:1883)` sin errores |
| 3 | Node-RED | `node-red` → importar `nodered/flows.json`, pegar token, *Deploy* | Nodos `Telemetria` y `Estado (LWT)` en verde “connected” |
| 4 | Grafana | Data source InfluxDB (Flux) + importar `grafana/dashboard.json` | Dashboard carga sin errores de query |
| 5 | Wokwi | Pegar `sketch.ino`, `diagram.json`, `libraries.txt` → ▶ | Monitor serie: `MQTT clientId: dcandino_nayely_esp32_XXXXXXXX`, `Conectando a MQTT... OK`, `PUB {...}` cada 5 s |

Cadena de verificación (de izquierda a derecha; si falla un eslabón, no sigas):

1. **Wokwi → EMQX**: el monitor serie muestra `PUB`.
2. **EMQX → Mosquitto (puente)**: `mosquitto_sub -h localhost -t "dcandino/#" -v` muestra los mensajes.
3. **Mosquitto → Node-RED**: pestaña *Debug* sin avisos “Lectura invalida”; contador de mensajes del nodo `Telemetria`.
4. **Node-RED → InfluxDB**: consulta 2 de `influxdb/queries.flux` devuelve filas recientes.
5. **InfluxDB → Grafana**: temperatura, iluminación y **Estado de iluminación (BAJA/NORMAL/ALTA)** se actualizan (refresco 5 s).

Si el puente no conecta, usar el respaldo: `python simulador/simulador_esp32.py --escenario normal` (publica directo en el broker local; documentarlo en el informe).

## Paso 3 – Pruebas, indicadores y capturas (≈ 1 h)

### Cómo provocar cada prueba
| ID | Con Wokwi | Con simulador (respaldo) | Qué anotar |
|---|---|---|---|
| P1 | DHT22 en ≈ 25 °C, luz media | `--escenario normal` | Estado NORMAL, filas en `ambiente` |
| P2 | DHT22 en 28.5 °C | `--escenario advertencia` | ADVERTENCIA amarillo, sin alerta crítica |
| P3 | DHT22 en 31 °C | `--escenario critico` | CRITICO rojo, 1 evento en `alertas` (no uno por lectura) |
| P4 | Mover el control de luz de ~15 % a ~85 % | `--escenario luz` (alterna cada 20 s) | Cambio en < 10 s en el gráfico y en **Estado de iluminación** (BAJA → ALTA) |
| P5 | Detener la simulación ≥ 30 s y reanudar | `--escenario normal --pausa-en 20 --pausa-seg 30` | A los 15 s SIN DATOS + evento `sin_datos`; al reanudar vuelve el estado |
| P6 | Forzar DHT22 con fallo (`null`) | `--escenario invalido` | Descartado, fila en `lecturas_invalidas`, nada nuevo en `ambiente` |

Completar las columnas **Observado** y **Evidencia** de `docs/PRUEBAS.md` con lo que realmente ocurrió (hora, valores, nombre de la captura).

### Indicadores (`influxdb/queries.flux`)
1. **Latencia de visualización**: cronometrar 5 veces (publicar → verse en Grafana) y promediar; la latencia ESP32 → Node-RED sale de `latencia_ms` (incluye el salto por EMQX y el puente).
2. **% de mensajes recibidos**: ventana de 5 min, consultas KPI 2a/2b; % = recibidos / (spread + 1) × 100. No reiniciar el ESP32 durante la ventana.
3. **Tiempo de detección crítica**: diferencia entre `_time` de la primera lectura ≥ 30 °C y el primer evento `temperatura_critica`.

### Capturas (propuesta de 19; cotejar con el enunciado de la TA3)
1. Estructura del repositorio · 2. `mosquitto.conf` · 3. Log de Mosquitto con el puente conectado · 4. Node-RED flujo completo · 5. Node-RED *Deploy*/nodos conectados · 6. Wokwi circuito · 7. Wokwi monitor serie (`clientId` + `PUB`) · 8. `mosquitto_sub` recibiendo · 9. InfluxDB bucket/modelo de datos · 10. InfluxDB consulta con `_time` · 11. Grafana dashboard completo (P1) · 12. P2 ADVERTENCIA · 13. P3 CRITICO + tabla de alertas · 14. P3 evento en InfluxDB · 15. P4 iluminación BAJA/ALTA · 16. P5 SIN DATOS · 17. P6 dato inválido (Debug + `lecturas_invalidas`) · 18. Indicador % recibidos · 19. Indicadores de latencia y detección.

> No declarar ahorros ni mejoras reales: no existe línea base medida.
