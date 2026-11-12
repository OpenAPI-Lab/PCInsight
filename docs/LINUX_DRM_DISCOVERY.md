# Descubrimiento limitado de clientes DRM Linux

discover_linux_drm_clients, en pcinsight/linux_drm.hpp, descubre PID/fd visibles
sin que el consumidor los enumere manualmente. No modifica Monitor, lanza hilos,
ejecuta programas, instala dependencias ni abre dispositivos GPU.

```cpp
pcinsight::LinuxDrmGpuSampler rates; // conservar entre ticks del consumidor
pcinsight::LinuxDrmDiscoveryOptions limits; // 4096 procesos,65536 fd,512 lecturas
auto found = pcinsight::discover_linux_drm_clients(limits);
if (found.value) {
    auto selected_rates = rates.sample(found.value->observations);
    // Mostrar scope, errores y límites junto al resultado.
    // selected_rates NO representa utilización total de GPU.
} else {
    rates.reset(); // no conservar una base si no se pudo iniciar el barrido
}
```

## Recorrido y cobertura

Enumera entradas numéricas de /proc y /proc/PID/fdinfo. Para cada descriptor,
stat de /proc/PID/fd/N determina si es character-device; sysfs identifica su
subsistema. Solo candidatos DRM pasan al lector completo de LINUX_DRM_CLIENT.md,
que revalida proceso, cliente y PCI. No se interpretan pipes/sockets como GPU ni
se abre /dev/dri para crear un cliente de prueba. Las lecturas aceptadas conservan
duplicados para que LinuxDrmGpuSampler deduplique por PCI/client-id.

El resultado se marca estimated SIEMPRE, incluso sin fallos ni límites: procfs
puede ocultar procesos, namespaces/contenedores restringen visibilidad y el driver
puede no publicar contadores. No es un snapshot atómico de procesos ni hardware.

Contadores del barrido:

- processes_seen: procesos cuyo directorio de descriptores se intentó enumerar.
- descriptors_seen: entradas cuya clasificación se intentó.
- drm_candidates: descriptores clasificados DRM, incluidos los que luego fallan.
- directory_failures: fallos al listar fdinfo; incluyen salida del proceso y acceso.
- unclassified_descriptors: no se pudo determinar el subsistema/tipo.
- unreadable_drm_clients: candidato sin lectura válida de tiempos ns/identidad.
- permission_denials: operaciones denegadas, no número de usuarios únicos.

No todas las entradas enumeradas llegan a leerse si se agota un límite. Error
inicial de /proc devuelve resultado ausente; fallos posteriores dejan un barrido
parcial con contadores. Las observaciones contienen SOLO lecturas disponibles.
Una lista vacía no significa GPU inactiva ni utilización0. El consumidor puede
agregar ese subconjunto accesible mostrando su cobertura; nunca es carga global.

## Límites

Por defecto:4096 procesos,65536 descriptores,512 observaciones y presupuesto
cooperativo250ms. Configurables hasta65536/262144/2048/60000ms respectivamente,
con mínimos1. Límite adicional65536 registros de motor,128 por lectura: el lote
cabe en el agregador sin truncamiento posterior. Flags limit_reached y
time_limit_reached indican interrupción/límites de enumeración; started_at y
finished_at exponen el intervalo real. Se conserva una selección parcial explícita,
no se promete que sea el conjunto de PID más bajos o el de mayor carga.

Se comprueba tiempo entre operaciones, pero NO se interrumpe opendir/readdir,
stat ni una lectura de kernel/driver bloqueada. El presupuesto no es un timeout
duro. Windows devuelve unsupported sin escanear nada. La API es opt-in explícita;
Monitor/Sampler general no adquiere este coste automáticamente.

## Verificación y pendientes

Pruebas inyectadas de descubrimiento->agregación con dos clientes y descriptor
duplicado: segunda muestra20, no30. También permisos, falta de driver stats,
salida/identidad inválida, directorios inválidos, reloj retrocedido, límites de
procesos/fd/observaciones/motores/tiempo y parámetros inválidos.
Barrido nativo limitado WSL:8 procesos intentados,0 fd leídos,8 denegaciones y
limit_reached=true. Se informa como parcial, no ausencia de uso GPU.
Linux Release45/45 y Windows58/58; paquetes/consumidores externos ejecutados.
Build final sin las advertencias de asignación initializer_list del fixture.

LINUX_DRM_MONITOR.md añade integración opt-in con Monitor/Sampler y resumen de
cobertura sin vectores raw en el historial. LINUX_DRM_CLASSES.md clasifica pares
conocidos AMDGPU/i915/Xe. LINUX_DRM_CYCLES.md añade pares busy/total por ciclos
al mismo lector y pipeline; LINUX_DRM_MAXFREQ.md añade la alternativa maxfreq.
Pendientes: otros drivers/clases, cobertura
global y positivo físico DRM/sysfs AMD/Intel/NVIDIA. WSL no expone GPU DRM física;
el positivo inyectado no certifica hardware ni compatibilidad de todas las distros.
