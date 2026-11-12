# L05/Linux: CheckAuthorization nativo, sin ejecutor

Revisión149, 2026-11-02. Se implementa la consulta nativa y el enlace al coordinador
146; NO se instala/activa helper, receptor, política, diálogo o acción privilegiada.
Validación aislada con libsystemd ABI simulada y sockets Unix privados, no Authority
real. No API pública/layout/formato/Monitor/SystemActions nuevos.

## Entrada y construcción cerrada

check_native_privileged_authorization requiere mensaje entrante prestado dentro
del dispatch, configuración de confianza del mecanismo, contexto esperado,
solicitud explícita y reglas del mecanismo. Primero repite preflight145; sin
intención/target/operación permitidos no consulta. Reobserva identidad147 y exige
coincidencia exacta con el contexto fijado antes de enviar CheckAuthorization.
No UID/nombre de subject arbitrario, ID polkit aportado por cliente ni root por defecto.

Construye mensaje para el owner ÚNICO ya verificado de Authority, path/interface
PolicyKit1 Authority y método CheckAuthorization. Subject system-bus-name con
name del sender del mensaje original; action_id fijo por start/stop/restart;
details cerrados unit y verb de la solicitud validada/copiada. No argumentos,
comandos, rutas, variables de entorno, scripts o expansión de texto.
Flags de autostart y de autorización interactiva del mensaje D-Bus son0.

Flag API polkit AllowUserInteraction por defecto0. Solo1 si hay intención explícita
validada Y el mecanismo permite interacción. Las pruebas de1 usan exclusivamente
la ABI simulada, sin diálogo en el sistema. No activar automáticamente un agente
de autenticación ni consultar permisos desde el muestreo de datos.

## Correlación, parseo y calidad

Una llamada síncrona sd_bus_call, timeout5s, sin retry ni fallback. La biblioteca
transporta el mensaje y asigna su cookie: el reply debe ser method-return del
owner fijado, firma exacta (bba{ss}), cookie de llamada no0 e igual a reply-cookie.
El cuerpo contiene dos booleanos0/1 y array de pares string. Falta de datos,
tipo/emisor/firma/cookie incorrectos, contenedor inválido o texto sobrante -> UNKNOWN.

Hasta64 detalles, key1..128 bytes, value0..4096, suma key+value hasta16KiB.
Duplicados de cualquier key y exceso invalidan todo, sin elegir un valor ni
truncar. Se preservan dismissed, retención tras challenge e ID temporal conforme
a presencia no vacía; otras claves solo se omiten tras validarlas/acotarlas.
La codificación D-Bus/UTF-8 depende del parser de libsystemd confiado, no se
reinterpreta una respuesta desde bytes arbitrarios aportados por el cliente.

Después se reobserva identidad147: bus/caller/UID/Authority/generaciones deben
coincidir con el contexto esperado. Cambios o fallos descartan el reply completo.
Reply available es dato válido del adaptador, **no grant ni ejecución**. El
coordinador146 aplica rechazo conservador de deny/challenge/dismissed/retención/
temporales. No convertir un raw is_authorized=true en permiso reutilizable.

## Adaptador por solicitud

native_privileged_authorization_api congela configuración, solicitud y reglas;
sus callbacks observe/check reutilizan el nativo147 y la consulta anterior.
El plan recibido del coordinador debe coincidir con el snapshot exacto (unidad,
operación, policy_action), o no se envía consulta. Cada llamada al coordinador
consulta de nuevo, no caché. Mensaje y bus siguen prestados: toda la vida del
adaptador y uso deben quedar dentro del handler válido/serializado, no retener
callbacks después ni compartirlos entre solicitudes.

La configuración congelada no implementa un watcher de recarga. El mecanismo
debe serializar cambios de reglas/credenciales/namespace/FD, usar generaciones
correctas y construir un adaptador nuevo por solicitud. El receptor real y su
parser de payload no existen todavía en esta ruta; no son sustituidos por C++
structs que un fixture puede fabricar.

## Lo que aún no se afirma

Sin helper desplegado, ejecución, elevación, reglas/root-owned config loader,
bootstrap/loader seguro, aceptación con bus/Authority/agente reales o cancelación
remota garantizada. cancellation_id vacío; un timeout local no asegura cancelar
el trabajo de autenticación remota. Interacción0 es default, pero no reemplaza
los requisitos de identidad y autorización. La política administrativa de polkit
puede autorizar sin diálogo; no se promete prueba de contraseña fresca.

La consulta tiene timeout propio, pero las observaciones nativas147 hacen varias
llamadas y no hay deadline total duro/atomicidad/ABA. Revalidar selección/identidad/
política antes de una futura ejecución y consumir autorización en el mismo flujo
sigue pendiente. Ningún ejecutor consume los reportes de esta unidad.
Config, endpoint y libsystemd deben protegerse antes de usar código privilegiado;
LD_LIBRARY_PATH de fixture NO es una configuración de despliegue admisible.

## Evidencia y siguiente paso

tests/linux_privileged_native_authorization.cpp usa ABI simulada que comprueba
el mensaje completo/subject/target/verb/flags. Socket real/SO_PEERCRED privado,
sin conexión al bus/polkit del host. Casos accept/deny/challenge/dismissed/retained/
temporary/unknown, tipo/sender/firma/cookie/booleans/truncado/struct-array-entry,
duplicados/bounds64-65/key/value/payload16KiB exacto-exceso/trailing, owner cambiado,
timeout/buildfail sin retry, interacción0/1 simulada, contexto/plan distintos y
falta de intención sin envío, FD sin fuga. MSVC solo argumentos/fallback.

Validación149: Linux176/176 +60 repeticiones L05, MSVC10 argumentos/fallback,
dos consumidores SDK relinkados exit0. Avisos clockskew iniciales preservados;
incremental final limpio y suite176 sin modificar reloj/mtime. Logs
work/privileged-native-auth-*. No broker/Authority reales ni CI remota.

Fuentes primarias: [Authority API](https://polkit.pages.freedesktop.org/polkit/eggdbus-interface-org.freedesktop.PolicyKit1.Authority.html),
[sd_bus_call](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_call.html)
y [sd-bus ABI](https://raw.githubusercontent.com/systemd/systemd/main/src/systemd/sd-bus.h).
Los límites, detalles unit/verb y política de rechazo son contratos PCInsight,
no una réplica de toda política/implementación polkit.

Actualización150: [receptor cerrado](LINUX_PRIVILEGED_RECEIVER.md) implementado y
probado con libsystemd/broker privados REALES, Authority simulada. La evidencia149
anterior sigue histórica. Sin ejecutor/despliegue/política polkit real; L05 E.
Siguiente: configuración protegida/ciclo de vida, sin instalar ni elevar.

Plan histórico149: receptor aislado con payload cerrado e identidad
del transporte real, sin ejecutor/instalación ni activar servicios del usuario;
después bootstrap/config segura e integración de ejecución aún pendientes.
Permiso OpenRC148 conservado solo para control explícito; SysV no ampliado,
K03/I07/UAC y restricciones de recopilación de datos intactos.
