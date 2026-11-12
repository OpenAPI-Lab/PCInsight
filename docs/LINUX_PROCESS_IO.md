# E/S de disco por proceso Linux

Actualización J04 parcial: [LINUX_USER_IO.md](LINUX_USER_IO.md) añade UID real
observado alrededor de las lecturas y entre extremos del intervalo para atribuir
la tasa al usuario. No cambia el cálculo D02 por proceso; una tasa puede seguir
conocida mientras su atribución a usuario permanece desconocida.

`ProcessInfo.disk_bytes_per_second` usa la suma de deltas separados de
`read_bytes` y `write_bytes` de `/proc/PID/io`, dividida por el intervalo
monotónico propio de ese proceso. Bytes/s enteros redondeados hacia abajo;
`Support::estimated`, incluido cero observado. No son rchar/wchar ni porcentaje.

Cada lectura fija el directorio con O_PATH y lee stat/io/stat mediante openat,
sin seguir symlinks finales. PID y start_ticks deben coincidir con la identidad
esperada antes y después. Archivos limitados a64KiB, io a10000líneas, enteros
uint64 estrictos y campos seleccionados únicos. No programas externos.

Primera muestra, identidad distinta, contador individual decreciente, reloj
sin avance, tasa fuera de uint64, lectura ausente o denegada: sin valor, nunca
cero inventado. Una ausencia borra la referencia; omitir procesos/historial/
usuarios también reinicia las referencias. Cada observación registra su reloj
después de leer sus contadores, sin depender del reloj global ni ticks CPU.
El tracker valida máximo100000 filas y rechaza PID0/duplicado reiniciando estado.

## Semántica y límites

La contabilidad del kernel no es throughput físico instantáneo del disco:
write_bytes se carga al ensuciar páginas, puede diferir de escrituras completadas,
y no se resta cancelled_write_bytes arbitrariamente. Compartición y contabilidad
de hijos esperados limitan atribución. Lecturas no son transacciones atómicas;
en32bits los contadores64bits pueden rasgarse. El token no identifica exec ni
es global entre boots/namespaces y tiene resolución de ticks. No se promete
coherencia de todos los campos de la fila, E/S por dispositivo ni red por app.
Referencia: https://www.kernel.org/doc/html/latest/filesystems/proc.html
sección3.3, /proc/PID/io.

## Verificación 2026-10-15

Fixtures: campos/unidades/duplicados/NUL/bounds, ceros, overflow, identidad,
reinicios independientes aunque crezca la suma, pausas/permisos, reloj igual/
inverso/extremos y validación de lotes. Prueba nativa WSL Linux: PID877,
start_ticks7767, lectura delta0 y escritura delta131072B, tasa estimated
66345749B/s durante una escritura/fsync de archivo privado. No benchmark físico.
Monitor verifica primera/segunda muestra y reset opt-out. No cambios a procesos
ajenos, caché global ni permisos. Denegación se valida sintéticamente.
Suite Linux74/74 y Windows94/94 Release. Sin nuevo layout
público; instalación/consumidores no repetidos en esta unidad.
