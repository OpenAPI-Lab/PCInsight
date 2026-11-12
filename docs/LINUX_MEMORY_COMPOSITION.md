# Composición y compresión Linux (C03)

`MemoryInfo::linux_details` añade datos propios de Linux con disponibilidad por
campo. No es composición Windows. El2026-10-19 el usuario respondió «prosigue»
a la propuesta concreta de aceptar la diferencia instalada-utilizable como
estimación etiquetada y desconocida sin datos válidos. Se registró esa aceptación
en contexto únicamente para C03, no para E06/L03 ni otras decisiones pendientes.

## Composición para gráficas

`linux_details.composition` ofrece tres segmentos estimados en bytes:

| Segmento | Cálculo |
|---|---|
| unavailable_estimate_bytes | MemTotal - MemAvailable |
| reclaimable_estimate_bytes | MemAvailable - MemFree |
| free_bytes | MemFree |

Solo se publican juntos si `MemTotal>0` y `MemFree<=MemAvailable<=MemTotal`.
Las restas se validan antes de calcular; los tres suman exactamente MemTotal
en esa muestra. El primero coincide con `in_use_bytes` de la API: complemento
de MemAvailable, no memoria de procesos ni una clase de páginas exclusiva.
El segundo es diferencia entre estimaciones, no garantía de recuperación inmediata.
MemAvailable lo estima el kernel; no se inventa en su ausencia.

No son estados Windows in-use/modified/standby/free. Falta de operando o relación
inconsistente deja composition ausente con nota, no una gráfica de ceros. Lecturas
independientes válidas siguen disponibles. Es vista del kernel, no cuota cgroup.

Se exponen también `free_bytes`, `buffers_bytes`, `page_cache_bytes` (Cached),
`anonymous_bytes` (AnonPages), `shared_bytes` (Shmem), `slab_reclaimable_bytes`
(SReclaimable), `slab_unreclaimable_bytes` (SUnreclaim), `dirty_bytes` y
`writeback_bytes`. Estas categorías se solapan: NO añadirlas entre sí ni a la
partición. `cached_bytes` legacy sigue siendo Cached+SReclaimable.
`paged_pool_bytes`/`nonpaged_pool_bytes` siguen ausentes; Slab no los sustituye.

Parser ampliado: MemFree/Buffers/AnonPages/Shmem/SUnreclaim/Dirty/Writeback/Zswapped,
manteniendo límites1MiB/10000líneas/64KiB por línea, kB=1024B, uint64 sin signo,
sin NUL/duplicados seleccionados/overflow. Fallo no publica mapa parcial. Aplicar
otra muestra limpia detalles y reserva anteriores, conservando tipo/slots/velocidad.

## Compresión: fuentes separadas

`zswap_pool_bytes` procede de Zswap; `compressed_bytes` legacy conserva ese
proxy estimated. `zswap_original_bytes` procede de Zswapped. Ausencia no demuestra
compresión desactivada;0 publicado sí se conserva. No sumarlos ni añadirlos otra
vez a uso de RAM. `zram_devices` enumera zramN visibles en `/sys/block`:

| Campo de LinuxZramMemory | Fuente mm_stat (bytes) |
|---|---|
| original_data_bytes | orig_data_size |
| compressed_data_bytes | compr_data_size |
| allocated_memory_bytes | mem_used_total, incluye metadata/fragmentación |

Lista válida vacía significa ninguno observado, no compresión global0. Una fila
puede tener estadísticas ausentes/permission_required; no tres ceros. zram puede
contener swap o filesystem; no mide RAM de una app. No se suma con zswap como
total universal ni se promete ahorro original-asignado: hay capas diferentes y
lecturas no atómicas. Las páginas iguales/incompresibles y el overhead importan.

Lectura read-only: directorio fijado, dev antes/después y mismo directorio al
final. Identidad no persistente; no detecta todos los resets/ABA entre extremos.
No compara deltas. Límites4096 entradas de bloques/256 dispositivos/4096B por
atributo/7..64 enteros en una fila mm_stat; tolera extensiones numéricas al final.
Rechaza unidades/signos/NUL/overflow/varias filas y atributos symlink/FIFO/no
regulares; O_NONBLOCK evita abrir FIFO bloqueante. Error de inventario no entrega
lista parcial; error de dispositivo conserva fila desconocida con razón.
Directorio inaccesible no equivale a vacío. errno distingue permisos cuando
hay evidencia. Límites no son deadlines. No eleva permisos ni crea dispositivos.

Se actualiza con cada muestra de memoria, incluso include_disks=false. No se
presenta caché como actual ni ejecutan zramctl/modprobe/dmidecode u otras apps.

## RAM instalada y reserva: alcance estimado aceptado

`firmware_installed_bytes` expone SMBIOS existente con procedencia/soporte; no
una suma parcial. Puede faltar/ser inaccesible/incorrecto; no acceso universal
al BIOS ni inventario vivo de hotplug. `installed_minus_usable_bytes` resta solo
MemTotal positivo disponible y no mayor que la RAM instalada válida. Diferencia0
conocida es0 estimated. Faltantes/contradicciones no producen resta, wrap ni
reserva total inventada. Denegación de firmware se conserva.

`hardware_reserved_bytes` legacy refleja la misma estimación y nota por
compatibilidad. NO mide exclusivamente hardware: la diferencia incluye también
kernel, límites de arranque o RAM offline. Preferir el nombre explícito nuevo en
el GUI. No sumar reserved de iomem indiscriminadamente: puede incluir rangos
fuera de RAM instalada. Este proveedor exclusivo no se declara implementado.
La aceptación2026-10-19 permite cerrar C03 con este límite ya previsto en su
criterio. NO implementa una nueva fuente exclusiva de reserva de hardware ni
acceso garantizado al BIOS. Validación física de firmware/compresión sigue N02.
Cuando alguna fuente es permission_required, la estimación no tiene valor y
conserva ese soporte/motivo, incluso si el objeto trae un valor contradictorio.

## API, compatibilidad y pruebas

```cpp
const auto& d = snapshot.memory.linux_details;
if (pcinsight::metric_is_known(d.composition)) {
    const auto& parts = *d.composition.value;
    // Dibujar con etiquetas Linux y conservar support/note.
}
if (pcinsight::metric_is_known(d.zram_devices)) {
    for (const auto& device : *d.zram_devices.value) {
        if (pcinsight::metric_is_known(device.memory)) {
            auto allocated = device.memory.value->allocated_memory_bytes;
            // allocated no es compressed_data_bytes.
        }
    }
}
const auto& difference = d.installed_minus_usable_bytes;
if (pcinsight::metric_is_known(difference) &&
    difference.support == pcinsight::Support::estimated) {
    // Etiqueta GUI: "RAM instalada menos utilizable (estimación)".
    // NUNCA "reserva exclusiva de hardware". Mantener note y unidades en bytes.
    auto bytes = *difference.value;
    (void)bytes;
} else {
    // Mostrar desconocido/permiso requerido, no 0 ni el último valor conocido.
}
```

La implementación inicial(revision9) amplió el layout público0.x de MemoryInfo
y, por contención, SystemSnapshot:
recompilar biblioteca Y TODOS los consumidores con cabeceras nuevas. Windows
no publica LinuxMemoryDetails; su proveedor nativo no cambia. No ABI estable.

`tests/linux_memory_composition.cpp`: partición/fronteras/faltantes/ceros,
campos solapados, parser ampliado, limpieza, reserva proxy coherente/incoherente/
denegada y límite uint64; mm_stat extensiones/bounds/overflow/texto/unidades.
Linux añade sysfs privado: dispositivo válido, stats ausentes/malformados,
atributo excesivo/FIFO/symlink, nombre no zram, directorio ausente y límite256
sin parcial. No crea dispositivos ni modifica swap real. Integración comprueba
partición e informe Monitor con discos desactivados. No fuerza compresión con
presión de RAM; fixtures positivos y disponibilidad nativa se distinguen.

Verificación2026-10-16: Linux80/80 y Windows100/100 Release, paquetes reinstalados
y consumidores externos recompilados/ejecutados exit0. Nativo WSL: segmentos
727801856B no disponible estimado,423112704B recuperable estimado y15406100480B
libre, total16557015040B. zram:0 dispositivos observados; Zswap y diferencia
instalada-utilizable ausentes. No se afirma positivo físico de compresión/BIOS;
los casos positivos provienen de fixtures. Se comprobaron límites256 dispositivos
y4096 entradas, y symlink de directorio válido como en sysfs. Advertencias de
formato/atributos de la compilación inicial corregidas; build final Linux sin
advertencias C++ ni clock-skew. Esto no cierra CI/hardware ni el riesgo previo N03.

## Revisión39 (2026-10-19)

La aceptación de alcance solo afecta C03 y no implica aceptar otras
aproximaciones del proyecto. Se conserva el criterio previsto(proveedor
exclusivo O aceptación del límite), no se marca como implementada la primera
alternativa. Continúan estimadas composición, MemAvailable/Zswap legacy y la
diferencia; pools Windows siguen ausentes. Nunca se certifica compresión0
porque no exista zram, ni reserva0 porque no se pueda leer SMBIOS.

Corrección adicional: apply_linux_memory_reservation conserva permission_required
y nota de la fuente utilizable, no solo de firmware. Nuevos fixtures comprueban
ausente/denegado/con valor contradictorio/unsupported, borrado de dato anterior,
diferencia0 con ambas fuentes medidas que sigue estimated, fuente utilizable
estimada y nota explícita NOT hardware-only. Alias legacy conserva valor/soporte/
nota, y el consumidor instalado verifica esos invariantes.

Sin layouts/ABI virtual/SystemSnapshot/historial nuevos, nuevas dependencias,
acciones privilegiadas, creación zram/swap ni presión de memoria de prueba.
Solo lecturas nativas y archivos sysfs privados del harness; positivos de
firmware/compresión siguen fixtures. Logs de esta revisión:
work/memory-accepted-{build,focused,suite,install,consumer}-*.log.
Regresión final: Linux108/108,Windows122/122 Release;10repeticiones focalizadas
Linux y ambos consumidores instalados recompilados/relinkados/ejecutados exit0.
Builds finales sin warnings. Nativo WSL: composición positiva,zram0observados,
Zswap/diferencia firmware ausentes. Es ausencia, no compresión/reserva global0.
No se declara corregida la causa de los fallos históricos CPU/apphistory N03.

## Fuentes primarias

[Definiciones de meminfo](https://docs.kernel.org/filesystems/proc.html),
[estadísticas zram](https://docs.kernel.org/admin-guide/blockdev/zram.html),
[ABI sysfs zram](https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-block-zram).
La partición propuesta y los límites son decisiones de la biblioteca, no nombres
ni garantías de composición dictados por el kernel.
