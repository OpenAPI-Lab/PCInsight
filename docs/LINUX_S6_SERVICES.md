# s6 en ServiceInfo: evidencia de archivo, no vida actual

Revisión142/K03. Consulta explícita de un árbol de escaneo s6 confiado por el host,
sin cambiar Monitor ni consultas systemd/SysV/runit. Sin detección automática de
init, CLI, shell, scripts, instalación ni solicitudes de control.

```cpp
#include <pcinsight/linux_s6_services.hpp>
auto result = pcinsight::query_linux_s6_service_inventory("/service");
if (pcinsight::metric_is_known(result)) {
    for (const auto& service : result.value->services) {
        const auto& status = service.linux_details.value->status;
        // Comprobar Metric; las etiquetas record-* NO son running/readiness.
        (void)status;
    }
}
```

La ruta es un ejemplo, no una ubicación universal. `source` conserva el inventario
s6 original, sus records/diagnósticos y metadatos de loggers/FIFO. `services[i]`
corresponde solo a `source.entries[i]` en esta llamada. El proyector privado es
puro, no vuelve a leer archivos ni cruza snapshots por nombre.

## Campos y disponibilidad

Proveedor `s6-status-record-inventory`, Metric linux_details estimated por tratarse
de reportes de archivos y no enumeración autenticada de un gestor activo. Nombre:
entrada relativa observada, available en censo coherente o estimated si parcial.
IDs opacos `s6-record:` más hex de los bytes del nombre relativo; el separador
del logger también se codifica. Sin colisiones dentro de ese inventario, NO
identidad global, ruta, comando ni token para acciones. La GUI debe representar/
escapar filenames con bytes no UTF-8/controles. Scope lógico no implica confinamiento
físico: aliases de directorio pueden resolver fuera del árbol.

Status proyectado, solo para record available y censo coherente:

- PID crudo distinto de0: `record-pid-present`;
- PID crudo0: `record-pid-zero`;
- finishing añade `/finishing`, paused añade después `/paused`.

Metric status es estimated con nota de origen histórico. Ninguna etiqueta afirma
supervisor/proceso vivo, running/stopped, readiness o peer autenticado. No se
publica ready como conclusión: `ready_reported` con PID0 sigue siendo un record
válido. want_up tampoco se interpreta como enabled. Los dos TAI64N permanecen
crudos, no Unix/uptime. Nanosegundos inválidos hacen status UNKNOWN; un u64 PID
grande NO se trunca a32 bits ni se rechaza como record solo por no caber en PID local.

PID común ausente y main_pid UNKNOWN siempre. PID u64 y wait word u16 íntegros
quedan en source: no número de host verificado, token de proceso ni exit code.
No se siguen /proc/pidfiles ni se envían señales por ese número. Descripción y
grupo vacíos; description/load_state/active_state/sub_state/unit_file_state/slice
UNKNOWN, group_kind vacío. Logger/parent_name es asociación de un nivel en
source, NO grupo Windows/systemd/credencial ni metadatos del script.

Fuente sin record available, permisos o censo parcial/inestable: status unknown
y Metric sin valor con motivo/permiso. No último valor reutilizado. Support
exterior describe coherencia del inventario filesystem, no cobertura activa:
un censo completo puede contener entradas unmanaged/records no disponibles.

## Alcance, guardas y acciones

Se conserva query_linux_s6_inventory: directorios/aliases visibles directos más
UN nivel log, límites1..4096 filas/16384 visitas/8MiB payload y64 diagnósticos,
records35 bytes dobles y guardas de pin/contexto/metadata. El proyector rechaza
nombres/paths/duplicados incoherentes, logger anidado o sin padre directo en
el resultado y payload excesivo. No corrige ni reasocia una fila desplazada.

`real_control_fifo` de source indica SOLO tipo/presencia, no permisos, lector,
versión, supervisor vivo o capability. No se abre escritor ni se escribe para
comprobar vida/control. Un archivo status sin supervisor sigue siendo histórico;
la proyección no aumenta su garantía ni la convierte en un heartbeat fresco.
Sin nuevo I/O, worker, caché, persistencia, snapshot atómico/ABA o deadline duro.

La vista no devuelve handles de control. Sus IDs no se pasan a systemd ni su PID
a acciones de proceso. LinuxS6Control conserva selección fijada/consentimiento y
su contrato de entrega separado; LinuxS6Observation conserva condiciones crudas,
NO completion/readiness/causa. Un path antiguo no autoriza a actuar sobre cualquier
reemplazo. Sin cambios a contratos133..136 ni al layout ServiceInfo común.

## Evidencia y siguiente unidad

tests/linux_s6_services.cpp: proyección pura, parent/logger y orden conservados,
u64 máximo sin truncar, PID0+ready válido, finishing/paused, timestamps inválidos,
permisos/Support/partial, IDs rawbytes, aliases lógicos, duplicados/logger huérfano/
anidado/directorio fuera de scope y vacío. PID común siempre ausente.
Linux privado: directorio alias, UN nivel log frente a log/log ignorado, fila
unmanaged, bound, logger roto, FIFO sin bytes y luego sin lector, records sin
supervisor real, sentinel de script no ejecutado y conteo de FD. MSVC prueba
proyección/argumentos/fallback, NO backend Linux. SDK instalado enlaza el símbolo.

Validación142: Linux168/168 +50 repeticiones s6, MSVC10 puro/fallback y dos
consumidores SDK instalado exit0. Logs work/s6-services-*. Avisos clockskew
iniciales /mnt/c <1s conservados; incremental final limpio y suite168 sin tocar
reloj/mtime. Pruebas locales, no CI remota ni aceptación s6 real.

K03 sigue E por OpenRC/proveedores, integración y aceptación con gestores reales.
Siguiente unidad funcional: proyección explícita del catálogo/observaciones OpenRC
al modelo común, sin convertir marcas/directorios en liveness ni ejecutar scripts.
OpenRC132/controlscript SysV sin excepción autorizada; I07 observador intacto.
