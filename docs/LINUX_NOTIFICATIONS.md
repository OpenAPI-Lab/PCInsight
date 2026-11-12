# Notificaciones Linux observadas: H05

Proveedor nativo opcional de captura desde que el consumidor lo activa.
LinuxNotificationObserver se conecta directamente a D-Bus mediante libsystemd
del sistema, si existe. No ejecuta dbus-monitor, gdbus, dbus-send, shells,
gestores de paquetes ni aplicaciones auxiliares. No instala ni descarga nada.

## Uso y consentimiento

```cpp
#include <pcinsight/linux_notifications.hpp>

pcinsight::LinuxNotificationObserver notifications; // No conecta ni captura.
pcinsight::LinuxNotificationOptions options;
options.consent_to_monitor_notification_traffic = true; // Consentimiento del usuario.
auto started = notifications.start(options);
if (started) {
    auto observed = notifications.poll(); // Llamar regularmente desde el mismo hilo.
    if (observed.value) {
        for (const auto& row : observed.value->apps) {
            // accepted_notify_calls: llamadas aceptadas, NO avisos mostrados/leidos.
            // declared_desktop_entry: hint NO autenticado, puede estar ausente.
        }
    }
    notifications.stop(); // Cierra SOLO nuestra conexion, no el servicio.
}
```

El GUI debe pedir consentimiento informado antes de poner el flag: monitorizar
trafico de notificaciones puede revelar actividad de aplicaciones y la conexion
recibe transitoriamente mensajes con contenido. PCInsight no conserva ni imprime
app_name, titulo, cuerpo, icono, acciones ni otros hints. Solo retiene la identidad
de conexion, desktop-entry cuando es utilizable, contadores y tiempos. El mensaje
D-Bus crudo existe transitoriamente en buffers de la biblioteca de transporte;
esto NO promete borrado criptografico de RAM ni invisibilidad ante depuradores.
No hay escritura a disco ni persistencia automatica.

Sin flag start devuelve invalid_argument antes de abrir el bus. No se activa
implicitamente desde Monitor, Sampler, un constructor, el consumidor de prueba
ni al importar historial. Instalar la libreria no instala un servicio capturador.

## Que cuenta y como identifica

Se observan llamadas Notify del protocolo org.freedesktop.Notifications en su
ruta estandar, dirigidas al servicio conocido o a su propietario unico fijado.
Cada llamada con acuse se guarda temporalmente por (sender unico, serial).
Solo una respuesta exitosa del propietario fijado, dirigida a ese mismo sender
y con reply_serial coincidente, incrementa accepted_notify_calls. Respuesta
repetida, ajena, anterior al periodo o sin llamada pendiente no vuelve a sumar.
Respuesta de error del servicio incrementa rejected_calls, no aceptadas.

accepted_notify_calls incluye actualizaciones/reemplazos. replacement_calls es
su subconjunto con replaces_id no cero, validando la identidad devuelta. No es
una cuenta de ventanas de notificacion distintas, notificaciones mostradas,
acciones del usuario ni mensajes leidos. DND, agrupacion o supresion visual no
se pueden inferir de un acuse. El GUI debe rotular esta metrica como solicitudes
aceptadas, manteniendo la nota de alcance, no como total universal de avisos.

NO_REPLY_EXPECTED se contabiliza aparte como unconfirmed_no_reply_calls, sin
esperar acuse ni presumir aceptacion/rechazo. Llamadas pendientes y expiradas
sin respuesta tienen contadores separados; nunca se convierten en aceptadas0.
Una llamada sin respuesta vence tras30s monotónicos observados por el consumidor.

La agrupacion es (conexion sender unica, hint desktop-entry declarado). Este
hint puede faltar, ser incorrecto o suplantar el nombre de otra app. No se
verifica leyendo archivos ni se transforma en PID/exe/UID/username. app_name no
se usa como identidad. Sin hint utilizable, la fila conserva contador por
conexion y declared_desktop_entry ausente: atribucion por app desconocida.
Dos conexiones con el mismo hint siguen separadas; un broker/portal puede
representar varias apps y una app puede tener varias conexiones. No sumar por
nombre como si demostrara origen causal. Las identidades solo valen en este bus
y periodo, no entre reinicios/buses diferentes.

## Periodos y disponibilidad

El resultado es Metric<LinuxNotificationReport>, estimated y limitado a llamadas
observadas con correlacion valida. Una lista vacia conocida solo significa que
no se han observado aceptaciones: NO demuestra cero notificaciones de cada app.
period_start/period_end son etiquetas wall-clock; observed_elapsed es monotónico.
La captura comienza durante la solicitud de habilitacion del monitor, no al
arrancar el SO. El tramo de setup puede tener huecos. No existe historial
retroactivo ni garantia de entrega completa del bus.

poll procesa sin espera explicita hasta1..4096mensajes (default256); no hay
thread oculto ni wait infinito. poll_budget_reached indica que se alcanzo ese
presupuesto, no prueba por si solo una perdida. El consumidor debe volver a
llamar y dimensionar su cadencia/carga; el broker puede imponer limites y cortar
una conexion lenta. report no drena el bus: devuelve la ultima observacion.

stop conserva el informe finito de lo ya observado, running=false; su extremo
final es el ultimo poll, no una promesa de haber drenado mensajes pendientes.
Una nueva captura valida empieza un periodo nuevo y reinicia contadores.
No reintentar/concatenar periodos como una cuenta acumulada unica sin conservar
su identidad y cobertura. Error de conexion/protocolo/limite/overflow o cambio
de propietario invalida el periodo actual y devuelve ausencia, no datos viejos
o cero. Los informes previos que haya copiado el GUI siguen siendo observaciones
historicas limitadas. No se reconecta automaticamente.

Esta API NO llena el entero legacy AppHistoryEntry.notifications ni mezcla la
identidad de desktop-entry con el historial CPU por ejecutable. El GUI puede
presentar este informe como otra fuente de historial con su periodo/disponibilidad.
HistoryStore no serializa este informe. H04/red y M08/Windows siguen separados.

## Bus, permisos y fuentes ausentes

Por defecto solo se usa /run/user/<geteuid>/bus. El consumidor puede seleccionar
explicitamente otro socket Unix filesystem local mediante bus_socket_path;
paths acotados y caracteres restringidos, no sintaxis de direccion D-Bus.
No se usa DBUS_SESSION_BUS_ADDRESS, autolaunch, tcp, unixexec, direcciones
abstractas, listas de endpoints ni fallback al bus de sistema. SO_PEERCRED
debe indicar el mismo UID efectivo en las conexiones de control y monitor.
Esto no autentica al software bajo ese mismo UID ni constituye un helper root.

Se consulta GetNameOwner sin iniciar el servicio y se fija su nombre unico;
se verifica otra vez tras BecomeMonitor y se observa NameOwnerChanged del bus.
Se pide solo Notify, respuestas/errores de ese propietario y cambios de su nombre,
no una regla vacia que capture todo. No se mandan notificaciones ni se cierra
ninguna existente. Tras convertirse en monitor no se hacen consultas sobre
esa conexion; la conexion temporal de control es independiente y se cierra.

BecomeMonitor requiere permiso segun el broker. Denegacion publica
permission_required/permission_denied, sin elevar privilegios, relajar policy,
activar eavesdropping legacy ni capturar a escondidas. Servicio/bus/API opcional
ausente produce unsupported. libsystemd es un cliente de protocolo opcional:
no exige que el init sea systemd; si no hay esa biblioteca compatible o un
servicio/broker soportado la fuente queda ausente.

Solo este protocolo FDO estandar esta cubierto. Interfaces GTK/GNotification,
portales antes de su traduccion al servicio, KDE/vendor privadas, navegador,
buffers anteriores, otros usuarios/namespaces/buses y notificaciones que nunca
pasan por este endpoint NO se convierten en cero ni en cobertura completa.
No se afirma soporte universal de cada compositor/distribucion o aplicacion.

Limites:1024pendientes,2048filas,desktop hint hasta4096B,256hints por llamada,
serial D-Bus valido y contadores uint64 con rechazo de overflow. Predicados de
nombre unico cubren el formato numerico de los brokers probados. Setup usa
timeouts de llamada de2s; no son un deadline total de todos los syscalls.
Los limites propios no acotan exactamente el RSS/buffers/decodificacion de
libsystemd o del broker. Clase confinada a un hilo; el host coordina su uso.

## Pruebas y compatibilidad

tests/linux_notifications.cpp prueba consentimiento/moved-from/stop, periodos,
identidad sender/serial, replicas, reemplazos, rechazo, app desconocida,
expiracion, relojes y limites/overflow con assertions activas en Release.
tests/linux_notifications_native.cpp conecta el API publico con un servidor
de notificaciones simulado sobre un broker D-Bus REAL, privado, como usuario
normal. Verifica aceptadas3/reemplazos1/rechazadas1/app-desconocida1/sin-acuse1,
stop/nuevo periodo, cambio de owner y denegacion real de BecomeMonitor.

Solo el harness usa dbus-daemon ya instalado como servidor privado, sin shell,
con config/socket en mkdtemp; no consulta datos por CLI ni modifica el bus de
la sesion/sistema. Retira/recolecta sus hijos y archivos. El target nativo se
incluye en CTest cuando existe ese binario de prueba; su ausencia no cuenta
como un positivo. El proveedor de produccion nunca lanza ese binario.

Primera prueba nativa: faltaba completar Hello en el cliente del fixture;
segunda: sd_bus_send con cookie nulo marcaba NO_REPLY_EXPECTED. Se conservaron
ambos logs, se corrigio el harness y se anadio cobertura explicita de solicitudes
sin acuse en produccion. No se debilitaron las aserciones ni se atribuyeron los
fallos a CPU/historial N03. Logs work/notifications-focused-linux-{first,handshake}.log.

API adicional linux_notifications.hpp y pimpl nuevos, sin modificar layouts
existentes, ABI virtual, SystemSnapshot ni formatos de historial. Consumidores
deben enlazar la nueva biblioteca para usar los simbolos. Windows ejecuta
fixtures portables y rechazo del proveedor nativo, no una captura Linux real.

Verificado2026-10-19: Linux106/106 y Windows118/118 Release aprobadas;10/10
repeticiones del fixture nativo y ejecucion directa final exit0, sin skip.
Builds finales sin warnings; paquetes reinstalados y consumidores externos
recompilados/relinkados/ejecutados ambos exit0 sin activar la captura del usuario.
Logs work/notifications-{build,suite,install,consumer}-{linux,windows}.log,
work/notifications-repeat-linux.log y work/notifications-native-linux.log.
H05 P->C: observador opt-in con atribucion/privacidad/periodos declarados y
positivo nativo, no solo campos/unsupported. H04/M08/N03 no se cierran.

Referencias primarias: [protocolo Desktop Notifications](https://specifications.freedesktop.org/notification/latest/protocol.html),
[D-Bus BecomeMonitor](https://dbus.freedesktop.org/doc/dbus-specification.html#bus-messages-become-monitor),
[envio sd-bus y respuesta esperada](https://www.freedesktop.org/software/systemd/man/latest/sd_bus_send.html).
