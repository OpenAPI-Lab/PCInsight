# Windows: rendimiento dinámico de discos

Implementado 2026-10-13 mediante PDH y contadores ingleses PhysicalDisk.
Cada Monitor posee su query. Se consulta el número de dispositivo mediante
IOCTL_STORAGE_GET_DEVICE_NUMBER, usando el handle de la interfaz de disco.
El prefijo numérico de la instancia PhysicalDisk se asocia a esa ruta del
inventario. No se usa `_Total` como si fuera un disco individual.

## Métricas

- read_bytes_per_second: Disk Read Bytes/sec.
- write_bytes_per_second: Disk Write Bytes/sec.
- transfer_bytes_per_second: suma de tasas de lectura/escritura publicadas,
  con verificación de overflow. Los enteros bytes/s se redondean hacia abajo.
- active_percent: 100 menos % Idle Time; fuera de 0..100 queda sin dato.
- average_response_ms: Avg. Disk sec/Transfer multiplicado por 1000.

Las cifras cubren el intervalo entre muestras, no un pico instantáneo. La
latencia es la media del contador de transferencias, no latencia de cada
operación ni promesa de que coincida exactamente con cualquier otro GUI.
Un cero válido se conserva; no se confunde con ausencia de soporte.

La primera muestra solo establece la base. Se requieren dos recolecciones
exitosas y datos PDH válidos. Cambiar cualquier relación número/ruta, vaciar
el inventario, desactivar discos o sufrir un fallo global de recolección
reinicia la query y exige una base nueva. Un error de enumeración también
invalida el proveedor dinámico.

## Asociación, errores y límites

Un número no disponible o compartido por varios endpoints no se asocia a
ninguno. Instancias duplicadas para el mismo número se consideran ambiguas.
La enumeración y PDH no forman una transacción atómica: hotplug puede cambiar
las relaciones entre llamadas. La invalidación de cambios observados reduce
mezclas; no elimina toda carrera ni garantiza identidad física permanente
en RAID/MPIO/virtualización que el controlador no publique correctamente.

Se validan los tamaños/conteos, punteros y NUL de los nombres en buffers PDH
(hasta 4 MiB/10000 entradas), CStatus y valores finitos/no negativos. Errores
de formato pueden afectar un contador o una instancia; quedan sin dato y no
se presentan como cero. Si falla abrir/agregar/recolectar la query, se cierra
y se intenta una query nueva en una muestra posterior. Acceso denegado se
etiqueta permission_required. Si falta uno de los cuatro contadores al crear
la query, este proveedor no publica las métricas dinámicas en esa muestra.

No se utiliza IOCTL_DISK_PERFORMANCE ni se ejecuta diskperf u otra utilidad
para habilitar contadores. Se consulta la infraestructura PDH disponible;
la configuración/proveedor del OS puede impedir obtener datos. La apertura
de query puede incrementar el coste de la primera muestra. No hay plazo
máximo propio para llamadas síncronas del proveedor del sistema.

## Verificación

Fixtures de base/deltas, métricas y conversión de unidades, cero, NaN/negativos,
overflow, porcentajes inválidos, CStatus inválido, buffers/nombres malformados,
duplicados, permisos, errores, cierre y recuperación, y reemplazo de relación
número/ruta. La integración real consultó tasas en 5/5 discos sin generar E/S
de prueba ni modificar discos; comprueba también desactivación/reactivación.
Windows Release 19/19 y Ubuntu WSL 6/6. Pruebas adicionales de hotplug/MPIO,
versiones de Windows y compiladores siguen pendientes.

Volúmenes/capacidad de filesystem, disco del sistema y page file ya tienen
proveedor en WINDOWS_VOLUMES.md. El medio reportado tiene proveedor en
WINDOWS_DISK_MEDIA.md, con respuesta positiva5/5 en actualización2026-10-19.
M03 cierra proveedores soportados con evidencia en WINDOWS_NETWORK_IDENTITY.md:
lectura/escritura/latencia5/5 y active_percent válido1/5 en muestra detallada.
Diagnóstico posterior:active5/5 sin cambio del proveedor; ambas muestras conservadas.
Disponibilidad de %IdleTime varía; valores fuera de rango siguen ausentes,
sin clamp/cero inventado. Otras topologías físicas no se anuncian probadas.

Referencias: [arrays de contadores PDH](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcounterarrayw),
[número de dispositivo](https://learn.microsoft.com/en-us/windows/win32/api/winioctl/ns-winioctl-storage_device_number),
[rendimiento y latencia de discos](https://learn.microsoft.com/en-us/windows-server/storage/storage-spaces/performance-history-for-drives).
