# OpenRC en ServiceInfo: candidatos y marcas separados

Revisión143/K03. Consulta explícita y de solo lectura; no cambia Monitor ni
systemd/SysV/runit/s6. Sin librc, CLI, shell, instalación o ejecución de scripts.

```cpp
#include <pcinsight/linux_openrc_services.hpp>
auto result = pcinsight::query_linux_openrc_service_inventory(
    "/etc/init.d", "/etc/runlevels", "/run/openrc");
if (pcinsight::metric_is_known(result)) {
    // Dos vistas ServiceInfo de fuentes independientes, NO identidad combinada.
    const auto& configured = result.value->configured_services;
    const auto& marked = result.value->marked_services;
    (void)configured;
    (void)marked;
}
```

Rutas de ejemplo, no precedencia universal de configuración. El llamador elige
los tres ámbitos explícitos. Cada colector mantiene sus guardas de contexto,
pins/metadata/dos censos y límites anteriores. El host serializa cambios de
credenciales/setns/fork. Son observaciones sucesivas independientes, no snapshot
combinado/atómico ni prueba de que corresponden a la misma instalación/boot.

## Sin unir identidades por nombre

`catalog` conserva el Metric original del catálogo131; `markers`, el Metric del
lector de marcas130. `configured_services[i]` corresponde a catalog.scripts[i];
`marked_services[i]`, a markers.entries[i], solo en esta llamada. La vista de una
fuente no disponible queda vacía, conservando su error/permiso en el Metric fuente.

Un archivo alpha y una marca alpha producen DOS filas distintas, nunca una fila
fusionada. Los enlaces de marcas no se siguen ni autentican su target: el nombre
no demuestra que una marca describa ese candidato ni su versión actual. Una marca
ghost sin candidato y un candidato beta sin marca se conservan por separado.
No se deduce estado stopped de la ausencia de marcas ni se borra una observación.

IDs opacos `openrc-file:` y `openrc-mark:` más hex de los bytes del nombre,
sin colisiones dentro de cada vista y distintos entre roles. Scope es el catálogo
de dos raíces o la raíz runtime, respectivamente; no ID global, path, comando ni
token de acción. La GUI debe escapar/representar nombres con bytes no UTF-8 o
controles. No se sugiere fusionar por el texto de name.

## Campos

Proveedor `openrc-file-catalog` o `openrc-runtime-marks`, linux_details estimated:
observación filesystem, no inventario de un gestor activo autenticado. Nombre
observado available en su fuente completa/coherente, estimated si parcial.

Configurados: status unknown y Metric status sin valor, incluso con execute bits
o referencias de runlevel. Descripción y grupo vacíos, PID ausente/main_pid UNKNOWN.
No shell parser para description ni evaluación de dependencias/defaults.

Marcas: primary única y coherente con mask produce `marked-started`,
`marked-stopping`, `marked-starting` o `marked-inactive`, Metric estimated.
Son nombres de marcas, NO running/stopped, proceso vivo, readiness o crashed.
failed/hotplugged/wasinactive y máscara original se conservan en source, sin
convertir failed en causa de salida ni hotplugged en grupo. Primary ausente,
conflictiva, bits desconocidos o estado incoherente -> UNKNOWN. Permisos conservados.
No PID: no se siguen targets, pidfiles ni /proc por nombre.

Ambas vistas: group_kind vacío, slice/load_state/active_state/sub_state/
unit_file_state UNKNOWN y strings comunes vacíos. Runlevel/stack/directorio no
se convierten en grupo ni enablement efectivo. No propiedades systemd inventadas.

## Calidad, límites y control

Cada fuente se evalúa por separado. Un catálogo parcial/no disponible NO invalida
una marca completa independiente, ni una marca parcial invalida nombres de un
catálogo coherente. Esto no autoriza a asociarlos. Support exterior available
solo si ambas fuentes están available y completas; estimated si hay al menos una
fuente conocida pero alguna incompleta/no disponible. Si ninguna tiene datos
conocidos, no hay resultado; permiso requerido si lo informa alguna.
Available NO significa cobertura de servicios activos o autorización de control.

`max_entries_per_source`1..4096 (default1024) se aplica independientemente a
scripts del catálogo y nombres de marcas; hasta8192 filas comunes combinadas,
no un límite global compartido. Se preservan los límites de visitas/payload/
runlevels/referencias de cada colector. El proyector puro rechaza raíces/nombres/
duplicados y bounds incoherentes; sin lecturas de cuerpos, joins o I/O adicional.

No handles ni capabilities. No se pasan IDs/nombres de esta vista a systemd,
acciones de proceso o scripts. La API no crea directorios runtime, ejecuta librc,
consulta status por scripts ni llama start/stop/restart. La excepción OpenRC132
sigue sin autorizarse: «prosigue» no la concede. I07 observador intacto.
Sin cambios de layout común, persistencia, workers/caché, garantía ABA o deadline
duro. Refrescar mantiene observación, no identidad permanente de un pathname.

## Evidencia y siguiente unidad

tests/linux_openrc_services.cpp: vistas separadas de nombre idéntico/IDs distintos,
cuatro marcas/flags, conflicto/bits inválidos/mismatch, permiso, fuentes parciales/
no disponibles independientes, duplicados/IDs rawbytes/vacío. Linux privado:
candidatos con y sin marca, marca sin candidato, targets rotos/no correspondientes
no seguidos, referencia de runlevel por identidad conservada, bound, marca regular
rechazada, runtime ausente sin crear, sentinel script nunca ejecutado y FD sin fuga.
MSVC prueba proyector/argumentos/fallback, NO backend Linux. SDK instalado incluye
y enlaza el nuevo header/símbolo. No init OpenRC real ni control de servicios.

Validación143: Linux169/169 +30 repeticiones OpenRC, MSVC10 puro/fallback y dos
consumidores SDK instalado exit0. Builds Linux inicial/final limpios. Logs
work/openrc-services-*. Pruebas locales, no CI remota ni aceptación OpenRC real.

K03 sigue E por selección/integración y aceptación con gestores reales/control
no disponible. Siguiente unidad funcional: selección explícita del proveedor de
servicios en Monitor, reutilizando estas vistas y preservando el default systemd
y los límites de cada proveedor; sin acciones nuevas ni fallback silencioso.
