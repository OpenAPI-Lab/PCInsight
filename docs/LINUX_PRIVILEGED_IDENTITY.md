# L05/Linux: observación nativa de identidad del mensaje y bus

Revisión147. Código nativo interno, **todavía sin receptor/helper operativo ni
CheckAuthorization nativo**. No instalación/activación de daemon, reglas, diálogos,
elevación o acciones. No conexión a Monitor/SystemActions ni API pública/layout
o formato nuevos. Esta unidad completa observación de identidad, no autorización.

## Entrada de confianza y conexión

observe_native_privileged_identity recibe un sd_bus_message prestado del receptor
confiado, válido/serializado durante la llamada, y configuración del mecanismo.
No recibe nombre/UID de solicitante desde el payload. Exige que sea EL mensaje
actualmente despachado por ese bus, method-call, con sender único y destino igual
al nombre único propio. Un mensaje construido localmente o retenido fuera del
dispatch no sirve. El cuerpo no se lee ni se interpreta como identidad.

El bus debe ser client/ready, address igual al unix:path de configuración y tener
timeout de métodos no0/hasta5s. No cambia ajustes del bus prestado ni abre otra
conexión usando entorno/default/session/autolaunch. Exige socket Unix stream;
SO_PEERCRED proporciona credenciales del peer de conexión, que deben coincidir
con bus_uid del mecanismo. getpeername debe resolver exactamente el endpoint
filesystem configurado, sin abstract socket ni redirección a otro path. Se fija
dev/inode del FD y PID/UID/GID del peer para revalidar al final.

El peer Unix es el router, NO el solicitante reenviado. La identidad de éste se
consulta al router por su sender único. UID del router tampoco se supone0:
bus_uid y authority_uid se configuran por el mecanismo, nunca por cliente ni por
autodescubrimiento que acepte cualquier UID. Ausentes/desconocidos -> sin consultas.
El endpoint por defecto es /run/dbus/system_bus_socket; configurar otro es una
decisión del mecanismo, no funcionalidad de redirección del cliente.

## Consultas limitadas al daemon D-Bus

Se carga libsystemd opcional y se requieren los símbolos exactos. Solo llamadas
org.freedesktop.DBus en /org/freedesktop/DBus: GetId, GetConnectionUnixUser del
sender, GetNameOwner de org.freedesktop.PolicyKit1 y UID de ese owner. Authority
debe tener nombre único distinto del caller y UID igual a authority_uid.
Se repiten ID/owner/ambos UID y peer del socket: cualquier cambio/ausencia/error
descarta resultado. Reply debe ser method-return del daemon, firma exacta s/u y
valor presente; string<=255, GUID32 lowerhex y nombres :decimal.decimal conservadores.
No compatibilidad inventada con otros formatos. UID0 es un dato válido, no fallback.

Antes/después se verifica contexto nativo boot/proceso/namespaces y eUID; mensaje
actual/sender/destino, estado/address/nombre del bus deben permanecer coherentes.
Se copian los datos y generaciones del mecanismo. Error -> Metric sin valor,
no UID0 por defecto, sin reintentar ni consultar otro bus. Los errores nativos de
consulta se publican conservadoramente unsupported con motivo, no se inventa una
clasificación de permisos más específica de la obtenida.

GetNameOwner no inicia el servicio buscado: esta unidad NO envía CheckAuthorization,
no registra/activa Authority ni helper, no cambia nombre/dueño/servicios. No se
añade una conexión autolanzada si Authority está ausente.

## Frontera que aún falta

La función presta el objeto recibido, no instala un receptor que autentique
mensajes externos por sí misma. El receptor debe entregar mensajes del dispatch
real del bus adecuado y validar método/payload cerrado. Punteros/structs internos
pueden fabricarse dentro del proceso: no son API de autorización ni ticket.

Configuración/endpoint y resolución de libsystemd deben protegerse frente a
clientes/env/LD_LIBRARY_PATH/bibliotecas modificables ANTES de desplegar un
mecanismo privilegiado. Esta biblioteca no es un bootstrap seguro para root y
todavía no carga una configuración root-owned. Las pruebas redirigen el loader
intencionadamente, solo dentro de su proceso aislado. Un router o biblioteca
malicioso de confianza puede inventar metadatos; no hay garantía contra ello.

Sin snapshot atómico/ABA, prueba de vida permanente o antirreplay global. Timeout
es por llamada, hasta8 consultas5s más operaciones locales, NO deadline total;
callbacks/I/O externos pueden bloquear. El host coordina forks/setns/credenciales,
bus/message/FD lifetime y toda mutación de configuración. Generaciones se obtienen
del mecanismo: esta unidad no implementa recarga/contador global.

## Evidencia

tests/linux_privileged_identity_fixture.cpp implementa ABI de libsystemd aislada;
tests/linux_privileged_identity.cpp la carga en un proceso con LD_LIBRARY_PATH de
CTest. No conexión al bus del host. Sí se crea un socket Unix stream real privado,
con endpoint real/credenciales SO_PEERCRED del kernel y limpieza/FD comprobados.
Las respuestas GetId/UID/owner y mensaje entrante están SIMULADAS, no se presentan
como broker real, autenticación real de un cliente o aceptación polkit.

Casos: positivo de contrato, no-current, sender/destination/type/address/client/
ready/timeout inválidos, Authority ausente, ID/owner/UID cambiados, reply de otro
sender/firma incorrecta/truncado, UID peer/Authority/endpoint distintos y argumentos
sin configuración. MSVC solo argumentos/fallback; SDK anterior sigue enlazando.
No scripts/procesos o servicios reales se lanzan para esta prueba.

Validación147: Linux174/174 +40 repeticiones L05, MSVC10 argumentos/fallback,
dos consumidores SDK relinkados exit0. Builds Linux inicial/final limpios.
Logs work/privileged-identity-*. No broker real, autorización polkit o CI remota.

Fuentes primarias revisadas: [API sd-bus](https://raw.githubusercontent.com/systemd/systemd/main/src/systemd/sd-bus.h),
[metadatos del mensaje](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_message_get_sender.html)
y [D-Bus specification](https://dbus.freedesktop.org/doc/dbus-specification.html).
Contrato propio conservador, no réplica completa de todas las implementaciones.

L05 sigue E. Siguiente unidad: consulta nativa CheckAuthorization con subject del
mensaje fijado, correlación de respuesta y parsing acotado; pruebas de protocolo
aisladas, todavía sin activar helper/diálogo ni conectar ejecutor. Receptor real,
bootstrap/config segura, integración/coordinador y aceptación polkit siguen abiertos.
K03/OpenRC132/I07 y aplazamiento UAC intactos.

Actualización149: [CheckAuthorization nativo](LINUX_PRIVILEGED_NATIVE_AUTHORIZATION.md)
ya implementado y conectado al coordinador interno con pruebas ABI aisladas, no
aceptación polkit real/receptor/helper/ejecutor. Actualización148 aprobó control
OpenRC explícito y privado; las notas147 sin excepción son históricas, no revocan148.
