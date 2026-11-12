# SysV en el modelo común ServiceInfo

Revisión140/K03. Consulta explícita, sin cambiar la selección automática de
Monitor ni el inventario/control de systemd existente. No ejecuta scripts ni
herramientas externas y no requiere un servicio SysV activo para leer archivos.

```cpp
#include <pcinsight/linux_sysv_services.hpp>

auto result = pcinsight::query_linux_sysv_service_inventory(
    "/etc/init.d", "/etc", 1024, 8 * 1024 * 1024);
if (pcinsight::metric_is_known(result)) {
    for (const pcinsight::ServiceInfo& service : result.value->services) {
        // service.name es el nombre de archivo candidato, NO daemon autenticado.
        if (service.linux_details.value) {
            const auto& description = service.linux_details.value->description;
            if (pcinsight::metric_is_known(description)) {
                // Texto declarado LSB; usar *description.value en la vista propia.
            }
        }
    }
}
```

## Contrato de proyección

`LinuxSysVServiceInventory.source` conserva el catálogo descrito139 completo:
raíces explícitas, candidatos, referencias de runlevel, disponibilidad, diagnósticos,
presupuesto e intentos. `services[i]` corresponde a `source.catalog.scripts[i]`
SOLO en este resultado. No se añaden filas por referencias externas/no resueltas.
El proyector privado es puro: no vuelve a leer archivos ni une por nombre.
Desalineación, nombres duplicados/inválidos o más4096 candidatos: sin resultado,
no se corre una descripción a otra fila ni se oculta el problema por truncado.

`id` es `sysv-file:` más codificación hexadecimal de los bytes del nombre.
Es opaco, ASCII, sin pérdida ni colisiones dentro de un catálogo. Está limitado
a las raíces de ESTE inventario; NO es clave global entre hosts/raíces/snapshots,
ruta, comando, nombre systemd o token de acción. Para mezclar inventarios la app
debe conservar su ámbito/proveedor. `name` conserva bytes del filename, que pueden
no ser UTF-8 ni texto seguro de terminal; la GUI debe escaparlos/representarlos.
La descripción LSB sí pasa por validación/normalización del parser138.

`linux_details.provider` = `sysv-file-catalog`; su Support es estimated porque
proyecta candidatos de archivos, no enumera servicios confirmados por un gestor.
`linux_details.name` describe el filename observado: available en censo coherente,
estimated en censo parcial. Esto NO identifica un daemon vivo.

`description` usa Short-Description declarada y disponible; si no está disponible, Description
declarada disponible. El campo común y el Metric contienen el mismo texto. Ambas
descripciones originales se conservan en source. Permiso denegado/ausencia y sus
notas se propagan; no hay fallback al nombre, comando ni texto inventado.
Catálogo parcial/inestable o fuente no available: descripción UNKNOWN aunque se
entregaran campos en el fixture; no se reutilizan valores de consultas previas.

Estado de ejecución: `status="unknown"`, Metric status sin valor. PID ausente,
main_pid UNKNOWN (no cero sustituto), group vacío/group_kind vacío; slice,
load_state/active_state/sub_state/unit_file_state UNKNOWN y strings comunes vacíos.
No se deduce running/stopped/enabled/disabled de S/KNN, modos ejecutables,
ausencia de enlaces, presencia de cabecera, pidfiles ni nombres de procesos.
No se convierte un runlevel en grupo Windows/systemd ni política efectiva.

Support exterior describe coherencia del inventario filesystem, NO cobertura de
servicios activos ni disponibilidad de todas las descripciones. Catálogo parcial
produce estimated; catálogo completo puede tener descripciones no disponibles por
presupuesto/permisos/header ausente. Un intento completo no garantiza un campo.
Siguen vigentes límites, reservas y guardas de identidad de139; no lectura de
cuerpo adicional, caché, worker, persistencia o deadline duro. Es necesario refrescar
para nueva observación; no snapshot atómico ni identidad permanente.

## Control e integración

Esta vista no añade control SysV. Sus IDs NO se pasan a control_linux_service,
que sigue siendo una API de systemd con selección explícita. Un filename con
aspecto .service tampoco autoriza a controlar una unidad systemd homónima.
La GUI debe seleccionar acciones por proveedor/capacidad, no por el texto del nombre.
La biblioteca no instala un init ni ejecuta scripts como vía de status/control.
OpenRC132/controlscript SysV continúan sin excepción autorizada; I07 observador
no cambia. No hay cambio del layout común ServiceInfo/LinuxServiceDetails.

## Evidencia y siguiente paso

`tests/linux_sysv_services.cpp`: proyección pura portátil, Short preferido/Long
fallback, campos no disponibles, permisos, Support contradictorio, censo parcial,
alineación/duplicados, IDs de nombres con bytes no UTF-8 y catálogo vacío.
Nativo Linux: árboles privados, asociación a referencia de distinto sufijo,
presupuesto0, enlace roto, FIFO omitido, sentinel de no ejecución y FD sin fuga.
MSVC valida proyección pura/argumentos/fallback, NO el colector Linux. Consumidor
del SDK instalado incluye el header público y enlaza el nuevo símbolo.

Validación140: Linux166/166 y40 repeticiones SysV, MSVC10 proyección pura/fallback,
dos consumidores instalados exit0. Logs work/sysv-services-*. Avisos iniciales
clockskew /mnt/c <1s conservados; incremental final limpio y suite completa,
sin modificar reloj/mtime. Pruebas locales, no CI remota ni aceptación init real.

K03 permanece E por integración de otros proveedores, control/vida y aceptación
real. Siguiente unidad funcional: proyección explícita de runit al mismo modelo,
manteniendo separados PID reportado, vida no demostrada y control por pin propio.
No se activa automáticamente Monitor ni se cambia a otro gestor cuando falla
la consulta explícita de systemd.
