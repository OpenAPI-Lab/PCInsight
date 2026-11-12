# Tasas de red Linux: continuidad y reinicios

Unidad: corrección de `NetworkAdapter::receive_bytes_per_second` y
`send_bytes_per_second` obtenidos por Monitor desde sysfs, sin comandos externos.
No añade contabilidad por proceso/usuario/app, ni historial de notificaciones.

## Correcciones

Antes, el backend dividía diferencias de contadores retenidos por el intervalo
global de la última muestra de Monitor. Ese instante avanzaba incluso cuando
include_networks=false: al reactivar podía dividir tráfico de una pausa larga
por un intervalo corto. Además restaba uint64 sin comprobar reinicios, y retenía
interfaces desaparecidas. Ahora hay una línea base propia de red por interfaz,
reemplazada completamente en cada muestra correcta, con reloj steady_clock.

Una tasa exige dos muestras consecutivas del mismo nombre e ifindex no cero,
tiempo creciente y ambos contadores no decrecientes. Un reinicio de cualquiera
de las direcciones invalida las dos tasas del intervalo, sin wraparound, y crea
la nueva baseline. Lo mismo ocurre con identidad cambiada, tiempo igual o hacia
atrás. Las interfaces ausentes se olvidan; renombrar no conserva continuidad.
Desactivar include_networks limpia todas las baselines. Tras reactivar, la primera
muestra no tiene tasas. Fallos que abortan fill_networks también limpian el tracker.

Tasas en bytes/s, redondeadas hacia abajo, no bits/s ni porcentaje. Diferencia
cero entre muestras válidas sí produce valor0; ausencia de baseline no. Valores
fuera de uint64 se devuelven ausentes/unsupported en esa dirección, no saturados
ni convertidos fuera de rango. La conversión usa un límite superior exclusivo
2^64 y evita restar durations enteras con signo a través de todo su rango.
Las tasas válidas llevan Support::estimated y nota: son diferencias de contadores
de interfaz divididas por tiempo de muestreo; las lecturas sysfs no son atómicas.

## Lectura y límites

Por fila se lee ifindex, rx_bytes y tx_bytes, y se vuelve a leer ifindex para
rechazar un cambio visible durante esa lectura. Los números aceptan decimal
uint64 completo y whitespace final; negativos, signos, overflow, texto sobrante
y lecturas vacías fallan. Como los totales legacy de NetworkAdapter no son Metric,
una fila sin contadores/identidad válidos se omite en vez de publicar ceros. Al
omitirse se pierde su baseline, así que recuperarla exige otra muestra antes de
publicar tasas. El consumidor no debe interpretar un adaptador ausente como
tráfico cero. Un error global de enumeración continúa propagándose al caller.

El tracker rechaza IDs vacíos/con NUL, nombres mayores de4096B, duplicados y más
de100000filas; no publica resultado parcial. La enumeración limita a100000filas
aceptadas. No es un presupuesto total de RAM ni un deadline de filesystem. La lista
getifaddrs tiene liberación RAII, también ante excepciones al copiar direcciones.
No hay cambio público de layout ni nuevas dependencias.

Nombre+ifindex no es una identidad persistente: una interfaz recreada entre polls
con ambos iguales y contadores crecientes puede no detectarse. No se detectan
todos los resets entre muestras. Usar un Monitor dentro del mismo network namespace;
al cambiar namespace recrearlo. Lecturas de contadores, direcciones y metadatos no
son un snapshot atómico. Datos corresponden a lo visible en sysfs/getifaddrs del
caller y pueden incluir tráfico de otros procesos. No equivalen a bytes útiles
de una aplicación ni prueban velocidad negociada o capacidad física del enlace.
Actualizacion F02: modelo/tipo de conexion ya no usan la heuristica previa.
Ver LINUX_NETWORK_IDENTITY.md: informe con disponibilidad, ARPHRD/marcadores,
PCI/USB y procedencia; no identidad comercial exacta universal ni inferencia por
nombre. F01 conserva calculo de tasas y politica de contadores.

## Verificación

2026-10-15: Release Linux59/59 y Windows79/79; paquetes reinstalados y consumidores
externos reconstruidos/ejecutados exit0 en ambos. Test linux-network-rates con
assertions activas en Release: intervalos, cero medido, reset de cada contador,
cambio de ifindex/nombre, desaparición, reset explícito, reloj igual/atrás, tasa
fuera de rango, truncación de fracciones, límites de filas/nombres, duplicados,
parseo uint64 y rango completo del reloj sin overflow de duration.

Linux nativo: UDP entre un socket privado y sí mismo en127.0.0.1,16datagramas de
1024B, recepción acotada por poll; crecimiento de totales y tasas positivas en
loopback comprobados. Monitor opt-out/reenable deja tasas ausentes. No configura
interfaces, no envía tráfico externo, no requiere root ni instala aplicaciones.
Windows ejecuta los fixtures del cálculo portable, no la captura sysfs nativa.
No se ha probado hotplug/reset real de un adaptador físico; esos casos son fixtures.
