# Registro de pruebas funcionales (plantilla)

Completar **Observado** y **Evidencia** (nombre de la captura) durante la ejecución.
Capturas sugeridas: terminal `mosquitto_sub -t "dcandino/#" -v`, depuración de Node-RED, tabla en InfluxDB y panel de Grafana.

| ID | Escenario | Entrada | Resultado esperado | Observado | Evidencia |
|---|---|---|---|---|---|
| P1 | Lecturas normales | Temp ≈ 25 °C, luz 40–60 % | Estado NORMAL; datos en `ambiente`; Grafana verde | | |
| P2 | Advertencia | Temp 28.5 °C | Estado ADVERTENCIA (amarillo); sin alerta crítica | | |
| P3 | Crítico + alerta + persistencia | Temp 31 °C | Estado CRITICO (rojo); 1 evento en `alertas`; visible en tabla de Grafana y en InfluxDB con `_time` | | |
| P4 | Cambio de iluminación | luz 15 % → 85 % | Gráfico de iluminación refleja el cambio en < 10 s | | |
| P5 | Interrupción y recuperación | Detener publicación ≥ 30 s y reanudar | A los 15 s estado SIN DATOS + evento `sin_datos`; al reanudar vuelve a NORMAL/otro estado | | |
| P6 (extra) | Dato inválido | `temp = null` o 200 | Mensaje descartado, registrado en `lecturas_invalidas`, no contamina `ambiente` | | |

## Comparación publicados vs. recibidos
1. Elegir una ventana (ej. 5 min) y anotar el `seq` inicial y final en el monitor serie del ESP32 / simulador.
2. Contar mensajes con `mosquitto_sub` (broker) y filas en InfluxDB (consulta KPI 2a/2b de `influxdb/queries.flux`).
3. Calcular % recibidos = recibidos / publicados × 100.

## Tres indicadores propuestos
| Indicador | Cómo se mide | Resultado |
|---|---|---|
| Latencia de visualización | Tiempo desde publicar hasta verse en Grafana (cronometrar 5 veces; la latencia ESP32→Node-RED sale del campo `latencia_ms`) | |
| % de mensajes recibidos | Recibidos / publicados en ventana de 5 min | |
| Tiempo de detección de condición crítica | Diferencia entre primera lectura ≥ 30 °C y el evento en `alertas` | |

> No declarar ahorros ni mejoras reales: no existe línea base medida (lo exige el enunciado).
