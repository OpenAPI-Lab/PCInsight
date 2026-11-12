# Control reversible de registros Run por PCInsight

Estado: proveedor implementado y flujo probado con modelo transaccional
inyectado; **validación nativa positiva pendiente**. En el entorno Windows actual
RegOpenKeyTransactedW devuelve6801 ERROR_RM_NOT_ACTIVE. Se verificó que ese caso
rechaza la acción como not_supported sin escrituras. No se reparó ni reconfiguró
el gestor de recursos del sistema para forzar disponibilidad. No cerrar la
subtarea enable/disable Windows como completada por estas pruebas.

Actualización: **lector de estado nativo terminado**, independiente de KTM.
La escritura anterior continúa pendiente; no se añadió fallback de mutación.
Se confirmó6801 también en consulta transaccional de una entrada HKCU real,
no solo en la clave privada. El nuevo lector ordinario sí obtuvo estado de esa
entrada, sin modificarla. Esto acota el fallo al camino transaccional observado,
no diagnostica la causa interna ni demuestra que todo Windows tenga KTM inactivo.

## API explícita, no aprobación Windows

Revision48 agrega [HKLM32/64 explicito](WINDOWS_MACHINE_RUN_CONTROL.md), con
listado separado y lecturas nativas positivas. Mutacion nativa sigue pendiente6801;
no modifica la aprobacion Windows ni convierte modelos inyectados en esa evidencia.

`pcinsight/startup.hpp` expone read_windows_run_registration(id) y
set_windows_run_registration_enabled(observed, enabled). HKCU/shared/Run del
usuario actual o HKLM32/64 por ID explicito emitido por inventario. Carpetas Startup, paquetes,
RunOnce, otros usuarios y ARM64 quedan fuera. No ejecuta comandos, usa shell,
eleva privilegios ni modifica StartupApproved. No instala utilidades; usa
Advapi32 y KtmW32 del sistema. API0.x: recompilar consumidores.

El GUI debe rotular «Retirar/restaurar registro Run mediante PCInsight», no
«aprobación de Windows». Las funciones genéricas SystemActions::set_startup_enabled
no se redirigen silenciosamente a este mecanismo. El nuevo método explícito
SystemActions::set_windows_run_registration_enabled delega al mismo proveedor y
AuditedActions lo envuelve por opt-in (WINDOWS_RUN_AUDIT.md). El host sigue siendo
responsable del consentimiento y del sink; la función libre no audita por sí sola.

```cpp
auto observed = pcinsight::read_windows_run_registration(entry.id);
// Presentar observed.value->state, procedencia y advertencia al usuario.
// Tras confirmación explícita del botón y en el mismo contexto de usuario:
// auto result = pcinsight::set_windows_run_registration_enabled(*observed.value, false);
// Consultar de nuevo por el ID retenido antes de ofrecer restauración (true).
```

WindowsRunRegistration conserva entry_id, command UTF-8 sin expansión,
native_type (REG_SZ1/REG_EXPAND_SZ2) y native_data exacto (incluido terminador
si existía). La acción compara estado, texto, tipo y bytes con la lectura actual
dentro de una transacción. No autoriza crear un comando arbitrario desde un
objeto fabricado: el origen debe coincidir. No es un token criptográfico, SID,
identidad histórica ni protección contra ABA con el mismo contenido. No pasar
el objeto entre cuentas/contextos de impersonación.

## Estados propios y recuperación

El lector observa dos ubicaciones, sin crear claves:

Usa RegOpenCurrentUser(KEY_READ), RegOpenKeyExW(KEY_QUERY_VALUE) y consultas
acotadas, sin crear/iniciar/confirmar/abortar transacciones. Lee Run y respaldo
dos veces y compara presencia, tipo y bytes de ambos. Solo devuelve valor si las
dos observaciones coinciden, con Support::estimated y aviso de no atomicidad.
Cambios detectados dejan el Metric sin valor, con nota para recargar; permisos
se conservan como permission_required. No detecta ABA, cambios posteriores ni
todos los estados intermedios coherentes entre ubicaciones. Un estado leído no
prueba que esté disponible la acción de escritura.

- registered: valor en Run, sin respaldo del mismo nombre.
- disabled_by_pcinsight: respaldo presente, Run ausente.
- conflict: ambos presentes; ninguna acción sobrescribe/elige uno.
- absent: ninguno presente; no se fabrica un registro para «habilitar».

Es presencia observada, no prueba de quién creó el respaldo. El almacén es
configuración editable por el usuario y no autentica autoría, confianza o seguridad.
Ni registered prueba que Windows vaya a ejecutar la entrada ni retirar esta
fuente impide que otro mecanismo/app la recree o lance el mismo programa.
StartupEntry.status del inventario no cambia de unknown a enabled por esta API.

Deshabilitar mueve el valor seleccionado a
HKCU\Software\PCInsight\StartupDisabled\v1\Run, conservando nombre/tipo/bytes,
y elimina únicamente ese valor de Run en la misma transacción. Restaurar hace
el movimiento inverso y elimina el respaldo solo al confirmar también la
restauración. Nunca sobrescribe un valor destino existente, aunque coincida.
Una llamada repetida con observación vieja devuelve stale_target, no éxito
idempotente inventado. Las claves vacías permanecen; no borra claves de aplicaciones.

Después de retirar, la entrada deja de aparecer en el inventario Run ordinario.
El caller puede conservar el ID para reconsultar/restaurar, o usar el nuevo
list_windows_run_registrations para recuperar IDs de respaldos desde el registro:
ver [listado y conflictos](WINDOWS_RUN_LISTING.md). Monitor/Sampler ahora ofrecen
informe opt-in independiente (WINDOWS_RUN_MONITOR.md); auditoría de la acción
explícita está integrada (WINDOWS_RUN_AUDIT.md), GUI sigue pendiente. No se presenta
esta unidad como un gestor de Startup completo.
El almacén permanece en registro entre ejecuciones; no depende de memoria
del proceso. No hay exportación/borrado masivo ni limpieza automática de respaldos.

## Transacciones y límites

Para la acción, RegOpenCurrentUser fija el hive del contexto actual de la llamada.
Se consultan las dos claves fijas con RegOpenKeyTransactedW. Si hace falta un destino,
RegCreateKeyTransactedW lo crea dentro de la transacción. RegSetValueExW y
RegDeleteValueW operan en handles transaccionales; CommitTransaction es el único
punto de éxito. Fallos abandonan/cierran la transacción, con RollbackTransaction
explícito y cierre RAII de claves/hive/handle. No hay fallback no transaccional,
reintentos destructivos ni segundo intento oculto al fallar commit.

Si commit falla se informa resultado no confirmado y se exige reconsultar;
no afirmar desde ese error que todo se restauró. Conflicto transaccional o
contenido/estado cambiado -> stale_target; acceso -> permission_denied; transacciones
no soportadas/gestor inactivo -> not_supported. Errores restantes conservan código
Win32. El lector usa Metric sin valor en fallo, no absent/disabled ficticios.
La acción vuelve a validar la observación estimada dentro de su propia transacción;
no degrada su garantía a las dos lecturas ordinarias del lector.

Límites: ID131128B; nombre hex canónico de UTF-8 estricto hasta16383 wchar_t,
sin NUL; lectura hasta64KiB por ubicación y tipo textual soportado, UTF-16 estricto
sin NUL intermedio/comando vacío. La lectura conserva strings no terminados usando
longitud explícita; la acción conserva sus bytes. Acciones se limitan a comandos
de hasta260 unidades UTF-16 por el límite documentado de Run; lecturas más largas
pueden observarse pero no se modifican. Timeout KTM5000ms no es deadline duro de
APIs/commit. Sin protección ante cambios posteriores a la confirmación.

## Evidencia de pruebas

Release: Windows73/73 y Linux54/54, instalación y consumidores externos
reconstruidos/ejecutados en ambos, exit0. El consumidor prueba solo llamadas
inválidas que no pueden mutar el sistema; no modifica Startup real. Linux valida
el enlace de la API y retorno unsupported, no el proveedor Win32. Pasar estas
suites no elimina el pendiente de transacción nativa positiva.

2026-10-15. Modelo inyectado: retirada/restauración de REG_SZ/REG_EXPAND_SZ y bytes
no terminados/Unicode/variables literales, conflicto de ambos lados, observación
vieja, respaldo alterado, destino existente, fallos query/set/delete/commit,
rollback tras writes preparados, límites/cadenas inválidas/API incompleta y
cierre de handles. **Esto no certifica transacciones reales exitosas**.

Prueba nativa de mutación usa solo una clave temporal privada fuera de Startup,
con redirección interna de rutas de test. La fixture crea su valor de prueba de
forma ordinaria; la acción transaccional obtiene6801 antes de cualquier write,
rechazada sin modificar ese valor. El lector ordinario sí observa la fixture y
una entrada real HKCU con Support::estimated. Si un host permite transacciones,
la prueba ejecutará
la misma batería en esa clave privada. Las dos claves vacías de pruebas abortadas
iniciales se comprobaron vacías y eliminaron por nombre exacto; la prueba final
limpia su clave privada sin borrado recursivo. Ninguna entrada real Run ni
StartupApproved fue modificada; no se lanzó ningún comando.

Regresión del lector: API inyectada sin begin/commit/rollback aún permite consultar;
rechazo de escritura sin esos callbacks, estados propios y cambio entre pasadas
verificados. La consulta nunca llama a set/delete/create ni requiere KTM. El
consumidor instalado Windows consulta HKCU de solo lectura cuando existe entrada.

Fuentes oficiales:
[múltiples operaciones de registro en una transacción](https://learn.microsoft.com/en-us/windows/win32/sysinfo/writing-and-deleting-registry-data),
[RegOpenKeyTransactedW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeytransactedw),
[CreateTransaction: timeout y abandono](https://learn.microsoft.com/en-us/windows/win32/api/ktmw32/nf-ktmw32-createtransaction),
[CommitTransaction](https://learn.microsoft.com/en-us/windows/win32/api/ktmw32/nf-ktmw32-committransaction),
[Run: límite del comando](https://learn.microsoft.com/en-us/windows/win32/setupapi/run-and-runonce-registry-keys).

Lectura independiente:
[RegOpenKeyExW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeyexw).
