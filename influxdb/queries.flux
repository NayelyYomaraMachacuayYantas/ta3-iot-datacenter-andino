// ===== Consultas Flux de la TA3 (bucket: sala_servidores) =====

// 1) Ultima lectura de temperatura
from(bucket: "sala_servidores")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "ambiente" and r._field == "temperatura")
  |> last()

// 2) Evidencia de persistencia con marca de tiempo (ultimas 20 lecturas)
from(bucket: "sala_servidores")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "ambiente")
  |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
  |> group()
  |> sort(columns: ["_time"], desc: true)
  |> limit(n: 20)

// 3) Alertas registradas
from(bucket: "sala_servidores")
  |> range(start: -24h)
  |> filter(fn: (r) => r._measurement == "alertas")
  |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
  |> group()
  |> sort(columns: ["_time"], desc: true)

// ===== INDICADORES =====

// KPI 2a) Mensajes RECIBIDOS en la ventana de prueba (ajustar start/stop)
from(bucket: "sala_servidores")
  |> range(start: -5m)
  |> filter(fn: (r) => r._measurement == "ambiente" and r._field == "seq")
  |> group()
  |> count()

// KPI 2b) Mensajes PUBLICADOS = (seq maximo - seq minimo) + 1  (valido si el ESP32 no se reinicio en la ventana)
from(bucket: "sala_servidores")
  |> range(start: -5m)
  |> filter(fn: (r) => r._measurement == "ambiente" and r._field == "seq")
  |> group()
  |> spread()
// % recibidos = recibidos / (spread + 1) * 100

// KPI 1) Latencia ESP32 -> Node-RED (ms): media, maximo y minimo
from(bucket: "sala_servidores")
  |> range(start: -5m)
  |> filter(fn: (r) => r._measurement == "ambiente" and r._field == "latencia_ms")
  |> group()
  |> mean()

// KPI 3) Tiempo de deteccion critica: primera lectura >= 30 C ...
from(bucket: "sala_servidores")
  |> range(start: -30m)
  |> filter(fn: (r) => r._measurement == "ambiente" and r._field == "temperatura" and r._value >= 30.0)
  |> first()
// ... versus la primera alerta "temperatura_critica" (la diferencia entre ambos _time es el tiempo de deteccion)
from(bucket: "sala_servidores")
  |> range(start: -30m)
  |> filter(fn: (r) => r._measurement == "alertas" and r.tipo == "temperatura_critica" and r._field == "temperatura")
  |> first()


// 4) Estado de iluminacion actual (BAJA / NORMAL / ALTA, clasificado por Node-RED en el tag estado_luz)
from(bucket: "sala_servidores")
  |> range(start: -1h)
  |> filter(fn: (r) => r._measurement == "ambiente" and r._field == "luz_pct")
  |> last()
  |> keep(columns: ["_time", "estado_luz"])
