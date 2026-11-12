# Cerrar sesión Linux: acción explícita, separada de Disconnect

Decisión confirmada por el usuario el 2026-10-14: añadir “Cerrar sesión” como
acción separada que puede terminar sus procesos. Implementación:
`SystemActions::terminate_user_session(UserSession::logout_token)`.
Puede perder trabajo no guardado. El GUI debe pedir confirmación explícita y
no invocarla como fallback de Disconnect, Lock o error de otra acción.

```cpp
// Solo después de una solicitud explícita de cerrar ESTA sesión.
if (!session.logout_token.empty()) {
    auto result = actions->terminate_user_session(session.logout_token);
    // Error después de aceptación NO prueba que no ocurrió ningún efecto.
}
```

## Inventario y disponibilidad

Monitor prefiere org.freedesktop.login1 cuando está disponible: ListSessions
por D-Bus nativo. Actualización J01: State/Active/Remote separados con Metric,
Id/Name/User contrastados con ListSessions y TimestampMonotonic antes/después.
Ver LINUX_SESSION_STATE.md para semántica/cobertura, incluido fallback utmp.
IDs de filas: login1:<id>. El token combina unique bus owner, ruta del objeto
Session y TimestampMonotonic, no PID/TTY ni un número inventado. Si la propiedad
de identidad no es legible o no coincide, la fila permanece sin token y con
State/Active/Remote desconocidos. logout_available es estimated: token presente no
certifica permisos para Terminate ni que la sesión siga existiendo.
Las métricas de recursos siguen sumas visibles por username, no por sesión.

SampleOptions::prefer_logind_sessions=false elige utmp explícitamente. Sin
libsystemd/bus/login1 se conserva fallback de registros utmp; esas filas no
tienen token de cierre. Una instancia encontrada pero con fallo de enumeración
no devuelve una lista parcial como completa. No agrega logind a una distro que
no lo usa ni instala librerías; dlopen libsystemd.so.0 es opcional en runtime,
sin headers/SDK externos/CLI. Compatibilidad elogind solo si ofrece esta misma
API y el transporte sd-bus requerido está instalado; no se presume verificada.

## Control y resultado

1. Validar token acotado: owner único numérico, path Session, timestamp uint64
   positivo. Rechaza IDs TTY/PID, NUL/colas inválidas y rutas auto/self/parent.
2. GetNameOwner confirma la misma instancia de login1; cambio -> stale_target.
3. Leer TimestampMonotonic usando el **unique owner**, no volver a resolver
   el well-known name para cada solicitud. Cambio -> stale_target, sin cierre.
4. Llamar Session.Terminate sobre esa ruta y unique owner, una sola vez.
   Se deshabilita autorización interactiva; no sudo/pkexec ni prompts polkit.
5. Consultar hasta retirar la identidad: UnknownObject/NoSuchSession o timestamp
   distinto confirma que la identidad anterior ya no está publicada. No vuelve
   a cerrar un objeto reemplazado. Otras fallas o timeout después de aceptación
   informan estado desconocido/no confirmado, sin Cancel ni rollback.

La retirada del objeto no certifica código de salida ni que un proceso que
escapó al scope de sesión haya terminado. El session manager aplica sus reglas
de permisos y semántica. No se cierran otras sesiones de ese usuario mediante
TerminateUser/TerminateSeat, no se envían señales a grupos calculados desde utmp.
“Cerrar sesión” no conserva una sesión para reconectar; Disconnect permanece
not_supported en el proveedor Linux actual.

## Límites

Token máximo 1340 bytes, rutas 1024, owner 255, valores D-Bus 4096; inventario
10000 filas/16 MiB de payload. Consultas D-Bus 5s por llamada. Enumeración usa
presupuesto de iteración 5s; cierre usa presupuesto de reconsulta 5s/pausas50ms.
No es un deadline total: una llamada/bus opening puede demorar fuera de ese
presupuesto. La librería no crea threads ni persistent storage automáticamente.

Token debe provenir de una muestra fresca del mismo host/boot/bus. No es un
secreto ni una autorización, no identifica una sesión globalmente ni sobrevive
reinicios de D-Bus. El chequeo de propiedades y Terminate no son una transacción
atómica: una implementación compatible podría retirar/reutilizar ruta entre
consultas. Unique owner evita redirigir a otra instancia de servicio; no se
promete una garantía universal de referencia inmutable a objetos D-Bus.
Structs, virtual interface y audit enum 0.x ampliados: reconstruir todos los
consumidores y wrappers. Backends Windows/otros heredan not_supported por ahora.
AuditedActions registra terminate_user_session con token/resultado; token puede
ser sensible y requiere la misma política de acceso/retención de otros targets.

## Evidencia de cierre

2026-10-14: Linux Release21/21, Windows Release32/32; paquetes instalados y
consumidores externos reconstruidos/ejecutados. La muestra nativa de lectura
observó 1 sesión login1. No se envió un cierre a esa sesión.

Pruebas inyectadas: cambio de owner/timestamp, permisos, target inexistente,
proveedor ausente, confirmación exitosa/timeout/fallo, parseo y auditoría.

Validación destructiva nativa posterior, 2026-10-14: ejecutable de prueba
`pcinsight-linux-logout-native-tests --native-fixture`, explícitamente en un
contexto root Ubuntu24.04 WSL. PAM se carga únicamente en el test; no es nueva
dependencia de la librería. pam_start_confdir/pam_open_session con pam_systemd
ya instalado, configuración privada /tmp y proceso en servicio transitorio
exclusivo. No se escribe /etc/pam.d ni se llama directamente CreateSession.

Antes de cerrar se comprueba que ID no estaba en inventario inicial, Leader
es el PID de ese servicio y Service es la marca PAM única de la prueba. El PID
se fija mediante pidfd. Token con timestamp distinto devuelve stale_target y
el hijo sigue vivo. El token correcto pasa por AuditedActions/acción pública:
logind retira la sesión y pidfd confirma salida del líder. IDs de sesiones
preexistentes permanecen. Servicio transitorio deja de estar registrado y
archivos PAM privados desaparecen; no se cerró ninguna sesión preexistente.

Ejecutado con éxito dos veces; la segunda incluye verificación explícita de
ausencia del servicio y archivos. No se ejecuta automáticamente en CTest:
necesita contexto privilegiado seleccionado y UID1000 ya existente. No crea
cuentas, no modifica PAM/scheduler/settings del sistema y solo usa las APIs
nativas para el fixture. Todos sus checks son require/exception, activos Release.
Suite Linux21/21, Windows32/32 e instalación/consumidores de ambos verificados.

Cierre funcional de este proveedor login1 en el entorno probado. No certifica
permisos de todos los consumidores, todas las distros/elogind ni detach que
conserva apps. Los límites de identidad/plazos descritos arriba siguen aplicando.
La protección de Disconnect se documenta por separado en LINUX_DISCONNECT.md.

Referencia primaria: [API login1 y semántica Terminate](https://github.com/systemd/systemd/blob/main/man/org.freedesktop.login1.xml).
