# Run HKLM: vistas explicitas y control transaccional

Revision48, 2026-10-19. Consulta/listado nativos positivos de HKLM32/64; control
implementado y probado con modelo transaccional inyectado, pero **mutacion nativa
positiva pendiente**: la prueba privada recibe6801/ERROR_RM_NOT_ACTIVE sin writes.
No se modifica HKLM real, StartupApproved, configuracion KTM ni permisos para
forzar disponibilidad. M05 no se cierra por esta unidad.

## API y alcance

```cpp
#include <pcinsight/startup.hpp>
auto machine32 = pcinsight::list_windows_machine_run_registrations(
    pcinsight::WindowsMachineRunView::registry32);
auto machine64 = pcinsight::list_windows_machine_run_registrations(
    pcinsight::WindowsMachineRunView::registry64);
// Las filas contienen el ID winrun:hklm:32:HEX o winrun:hklm:64:HEX.
// Reconsultar por ID antes de ofrecer una accion:
// auto observed = pcinsight::read_windows_run_registration(row.entry_id);
// Mostrar HKLM/TODOS LOS USUARIOS + vista + limitaciones, pedir consentimiento.
// auto result = audited.set_windows_run_registration_enabled(*observed.value, false);
```

list_windows_run_registrations() y el informe include_windows_managed_run de
Monitor/Sampler siguen siendo SOLO HKCU/shared. No incorporan automaticamente
datos de maquina. El nuevo listado selecciona una sola vista por llamada; no
mezcla errores de una vista con filas supuestamente completas de otra. Enum
invalido deja Metric sin valor. En AMD64 se permiten32/64; en x86 nativo solo32,
sin reinterpretar64 como32; ARM64 sigue no soportado por la politica del proveedor.
Linux/otros devuelven requires Windows, no una lista vacia medida.

read_windows_run_registration y la accion explicita existente reconocen IDs
HKCU/shared y HKLM/32/64. Cambio de comportamiento0.x: un ID HKLM antes rechazado
puede ahora solicitar mutacion si el caller ya tiene permisos y KTM funciona.
El host debe informar del impacto sobre TODOS LOS USUARIOS y obtener consentimiento.
No es aprobacion efectiva de Windows, elegibilidad, autenticacion de editor,
prueba de quien creo un respaldo ni garantia de lanzamiento. La accion generica
set_startup_enabled de Windows no se redirige al proveedor.

## Hive, vista y respaldo

HKLM abre Software con un handle ordinario de solo lectura, en la vista elegida;
las operaciones posteriores son relativas a ese handle. No usa apertura vacia
de una clave predefinida como supuesto handle nuevo. Cada open/create de hijo
conserva explicitamente KEY_WOW64_32KEY o KEY_WOW64_64KEY; no construye rutas
Wow6432Node ni cambia la redireccion del proceso. Ver [contrato de vistas](https://learn.microsoft.com/en-us/windows/win32/winprog64/accessing-an-alternate-registry-view)
y [RegOpenKeyEx](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeyexw).

Run es HKLM/Software/Microsoft/Windows/CurrentVersion/Run y el respaldo es
HKLM/Software/PCInsight/StartupDisabled/v1/Run, ambos en la MISMA vista. No usa
HKCU como respaldo de una entrada HKLM, ni viceversa. HKCU conserva su mecanismo
anterior con RegOpenCurrentUser. Las etiquetas hive/vista del ID seleccionan
el destino solicitado; no son tokens criptograficos ni autorizacion. Si se
fabrica otro ID para una entrada distinta con identico estado/texto/bytes, la
comparacion de contenido no demuestra procedencia historica de la observacion.
No usar datos de un caller no confiable como permiso de un helper elevado (L05).

El lector toma dos observaciones de Run/respaldo y el listado dos enumeraciones
de ambos, conserva bytes REG_SZ/REG_EXPAND_SZ sin expandir ni ejecutar. Estados
registered/disabled_by_pcinsight/conflict/absent tienen el mismo significado
[limitado](WINDOWS_RUN_CONTROL.md). Datos invalidos, permisos o cambios detectados
dejan el Metric sin valor; ausencia de las dos claves tras abrir Software permite
una lista vacia conocida. Estimated/no atomico: no prueba ausencia de ABA ni
inmutabilidad despues del retorno. Lectura no crea claves ni requiere KTM.

## Acciones, permisos y auditoria

El mismo proveedor transaccional verifica estado/texto/tipo/bytes dentro de la
transaccion, escribe destino sin reemplazar y elimina el origen, con commit
unico. Fallos abandonan/rollback/cierran; commit fallido requiere reconsulta,
sin afirmar rollback confirmado. Se mantienen limites:64KiB de valor,10000filas
por pasada,16MiB de input/output del listado y260unidades UTF-16 para comandos
mutables. No se relajan limites para HKLM ni se añade fallback no transaccional.

Se solicita KEY_READ en Software, QUERY_VALUE para leer hijos y QUERY_VALUE/
SET_VALUE para acciones; crear respaldo depende tambien de la ACL del padre.
No se exige una raiz HKLM escribible ni se activan privilegios. ACL puede denegar
incluso a un proceso con capacidad de lectura; AccessDenied/PrivilegeNotHeld se
conservan como permiso requerido/permission_denied, no cero/deshabilitado.
RegCreate puede comprobar el descriptor del padre independientemente de la
mascara del handle; ver [RegOpenKeyEx, permisos](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeyexw).

No UAC, impersonacion automatica, otros perfiles, RunOnce/paquetes, herramienta
externa, cambios de ACL/reflection/KTM, reintentos elevados ni reparacion del
gestor de recursos. Poder enumerar no prueba que la accion se pueda ejecutar.

SystemActions y AuditedActions usan el metodo explicito existente, sin nuevo
virtual/enum AuditOperation. El target identifica hive/vista; command/native_data
siguen fuera del evento. Ahora se copia TODA la solicitud antes del callback
requested, fijando tambien hive/vista/contenido frente a cambios posteriores
del objeto original desde ese callback. No protege contra carreras durante
la copia; otros metodos mantienen sus propios contratos. Se conservan politica
fail-closed/best-effort, errores de sink y excepciones; no se inventa rollback.
ID/actor siguen siendo sensibles/no autenticados. No log persistente automatico.

API libre/enum adicionales, sin nuevos layouts de snapshots/opciones/payloads,
vtables ni formatos persistidos en esta unidad. Actualizar/recompilar consumidores
para el simbolo adicional; no se declara API/ABI1.0 cerrada.

El alcance describe la vista solicitada a Win32 en el contexto del host, no una
prueba universal de almacenamiento fisico/global. No se han certificado hosts
legacy32bits con virtualizacion del registro ni MSIX: pueden presentar otras
capas/vistas fusionadas. El host debe controlar su manifiesto/contexto; la
biblioteca no cambia esa configuracion ni atribuye autores a respaldos. Ver
[virtualizacion de compatibilidad](https://learn.microsoft.com/en-us/windows/win32/sysinfo/registry-virtualization).

## Evidencia

tests/windows_machine_run.cpp: redirige solo Software HKLM32/64 a dos ramas
privadas diferentes en HKCU, fuera de Startup. Las lecturas/enumeraciones son
reales; verifican bytes/datos distintos, backup-only, vistas en cada apertura,
sin API user/KTM, errores iniciales/tardios/privilegios inyectados, vista invalida,
x86/ARM64 simulados y cierre de handles. La separacion de ramas privadas no es
una mutacion de las vistas HKLM reales. El probe transaccional privado conserva
6801 sin writes en ambas; si un host permite transacciones, el test valida
retirada/restauracion solo en esas ramas privadas. Limpieza por claves/valores
exactos, sin borrado recursivo ni permisos de administrador.

tests/windows_run_control.cpp ejecuta el modelo transaccional completo tambien
para HKLM32/64: bytes exactos, set/delete/commit fallidos, rollback despues de
writes preparados, observaciones obsoletas/conflictos, limites y handles; exige
la vista correcta en TODOS los open/create. Es modelo, no validacion nativa
positiva de mutacion. windows_run_audit agrega regresion de callback que intenta
cambiar ID a HKLM y bytes: el backend recibe la copia anterior.

Consulta HKLM real read-only en este host:32bits1fila,64bits4filas y lectura
individual positiva en ambas sin KTM. Puede variar con el equipo/momento. No
se modifican valores HKLM reales ni se prueban ACL reales de escritura, UAC,
MSVC/ARM64/x86 fisicos u otros sistemas. Consumidores instalados enlazan el
nuevo listado sin acciones sobre entradas validas. Logs work/machine-run-*;
resultados finales en work/machine-run-pinned-*.log.

Resultado final Release: Windows126/126,Linux110/110;20repeticiones de machine-run,
run-control y run-audit Windows(60ejecuciones),20de run-audit Linux. Ambos
consumidores instalados recompilados/relinkados/ejecutados exit0. Builds finales
sin warnings, sin claves de fixtures residuales. Linux valida stub y auditoria,
no registro Windows; no equivale a CI remota ni corrige6801.
