# Persistencia explicita del historial observado Linux

Revision99, 2026-10-24. `pcinsight/linux_history_store.hpp` ofrece
`LinuxHistoryStore::{load,save,merge,retain_since,prune,reset}` para filas
`LinuxAppHistoryEntry`. Es el componente de persistencia de H04, NO un colector
de red por si mismo. Desde revision100, F03 alimenta el informe separado
`linux_network_app_history`; ver LINUX_NETWORK_APP_HISTORY.md. CPU y red usan
periodos/archivos independientes. H02/HistoryStore legacy y WindowsHistoryStore no cambian.

Conserva para CPU/red/notificaciones el valor opcional, `Support` y la nota.
CPU en nanosegundos, red en bytes acumulados, notificaciones en unidades, no
tasas. Un cero conocido se conserva como cero; una ausencia sigue ausente.
No guardar un informe exterior ausente como vector vacio: comprobar primero
`metric_is_known(snapshot.linux_app_history)`. Solo se guardan FILAS, no el
Support/nota del Metric exterior, ni snapshots F03/brackets/pidfds/baselines.
La persistencia no reconstruye recursos anteriores, identidades ni permisos.

## Uso C++20

```cpp
#include <pcinsight/linux_history_store.hpp>
#include <stdexcept>

pcinsight::HistoryIoResult guardar(
    const std::filesystem::path& path,
    const pcinsight::Metric<std::vector<pcinsight::LinuxAppHistoryEntry>>& report)
{
    if (!pcinsight::metric_is_known(report))
        return {false, "Informe ausente: no reemplazar por historial vacio"};
    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if (ec) return {false, ec.message()};
    pcinsight::LinuxHistoryLoadResult previous;
    if (exists) {
        previous = pcinsight::LinuxHistoryStore::load(path);
        if (!previous) return {false, previous.error};
    }
    try {
        const auto merged = pcinsight::LinuxHistoryStore::merge(
            previous.entries, *report.value);
        return pcinsight::LinuxHistoryStore::save(path, merged);
    } catch (const std::invalid_argument& e) {
        return {false, e.what()};
    }
}
```

El consumidor elige archivo/momento/retencion y serializa load/merge/save;
no hay escrituras de Monitor/Sampler, mantenimiento oculto, CLI, daemon ni
dependencias nuevas. Puede incluir el header/enlazar la biblioteca desde su
IDE durante el GUI; no exige crear el CMake del consumidor antes de tiempo.

## Identidad y fusion acumulativa

Clave `(id, period_start)` con tiempos normalizados a milisegundos. Snapshots
del mismo periodo REEMPLAZAN contadores, no se suman. Los antiguos se ignoran;
uno igual/nuevo no puede reducir un contador ni perder un valor conocido.
No se combinan campos de filas conflictivas para aparentar cobertura mayor.
`merge` lanza invalid_argument y no modifica entradas; save/load devuelven error.
`save` reemplaza el documento validado, NO hace merge ni verifica progresion
respecto al archivo anterior. Para actualizaciones acumulativas usar la secuencia
load/merge/save del ejemplo, bajo bloqueo del consumidor.

Cada entrada de merge/save y cada archivo debe contener claves unicas; dos
inicios que caen en el mismo milisegundo colisionan. `merge` normaliza tambien
sus tiempos de salida. Periodos distintos permanecen separados. Si se reinicia
un contador, usar un periodo nuevo, NO continuar el mismo con un valor menor.
Colecciones simultaneas/independientes no deben compartir id+periodo ni archivo
sin coordinacion. No mezclar CPU y red de colectores distintos por coincidir
una ruta: el informe de red tiene periodo independiente y sus propios brackets.

## Formato y seguridad de archivos

Formato Linux propio v1: magic8 `PCILHST\0`, version u32/count u32, filas con
strings length-prefixed, tiempos wall u64 en ms, metricas support/present/valor
opcional/nota; enteros little-endian. Al final FNV-1a64 de todo el cuerpo,
deteccion de corrupcion accidental, NO firma ni cifrado. Ninguna conversion
automatica desde legacy o Windows, ni promocion de sus contadores crudos.

Limites:64MiB por archivo,100000 filas,1MiB por string/nota. Rechaza checksum/
version/flags invalidos, truncamiento/trailing data, soporte incoherente con
presencia, CPU negativa/fuera de rango, tiempos invertidos y claves duplicadas.
Load fallido no expone filas parciales. Save rechaza reemplazar un destino
existente corrupto o de formato distinto y usa temporal hermano exclusivo,
flush/cierre y reemplazo de la primitiva compartida de H02. Un fallo antes
del reemplazo conserva el destino y no toca backups ajenos. No garantia de
durabilidad ante power loss ni fsync de metadata del directorio Linux; dependera
del FS. Saves concurrentes son last-writer-wins; requiere bloqueo del consumidor.

Retencion conserva periodos COMPLETOS cuyo fin es >=cutoff, no prorratea bytes.
`prune` valida antes de reemplazar; error/corrupcion preserva el archivo.
`reset` elimina SOLO el archivo elegido, devuelve errores y acepta ausencia;
es una accion destructiva explicita, no verifica formato antes de borrar. Usa
unlink de la entrada (enlace no se sigue); rechaza incluso directorios vacios,
sin borrado recursivo ni un chequeo de tipo separado sujeto a carrera.
No se retiene contenido de red, pero rutas/nombres/tiempos tambien pueden ser
datos privados: el consumidor es responsable del destino/permisos/cifrado.

## Verificacion

Fixture Release con checks activos: roundtrip de presencia/soporte/notas/cero/
CPU nanosegundos/red uint64_max,50replays sin doble cuenta, snapshot obsoleto,
decremento/perdida de metrica conocida, periodos distintos, normalizacion ms,
duplicados incluso archivo con checksum valido, todos los truncamientos de un
archivo muestra, corrupcion/flags/bounds, formato Windows/legacy preservados,
destino directorio y backup ajeno intactos, temporal limpio, prune/reset.
En Linux tambien hace roundtrip del reporte CPU REAL de Monitor con red/
notificaciones aun ausentes; NO captura eBPF. Resultados finales en el TXT,
logs work/linux-history-store-*. No layouts existentes ni semantica CPU nuevos.

Resultado99: Linux134/134 sin skips+30ejecuciones focales (tres almacenes10veces),
consumidor instalado incluye/enlaza nuevo header/API y ejecuta exit0. MSVC20
ejecuciones focales fuera del sandbox (almacen nuevo/antiguo10veces) pasan. Dentro
del sandbox ambos fallan reemplazo temporal AccessDenied5; no se cambio codigo
Windows ni se relajaron controles. Se conservan logs de los fallos: fixture
Linux usaba el archivo corrompido por otro caso (ahora native.bin separado),
fixture chrono cambio de100ns a1us representable en ambos clocks, y denegacion
de reemplazo dentro del sandbox. Sin nuevos warnings C++/D9025 esperado; N03
del consumidor Windows antiguo sigue abierto y NO se considera arreglado.
H04 pasa P->E, no C;77,3%/22,7% sin credito parcial. TXT es control oficial.
Espejo99 confirmado: comentario90140272938633; historicos remotos intactos.
