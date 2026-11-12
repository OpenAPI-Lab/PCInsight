# Informe opt-in de respaldos Startup en Monitor/Sampler

Revision45, 2026-10-19. `SampleOptions::include_windows_startup_file_backups`
(false por defecto) solicita el listado explicito de archivos retirados.
`SystemSnapshot::windows_startup_file_backups` contiene el mismo Metric con
WindowsStartupFileBackup (nombre UTF-8 + registration). No se mezcla con
startup_entries ni windows_run_registrations; las tres opciones son independientes.

```cpp
pcinsight::SampleOptions options;
options.include_windows_startup_file_backups = true;
pcinsight::Monitor monitor;
auto snapshot = monitor.sample(options);
const auto& report = snapshot.windows_startup_file_backups;
if (report.value) {
    for (const auto& item : *report.value) {
        // Mostrar item.name y item.registration.state.
        // Restaurar SOLO tras consentimiento mediante la API de control separada.
    }
}
// Para muestreo continuo, pasar las mismas opciones al constructor de Sampler.
```

Con la opcion desactivada no se invoca el proveedor de respaldos: Metric sin
valor, Support::unsupported y nota "disabled". No es una lista vacia medida.
Al activarla se conserva valor/soporte/nota del proveedor, incluidos errores,
permission_required y lista vacia conocida; no se reutiliza un informe anterior.
En Linux/otros sistemas el proveedor declara unsupported/requires Windows.
Un fallo representado por Metric no invalida por si solo los demas campos del
snapshot. Excepciones de asignacion u otros fallos del muestreo conservan el
contrato general de Monitor/Sampler; no se convierten en una consulta exitosa.

No crea carpetas, mueve/restaura archivos, pide UAC, consulta Run/KTM, ejecuta
aplicaciones ni cambia StartupApproved. Es metadato de una fuente particular,
no estado efectivo enabled/disabled de Windows ni garantia de lanzamiento.
Mantiene alcance local-drive/current-user, limites, estimated/no atomicidad y
restricciones del [proveedor](WINDOWS_STARTUP_FILE_CONTROL.md).
No es una implementacion de auditoria de acciones ni de botones genericos.

## Propiedad, muestreo y compatibilidad

Cada snapshot posee sus vectores/strings; desactivar muestras posteriores o
cambiar una copia obtenida por el caller no modifica snapshots anteriores ni
el estado interno de Sampler. latest/history/latest_timed/timed_history contienen
el informe en memoria como parte del snapshot. La ventana monotona de60s y sus
limites de capacidad no cambian. No se añade persistencia: HistoryStore,
WindowsHistoryStore y sus formatos/merge siguen sin serializar estos respaldos.

La enumeracion puede ser costosa y cada muestra retiene sus propios metadatos
(hasta16MiB de texto del proveedor, mas overhead). Usar una cadencia/capacidad
apropiada para el GUI; no se incorpora cache, deduplicacion compartida ni refresco
independiente por grupo. Tampoco hay timeout duro para llamadas nativas. Una
muestra combina consultas secuenciales: captured_at no certifica un instante
atomico de todos los datos. Sampler trabaja en su hilo, sin transferir de forma
implicita un contexto de impersonacion del hilo caller. No usar filas entre
cuentas como autorizacion de una accion.

**Cambio de layout0.x:** se agregan campos al final de SystemSnapshot y SampleOptions.
Recompilar/relinkar biblioteca y TODOS sus consumidores juntos; no mezclar
objetos/bibliotecas antiguos con headers nuevos ni usar sizeof como formato
persistente. No se promete ABI binaria estable0.x. Los tipos del proveedor se
mueven a types.hpp sin cambiar su definicion/layout; el header de control sigue
incluyendolos. No hay nuevos metodos virtuales, acciones ni cambios de formatos
persistidos. A05 (revision API/ABI1.0) sigue pendiente.

## Evidencia

tests/windows_startup_file_monitor.cpp, portable: proveedor interno inyectado
para comprobar que opt-out no lo invoca, lista positiva/empty/error sin coercion,
proveedor ausente, limpieza de valor previo y ownership. Monitor nativo read-only
opt-in/out independiente y Sampler con deadline5s para obtener una muestra,
latest/history/timed_history y copias independientes. No afirma positivos de
respaldos reales del usuario si su almacen esta vacio/inaccesible.

tests/windows_startup_file_control.cpp: el mismo llenado del informe recibe un
proveedor nativo redirigido a fixture privada con respaldo real, confirma fila/
identidad y desactiva sin tocar el archivo. No se modifica el inicio automatico
real. El consumidor instalado compila/enlaza los nuevos campos/opcion e invoca
Monitor sin acciones. Linux comprueba el stub y sus regresiones propias, no el
proveedor Windows. Logs work/startup-backup-monitor-*.log. No equivale a CI remota
ni a validacion de otros filesystems/arquitecturas.

Resultado Release: Windows124/124 y Linux109/109;20repeticiones por cada una de
las2pruebas Windows (monitor/control,40ejecuciones) y20del monitor en Linux.
Consumidores instalados ambos recompilados/relinkados/ejecutados exit0.
La compilacion inicial e incremental WSL emitieron Clock skew por archivos
compiler_depend.make con mtime0.1-0.2s futuro; logs originales conservados,
sin tocar reloj/configuracion ni ocultar el aviso. Ver tambien la ultima pasada
work/startup-backup-monitor-settled-build-linux.log; no atribuir causa interna
del desfase ni confundir un aviso de make con un error de compilacion C++.
La ultima pasada settled termino exit0, sin warnings ni recompilaciones/enlaces
adicionales; las suites finales validan los mismos binarios. Windows tambien
compilo sin warnings. El aviso previo no se borro de sus logs.
