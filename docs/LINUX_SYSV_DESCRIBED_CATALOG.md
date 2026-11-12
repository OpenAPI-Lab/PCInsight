# SysV: catálogo con descripciones ligadas al candidato

Revisión139/K03. Extensión opt-in; no cambia el catálogo137, que continúa sin
leer cuerpos. No ejecuta scripts, shell, CLI, status ni servicios reales.

```cpp
#include <pcinsight/linux_sysv.hpp>
auto result = pcinsight::query_linux_sysv_described_catalog(
    "/etc/init.d", "/etc", 1024, 8 * 1024 * 1024);
if (result.value) {
    const auto& view = *result.value;
    // view.metadata[i] corresponde a view.catalog.scripts[i] de ESTA consulta.
    // Comprobar cada Metric, incluidas short_description y description.
}
```

## Asociación e identidad

Entre los dos censos de inventario, cada candidato se selecciona relativamente
al directorio init.d fijado mediante O_PATH/openat2 NO_MAGICLINKS. Antes de leer
debe coincidir dev/inode/tipo/modo/tamaño/mtime/ctime con el candidato del primer
censo. El lector compartido con la API138 reabre solo ese descriptor propio,
comprueba regularidad/identidad, hace dos lecturas iguales del prefijo y revalida
metadata. No vuelve a seleccionar un archivo distinto por su nombre para I/O.

Después se revalida la asociación nombre/objeto dentro del directorio fijado.
El segundo censo, las raíces absolutas y el contexto del llamador deben seguir
siendo coherentes. Catálogo parcial/inestable: todas las descripciones quedan
UNKNOWN, con vector alineado a las filas del segundo censo; no se intenta unir
filas de snapshots distintos por basename. Cambio de contexto: sin resultado.
No hay snapshot atómico, garantía frente a ABA o actualizaciones posteriores.

Hardlinks pueden tener comentarios válidos en ambas filas; NO se elimina la
ambigüedad de script_index para una referencia que coincide con varios candidatos.
No se modifica la política de asociación de referencias137.

## Disponibilidad y presupuesto

El presupuesto predeterminado es8MiB y el máximo16MiB. Cero desactiva las
lecturas descriptivas. Cada candidato reserva antes de I/O dos veces
min(tamaño,64KiB), sin reembolso por error/ausencia de header. La suma reservada
no excede el presupuesto. No es medición de bytes físicos leídos ni disk usage.
Se recorre el orden de candidatos del catálogo; si uno no cabe se omite y puede
intentarse otro menor que sí quepa. No hay truncado silencioso de descripciones.
Los demás límites de inventario y del parser138 siguen vigentes.

`catalog.complete` y Support del resultado describen la coherencia del inventario,
NO disponibilidad de cada descripción. `all_metadata_reads_attempted` distingue
un intento para todos los candidatos de omisiones por presupuesto/identidad/
catálogo parcial. Un catálogo vacío estable da true; uno parcial da false.
True no garantiza header, permisos de lectura, campos presentes ni éxito del
lector. `prefix_bytes_reserved` es reserva conservadora, también si después se
descarta el resultado descriptivo por inestabilidad. Cada Metric conserva su
motivo de ausencia; permisos negados dentro del lector mantienen su Support.

Son comentarios declarados, no descripción autenticada de un proceso vivo,
publisher, PID, enablement, dependencia efectiva ni capacidad de control.
La API138 independiente sigue siendo una lectura fresca de un path, no un token
para enlazar retroactivamente con un catálogo viejo. Sin caché/worker/persistencia,
sin deadline duro de I/O. El host coordina fork/setns/credenciales y sus FD.

## Evidencia y pendiente

`tests/linux_sysv_described.cpp`: asociación por índice de esta consulta,
sufijo distinto, hardlinks ambiguos, header ausente, presupuesto0/insuficiente/
exacto, prefijo de archivo grande, catálogo parcial sin leer cuerpos, cambio de
archivo antes del pin y después de leer el pin, identidad esperada incorrecta,
reemplazo de raíz y conteo de FD. Hooks privados por llamada, sin override global.
Árboles privados con sentinel de no ejecución; FIFO init.d ignorado. MSVC solo
argumentos/fallback; no valida backend Linux. SDK enlaza el símbolo público.

K03 permanece E: esta asociación no integra todavía estas vistas en Monitor ni
prueba control/vida/aceptación con init SysV real. Revisión140 añade la
[proyección explícita a ServiceInfo](LINUX_SYSV_SERVICES.md), conservando
UNKNOWN para PID/vida/control no demostrados. La ejecución de scripts SysV y la
excepción OpenRC132 no están autorizadas; I07 observador permanece intacto.
