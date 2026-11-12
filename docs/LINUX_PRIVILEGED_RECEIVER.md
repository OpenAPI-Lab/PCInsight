# L05 — receptor cerrado, solo observación de autorización

Revision150, 2026-11-02. Interno, no instalado ni conectado a SystemActions,
Monitor o un helper permanente. L05 sigue E: esta unidad no es despliegue seguro
ni autorización para ejecutar acciones del sistema.

`receive_native_privileged_request` en src/linux_privileged_identity.cpp recibe
un sd_bus_message prestado del dispatch actual y configuración/lista permitida
del mecanismo. Solo admite path `/org/pcinsight/Helper`, interfaz
`org.pcinsight.Helper1`, miembro `AuthorizeSystemService`, firma exacta `uusb`:
versión1, operación1/start-2/stop-3/restart, unidad, intención explícita booleana.
No acepta UID, nombre del solicitante, actionID, autoridad, comandos, argumentos,
entorno, PID ni rutas adicionales como campos del cliente.

Comprueba mensaje actual/tipo/metadatos, rebobina y decodifica una vez, exige fin
del cuerpo, versión/operación/boolean válidos y unidad de hasta255 bytes. Copia
la solicitud y aplica preflight145 antes de consultar identidad o autorización.
La identidad procede del adaptador147 y sus consultas autenticadas al broker,
no de esos campos. El adaptador149 congela la solicitud y enlaza la única
CheckAuthorization al sujeto/acción/unidad/verbo/owner/cookie; el coordinador146
aplica revalidación y rechazo estricto. Interacción sigue OFF por defecto;
autostart de la consulta y flag interactivo del bus siguen0. Sin ejecutor,
respuestas IPC de producción, registro de objetos ni apertura de conexiones.

La intención es una declaración del cliente, NO prueba de un clic/consentimiento
humano ni sustituto de una política segura. El gate del mecanismo para permitir
interacción sigue separado. Un resultado `authorization_observed` no es ticket,
capacidad, garantía de vigencia posterior ni permiso reutilizable para ejecutar.
No conservar mensaje/adaptadores después del handler ni despachar concurrentemente
la misma conexión. Una futura ejecución necesitará su propia frontera y comprobaciones.

## Prueba con transporte real

tests/linux_privileged_receiver.cpp utiliza libsystemd REAL y un dbus-daemon ya
disponible, exclusivamente como infraestructura de desarrollo. CMake añade el
test privado solo si encuentra ese ejecutable en /usr/bin o /bin; no descarga ni
instala nada. La biblioteca no ejecuta dbus-daemon, ni otras utilidades para datos.

El test crea configuración propia y socket en directorio mkdtemp privado: sin
service directories/activación ni acceso a buses del host. Tres conexiones al
broker privado representan cliente, receptor y Authority SIMULADA. Credenciales
SO_PEERCRED, consultas daemon UID/ID/owner, mensajes, remitentes, seriales y
serialización libsystemd son REALES. No se ejecuta polkitd ni se prueba su política,
agente, diálogo, aceptación del host o control de servicios. Se conserva el UID
actual, sin elevación. No se inicia ni detiene ningún servicio del usuario.

Cada recorrido prueba16 solicitudes: cuatro observaciones aceptadas (start,
start con header sender falsificado, stop, restart), once rechazos sin consultar
Authority (versión/operaciones/intención/unidad/ruta/interfaz/miembro/campo extra
que intenta añadir UID) y una denegación de Authority. Cinco CheckAuthorization
exactas; Authority verifica que el sujeto sea el nombre único REAL del cliente
incluso al intentar falsificar el encabezado, y que actionID/unit/verb/flags
coincidan. Descriptores antes/después iguales. Un proceso padre supervisa al
worker acotado y retira broker/config/socket incluso si falla el worker; el broker
también recibe señal de muerte si desaparece su padre. Sin shell para lanzarlo,
sin procesos ni archivos persistentes tras las pruebas exitosas.

Validación150: Linux178/178; ocho tests L05 repetidos10 veces (80 ejecuciones,
incluidas10 del broker privado); MSVC10 argumentos/fallback; dos consumidores
SDK reinstalados/relinkados exit0. Build inicial con clockskew preservado en logs;
incremental final limpio, sin tocar reloj/mtime. work/privileged-receiver-*.
Sin cambios API pública/layout/formatos/acciones ni afirmación de CI remota.

## Pendiente para L05

Actualización151: [política protegida](LINUX_PRIVILEGED_POLICY.md) implementa carga
desde ancla verificada y generaciones sin fallback. El test usa el receptor
gestionado y ahora recorre21 solicitudes/8checks, incluida invalidación durante
la respuesta positiva. Evidencia150 anterior histórica; bootstrap sigue pendiente.

Bootstrap y configuración protegidos (incluido cargador dinámico/entorno,
propiedad/permisos/rutas y generaciones), aislamiento de conexión y concurrencia,
límites de mensaje/FD/cola antes de decodificar, deadline total/rate limiting,
política/consentimiento polkit reales, ejecutor cerrado con revalidación antes de
actuar y aceptación real bajo autoridad específica. El límite255 de unidad NO
limita memoria de mensajes ya recibidos por el transporte. Las consultas
individuales acotadas NO son un deadline agregado ni garantizan cancelación remota.
UAC/Windows aplazado. Siguiente unidad: configuración protegida y ciclo de vida
del mecanismo, aún sin ejecutor, instalación, activación ni elevación.
OpenRC148 sigue autorizado solo para control explícito; SysV no ampliado,
K03/I07 y las restricciones de recopilación permanecen intactos.

Fuentes primarias consultadas para el ABI y el aislamiento del test:
[sd-bus.h](https://raw.githubusercontent.com/systemd/systemd/main/src/systemd/sd-bus.h),
[registro de callbacks](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_add_object.html),
[dbus-daemon](https://dbus.freedesktop.org/doc/dbus-daemon.1.html).
El protocolo cerrado y los límites son decisiones PCInsight, no una API pública
de polkit o un sustituto de las comprobaciones pendientes de despliegue.
