# Núcleo de medición de startup impact

Subtarea verificada 2026-10-14: lectura de contadores de proceso identificado y
clasificación de sus deltas entre dos muestras. API en startup_impact.hpp:
read_startup_process_sample(pid,linux_start_ticks) y measure_startup_impact(id,
before,after,policy). No observador automático del arranque ni asociación por
nombre/Exec inventada: el caller aporta la relación startup_entry_id -> proceso
y delimita la ventana. Monitor.startup_entries.impact sigue not_measured hasta
integrar correlación/observación completa, no cambia por esta API separada.

La unidad posterior StartupImpactObserver incorpora observación incremental con
asociaciones explícitas, sin scheduler oculto; ver STARTUP_OBSERVER.md.

```cpp
#include <pcinsight/startup_impact.hpp>
auto before = pcinsight::read_startup_process_sample(pid, start_ticks);
// Más tarde, en el límite elegido por el caller; sin sleep oculto en la biblioteca.
auto after = pcinsight::read_startup_process_sample(pid, start_ticks);
if (before && after) {
    auto measured = pcinsight::measure_startup_impact(entry_id, *before.value, *after.value);
    // measured.value->impact: Metric<StartupImpact>, no convertir unknown en low.
}
```

## Lectura nativa Linux

Procfs genuino (fstatfs) y PID namespace del mount compatible con getpid del caller.
Abre descriptor /proc/<PID> antes de stat/io/stat; no busca otro proceso si termina.
Compara PID y start_ticks de ambas lecturas con token esperado de ProcessInfo.
Contadores desaparecidos, malformados o identidad cambiada dan error explícito;
PID inválido/token cero rechazados. No señales, pidfd actions, CLI ni elevación.

Contexto de muestras: boot_id del kernel y dev/inode del namespace PID. Antes y
después deben coincidir contexto, PID y start_ticks; los timepoints steady_clock
son del mismo contexto/ejecución, no timestamps portables persistibles. Tokens
son de resolución de tick, no identidad global/autenticada ni prueba frente a un
caller que fabrique muestras. No usar para autorización de acciones destructivas.

CPU: utime+stime del proceso, sin sumar cutime/cstime ni duplicar guest_time;
convierte ticks mediante _SC_CLK_TCK, comprueba overflow y conserva cuantización
como estimated. CPU de hijos separados no está agregado por este módulo.
Disco: read_bytes+write_bytes de /proc/PID/io, sin sumar rchar/wchar ni descontar
cancelled_write_bytes. Es contabilidad del kernel, puede incluir I/O de hijos
esperados por el proceso; no equivale a todos los hijos vivos ni transferencias
físicas exactas/latencia. Página cache, writeback y filesystem afectan la semántica.
No mide red, memoria, readiness ni demora de login.

I/O protegido -> Metric ausente/permission_required; CPU legible se conserva.
No presupone cero cuando no hay permiso. Lecturas individuales hasta64 KiB.
CPU/I/O no son un snapshot atómico: CPU final se lee después de I/O, timestamp al
terminar; el intervalo y deltas son observaciones acotadas con skew, no integración
perfecta. Ningún worker/daemon/sleep se crea. Procfs/FS/syscalls pueden bloquear;
no hay deadline universal de lectura. No recupera contadores tras salida ni valida
correlación automática con Exec o un cgroup de startup.

## Clasificación portable

Policy configurable, inspirado en los [criterios de Microsoft](https://learn.microsoft.com/en-us/windows/compatibility/startup-apps),
sin afirmar igualdad con Task Manager Windows. Default explícito SI:
medium CPU300ms/disco300000 bytes; high CPU1s/disco3000000 bytes.
High si un delta conocido excede su umbral high. Si ambos contadores conocidos,
medium si alguno alcanza medium, de otro modo low. Igual a high sigue medium.
Un recurso high basta aunque el otro sea unknown: el otro no puede reducir high.
Sin ambos contadores y sin high conocido -> clasificación unknown, no low/medium.
Zero conocido en ambos es low, no none (disabled/no app es otro concepto).

Todos los resultados de impacto son estimated y describen SOLO esta ventana y
scope de proceso. Counter decrease/reset -> delta unknown; nunca negativo/clamp
ni diff unsigned envuelto. Values con support unsupported/permission_required no
se utilizan aunque contengan un número. Identidad/contexto distinto -> stale_target.
IDs vacíos/NUL/excesivos, CPU negativa, cronología no creciente/intervalo overflow y
políticas inválidas -> invalid_argument. ID de startup es opaque del caller, no
se verifica existencia actual del archivo. No se añade el delta a historias ni
se guarda/mezcla automáticamente; evitar contar dos veces intervalos superpuestos.

## Verificación y pendiente exacto

Linux Release28/28, Windows39/39; nuevo header instalado y consumidores externos
compilados/ejecutados. Windows verifica algoritmo portable y unsupported del lector
Linux, NO un proveedor nativo Windows de impacto.
Fixture Linux /tmp privado: child C++ por fork, identidad de Monitor, baseline,
carga1.1s CPU propia +4MiB escrituras/fsync, segunda muestra y clasificación high;
rechazo de token incorrecto y de proceso recolectado. Se termina/recolecta solo el
hijo propio; ningún autostart/app real se ejecutó ni modificó. Tests de límites,
umbrales exactos, permisos/unknown, contador reset, PID/token/boot distintos,
políticas/CPU/cronología inválidas y overflow de intervalo.

Pendiente para startup impact completo: correlación fiable de apps/procesos/
descendientes, observador temprano de login/arranque y definición de ventana,
captura de procesos de corta vida antes de salida, agregación sin doble cuenta,
persistencia/retención y enlace explícito con inventario. Esta subtarea cierra el
núcleo medidor, no el padre startup ni el startup impact total del proyecto.

Actualización: STARTUP_OVERLAY.md resuelve el enlace explícito, STARTUP_CHILD_EXIT.md
captura finales de hijos no recolectados y STARTUP_STORE.md guarda/carga capturas
cerradas históricas. El recorder temprano universal, origen fiable, descendientes,
agregación y mantenimiento/retención automáticos siguen pendientes.
