# Archivos Startup comunes: alcance explicito para todos los usuarios

Revision47, 2026-10-19. El proveedor de archivos Startup soporta tambien IDs
`winfolder:common:` del inventario. Se resuelven mediante FOLDERID_CommonStartup,
no FOLDERID_Startup, rutas inglesas hardcodeadas ni exploracion de perfiles.
FOLDERID_Startup sigue correspondiendo exclusivamente a `winfolder:user:`.
No se interpretan IDs Run/HKLM como archivos ni se implementa StartupApproved.

## API y separacion de alcances

```cpp
// Solo lectura, sin crear la carpeta ni un respaldo:
auto shared = pcinsight::list_windows_common_startup_file_backups();
// list_windows_startup_file_backups() sigue siendo SOLO del usuario actual.

// Consultar una entrada comun del inventario o una fila descubierta:
// auto observed = pcinsight::read_windows_startup_file_registration(common_entry.id);
// Mostrar claramente el alcance TODOS LOS USUARIOS y obtener consentimiento.
// auto result = actions.set_windows_startup_file_registered(*observed.value, false);
// true restaura; se recomienda AuditedActions para registrar intento/resultado.
```

Lectura/accion seleccionan la carpeta por prefijo exacto del ID, verifican que la
ruta codificada sea una entrada directa de la carpeta correspondiente y comparan
las identidades observadas antes de mutar. El listado comun emite IDs comunes;
no se cambian a user ni se mezclan automaticamente con el listado de usuario.
Un ID de un alcance con ruta de otro no permite retargeting fuera de la carpeta
seleccionada. Si las dos carpetas del sistema estan redirigidas al mismo objeto,
pueden aparecer IDs por alcance distintos del mismo archivo: no se afirma que
representen apps independientes ni se ofrece una barrera de autorizacion por ID.

El informe opt-in actual de Monitor/Sampler sigue consultando SOLO los respaldos
del usuario. El host consulta el nuevo listado comun separadamente cuando lo
necesita. No se agregan campos/opciones a snapshots ni refresco automatico comun.
SystemActions/AuditedActions mantienen el mismo metodo explicito, sin nuevo
virtual ni nuevo valor AuditOperation: el target conserva el prefijo common.

## Permisos e impacto

**Retirar/restaurar un archivo comun puede afectar el inicio de TODOS los usuarios.**
El host debe mostrar ese alcance antes de pedir consentimiento. Este es un
cambio de comportamiento0.x: un ID comun valido antes rechazado ahora puede
ejecutarse si el caller ya tiene permisos. La accion generica set_startup_enabled
de Windows no se redirige a este mecanismo y los listados previos no se amplian
silenciosamente. Actualizar/recompilar consumidores para la funcion adicional;
sin nuevos layouts/ABI virtual/formatos persistidos en esta unidad.

Se usan los derechos del proceso/hilo actual, nunca un token elevado obtenido
por la biblioteca. No hay UAC, impersonacion automatica, cambios de ACL, habilitar
privilegios, reintentos elevados ni fallback al Startup personal cuando falla el
comun. Denegacion de lectura es Metric sin valor/permission_required; denegacion
de accion devuelve permission_denied del proveedor. Poder leer NO prueba poder
retirar/restaurar. Errores no se convierten en enabled/disabled ni listas vacias.

El respaldo es PCInsight.Disabled.v1 dentro de la carpeta del alcance elegido,
no un almacen privado del usuario que invoca la funcion. No borrar ni cambiar
permisos del respaldo. El proveedor no autentica autores, firmantes, contenido
seguro o proteccion de las ACL del archivo/almacen. En particular, un helper
privilegiado no debe usar un ID/observacion/actor_label como autorizacion para
publicar en Startup comun archivos aportados por usuarios no confiables. L05
(helper/requester/consentimiento) sigue pendiente; esta API no lo sustituye.

Se conserva el [contrato checked de archivos](WINDOWS_STARTUP_FILE_CONTROL.md):
rename unico por handle sin reemplazo, sin copiar/borrar contenidos, sin KTM;
indices/tiempos/tamano no son identidad criptografica ni garantia ante ABA.
No modifica ni detiene procesos, ni cambia aprobacion Windows/otras fuentes.
Solo rutas de unidad local; UNC/unidad de red/reparse final siguen rechazados.
Mismo limite de registros/texto, estados estimated/no atomicos y errores sin
resultados parciales. El almacen puede quedar vacio tras restaurar, sin limpieza
automatica. La biblioteca no aplica expresamente nuevas ACL: no se inventa
una ACL mas segura ni se garantiza que nadie mas pueda modificarlo despues.

## Evidencia y limites de validacion

tests/windows_startup_file_control.cpp agrega dos carpetas privadas distintas,
con nombres de fixture unicos incluso en el mismo milisegundo. La inyeccion
interna redirige solo Known Folder/alcance: NtCreateFile/rename son reales.
Retirada/listado/restauracion comunes auditados positivos, conflictos sin
sobrescribir, prefijo incorrecto con ruta correcta rechazado, raiz equivocada
rechazada sin retargeting, listado user no incluye comunes, enum invalido y
error de permisos de enumeracion inyectado preservado. Se comprueban bytes al
retirar/restaurar. No se cambian permisos de fixtures ni directorios reales.

La nueva funcion publica usa Known Folder real solo para lectura; consumidor
instalado la enlaza y valida prefijos si hay filas. No se crean entradas comunes
reales ni se ejecutan archivos. La prueba positiva en fixture escribible NO
certifica permisos de ProgramData reales, UAC, otras politicas/ACL, filesystem
distinto o que un usuario no elevado pueda escribir en el Startup comun real.
No hubo prueba de mutacion elevada ni de una denegacion ACL real en esta unidad;
permission_required se verifica con la API de enumeracion inyectada y el
proveedor sigue dejando la autorizacion de acciones al kernel.

Logs work/startup-common-*.log. Esto completa el alcance common del proveedor
de ARCHIVOS, no HKLM Run, StartupApproved ni el criterio completo M05.

Validacion Release: Windows125/125 y Linux110/110;20repeticiones de cada prueba
Windows control/audit/monitor(60ejecuciones). Ambos consumidores instalados
recompilados/relinkados/ejecutados exit0, builds sin warnings. Linux verifica el
stub y regresiones, no un proveedor Common Startup. Fixtures privadas limpiadas;
no modificacion de Startup comun real, ACL, configuracion o privilegios.
