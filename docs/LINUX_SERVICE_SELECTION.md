# Selección explícita de servicios desde Monitor

Revisión144/K03. `SampleOptions.linux_service_selection`, también requiere
`include_services=true`. Solo Linux; Windows ignora esta selección. No acciones
nuevas, detección automática de init, instalación, CLI, shell o scripts.

```cpp
#include <pcinsight/monitor.hpp>

pcinsight::Monitor monitor;
pcinsight::SampleOptions options;
options.include_services = true;
options.linux_service_selection.provider = pcinsight::LinuxServiceProvider::s6;
options.linux_service_selection.root = "/service"; // Elegido por el host.
auto snapshot = monitor.sample(options);
if (pcinsight::metric_is_known(snapshot.linux_service_inventory_source)) {
    // snapshot.services usa ServiceInfo y sus Metric/notas linux_details.
    // Un censo disponible no demuestra vida de cada fila ni capacidad de control.
}
```

Las rutas son explícitas, no ejemplos de ubicaciones universales. Selección
desactivada si include_services=false, incluso con opciones inválidas/configuradas.

## Proveedores y argumentos usados

| provider | Argumentos de selección usados | Fuente |
| --- | --- | --- |
| legacy_default | Ninguno | Comportamiento anterior: systemd sistema y fallback parcial por archivos |
| systemd_system | Ninguno | Solo manager systemd del sistema |
| systemd_current_user | Ninguno | Solo manager del effective UID actual, sin fallback al sistema |
| sysv | init_directory, runlevel_directory, max_entries, sysv_prefix_budget | Catálogo SysV con comentarios LSB opt-in |
| runit | root, max_entries | Inventario runit/reportes de protocolo |
| s6 | root, max_entries | Inventario s6/records históricos y loggers |
| openrc_configured | init_directory, runlevel_directory, max_entries | Solo candidatos y referencias configuradas |
| openrc_marked | root, max_entries | Solo marcas runtime; no catálogo/script |

`legacy_default` es el valor predeterminado. Se conserva su comportamiento,
incluido el fallback histórico: NO se convierte en selección explícita silenciosa
ni se cambia la lectura vieja a contenido SysV. Los otros siete modos nunca
invocan otro proveedor ni el fallback por error/permisos/datos ausentes.

Campos no usados se ignoran: no abren otras raíces ni validan como si fueran
otro proveedor. Rutas usadas absolutas/sin NUL/hasta32768 bytes; max_entries1..4096
para proveedores filesystem. sysv_prefix_budget0..16MiB/default8MiB solo SysV:
cero evita leer descripciones. No afecta a systemd/runit/s6/OpenRC. Límites y
guardas nativos de cada proveedor se mantienen; el dispatcher no los mejora.

## Resultado y calidad

Solo se publican las filas del proveedor/vista seleccionado. Se preservan los
IDs/provider/Metric/etiquetas de las proyecciones anteriores. Snapshot no conserva
las estructuras raw de cada API tipada: para PID crudo/flags/relaciones/diagnósticos
detallados usar la consulta explícita tipada correspondiente y conservar su scope.
No se fabrican propiedades para llenar las columnas comunes.

`linux_service_inventory_source.value` identifica la fuente si hay resultado
conocido (también censo vacío conocido). Su Support refleja calidad del censo;
sus notas incluyen selección/scope y ausencia de fallback. Error/permisos/excepción/
argumentos inválidos: sin filas ni valor fuente, motivo explícito, no último dato
reutilizado. Parcial: filas parciales y Support estimated, no cobertura completa.
Disabled: sin filas/valor y nota disabled. Sin caché ni persistencia nuevas.

OpenRC selecciona UNA vista: configured no consulta runtime y marked no consulta
init.d/runlevels. No se unen las marcas con candidatos por basename ni se usa la
calidad de una fuente no solicitada para degradar una completa. Las vistas se
mantienen distintas. Para observar ambas independientemente, API143 tipada.

Los labels reported-* de runit, record-* de s6 y marked-* de OpenRC no significan
running confirmado, proceso vivo/readiness/control. PID común desconocido donde
la identidad no está probada; no se convierten PIDs crudos en targets de proceso.
Selección systemd no cambia su contrato de propiedades/manager/PID reportado.

## Acciones, compatibilidad y límites

Seleccionar un proveedor para Monitor NO selecciona un proveedor de acciones.
No pasar IDs/filenames de estas vistas a las acciones systemd/proceso. Cada API
de control conserva su selección fijada, autorización y contrato anteriores;
esta unidad no añade capacidades, no ejecuta scripts ni confirma jobs.
La exclusión de OpenRC en esta revisión144 es histórica: el usuario autorizó
solo el control explícito en148 y se añadió [la API separada](LINUX_OPENRC_CONTROL_PUBLIC.md)
en159. El selector de lectura no cambia; SysV sigue sin excepción e I07 observador intacto.

Se añade un campo al final de SampleOptions y nuevos tipos públicos en types.hpp:
**recompilar TODOS los clientes/bindings/plugins 0.x** contra la misma versión de
headers y biblioteca. No mezclar binarios con tamaños anteriores. No cambia el
layout de ServiceInfo/SystemSnapshot ni formatos de historial. No se requiere
configurar CMake en la app GUI para usar la API: este es el contrato C++ público.

No atomicidad entre samples, identidad permanente de path, garantía ABA o deadline
duro de I/O. El host coordina cambios de namespace/credenciales/fork y actualiza
su selección explícita. Por defecto no se cargan nuevos proveedores por esta opción.

## Evidencia y siguiente paso

tests/linux_service_selection.cpp: siete rutas por callback privado/por llamada,
un solo proveedor invocado, campos usados/ignorados, enum/roots/bounds/budget
inválidos sin consultas, error/permiso/excepción sin fallback, vacío conocido y
parcial sin valores anteriores. Linux Monitor real con árboles privados SysV/
runit/s6/OpenRC configured/marked, cambio de proveedor, fallo/switch sin datos
viejos, flag disabled, runtime ajeno no consultado y sentinel/FD sin fuga.
MSVC solo dispatcher puro, NO Monitor Windows ni backend Linux. SDK instalado
recompila dos consumidores y el nuevo consume la selección en Monitor Linux.

Validación144: Linux170/170 +10 repeticiones selector, MSVC10 dispatcher puro,
dos consumidores SDK recompilados/relinkados exit0, incluyendo el nuevo sample
Monitor Linux. Avisos clockskew iniciales /mnt/c preservados; incremental final
limpio/suite170 sin tocar reloj/mtime. Logs work/service-selection-*. No Monitor
Windows/N03 fix, CI remota ni aceptación con gestores init reales.

K03 sigue E por control/aceptación reales y alcance restante. Siguiente unidad:
auditar el cierre de K03 por proveedor contra evidencia y bloqueos reales, separar
lo terminado de lo aplazado/no autorizado y continuar L05/Linux cuando proceda;
sin declarar universalidad ni prolongar pruebas de hardware no disponibles.
