# Unidades e identidad para consumidores Linux (A05)

Estas reglas se aplican al SDK 0.2.0. No crean identificadores persistentes ni garantizan compatibilidad binaria 1.0.

| Dato | Unidad y uso |
| --- | --- |
| `*_bytes`, `*_bytes_per_second` | Bytes y bytes/s, respectivamente. Para KiB/s dividir por 1024; para Mbit/s multiplicar por 8 y dividir por 1 000 000. Convertir solo al presentar. No derivar tasas de inventarios acumulados sin validar identidad e intervalo. |
| `*_percent`, `average_response_ms` | Porcentaje (25 representa 25%, no 0,25) y milisegundos, respectivamente. No aplicar un máximo genérico de 100: respetar el alcance del proveedor. |
| `AppHistorySample::cpu_time`, `period_start/end` | Duración CPU en nanosegundos y fechas del periodo en `system_clock`. No confundir CPU acumulada con duración de pared ni `network_bytes` con una tasa. |
| `SamplerSample::finished_at` | Tiempo monotónico de finalización. Para graficar los últimos 60 s, filtrar por tiempo transcurrido, no por número de muestras. Dejar huecos para mediciones no disponibles. |
| `ProcessInfo::linux_start_ticks` | Token de inicio en ticks desde el arranque. No convertir a segundos ni a fecha para enviarlo a `terminate_process_checked`. |

## Continuidad de filas, no autorización

- **Proceso:** en un mismo flujo, arranque y espacio de nombres PID, usar PID y token de inicio disponible. Si falta el token o cambia, no continuar la serie como si fuera el mismo proceso. La resolución del token no lo hace globalmente único. Para acciones, dejar que la API verifique el destino; la clave de pantalla no basta.
- **Disco y red:** el `id`, nombre e índice de interfaz son referencias del inventario, no identificadores permanentes. Cortar la serie al perder la fila o cambiar de contexto. Una sustitución con el mismo nombre puede no ser visible al consumidor. Preferir tasas calculadas por el proveedor a derivarlas de filas unidas por nombre. Consultar [discos](LINUX_DISK_RATES.md) y [red](LINUX_NETWORK_IDENTITY.md) para las guardas específicas.
- **GPU:** PCI indica ubicación, no número de serie; los IDs de fabricante/dispositivo identifican modelo o familia, no una tarjeta individual. No unir GPUs por esos campos ni comparar índices de motor entre proveedores.
- **Servicio e historial:** conservar el proveedor junto al identificador de servicio; un PID principal no identifica al servicio. Para historial, preservar origen, ámbito de captura e intervalo; un nombre visible no permite unir aplicaciones ni sumar periodos solapados sin una política explícita.

La prueba instalada `linux-sdk-result-contract` verifica los tipos públicos seleccionados y ejercita continuidad de proceso sintética: cambio de nombre, reutilización de PID y tokens ausentes o estimados. Las conversiones de presentación y las reglas de los demás proveedores están documentadas aquí; la prueba no simula reemplazos físicos.
