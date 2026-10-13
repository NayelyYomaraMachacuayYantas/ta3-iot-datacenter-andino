# Configuración de InfluxDB 2.x (sin contenedores)

1. Instalar InfluxDB 2.x y la CLI `influx` (https://docs.influxdata.com/influxdb/v2/install/).
2. Iniciar el servicio (`influxd`) y abrir http://localhost:8086.
3. Configuración inicial (UI o CLI):

```bash
influx setup --org dcandino --bucket sala_servidores --username admin \
  --password <TU_PASSWORD> --retention 30d --force
```

4. Crear un token con permisos de lectura/escritura **solo** sobre el bucket:

```bash
influx bucket list                      # copiar el ID del bucket
influx auth create --org dcandino \
  --read-bucket <BUCKET_ID> --write-bucket <BUCKET_ID> \
  --description "TA3 node-red/grafana"
```

5. Guardar el token en `.env` (nunca en el repositorio) y pegarlo en:
   - Node-RED → nodo de configuración **InfluxDB local**.
   - Grafana → Data source InfluxDB.

## Modelo de datos

| Measurement | Tags | Fields |
|---|---|---|
| `ambiente` | device_id, sala, estado_temp, estado_luz | temperatura, humedad, luz_pct, luz_raw, seq, latencia_ms |
| `estado_monitoreo` | device_id | codigo (0 normal, 1 advertencia, 2 crítico, 3 sin datos), estado |
| `alertas` | tipo, severidad, device_id | temperatura, mensaje |
| `lecturas_invalidas` | device_id | motivo, crudo |
| `dispositivo` | device_id | estado (online/offline, vía LWT) |

La marca de tiempo (`_time`) la asigna InfluxDB al recibir la escritura desde Node-RED.
