# Disconnect Linux: capacidad por proveedor y limite justificado

Revision50, 2026-10-19. J05 se resuelve por la alternativa que ya figura en su
criterio oficial: **capacidad indisponible justificada**, para los adaptadores
Linux utmp/login1 incluidos. NO se implementa una desconexion real, no se afirma
que ningun servidor remoto Linux pueda desconectar, y Windows WTS sigue en M06.
El rechazo de la accion por si solo no cerraba J05; esta unidad fija el contrato
de capacidad por proveedor, su fundamento, su propagacion al inventario y las
pruebas de ausencia de sustitucion destructiva. No se cambia el criterio ni se
cuenta otra vez la remediacion J03.

## Consulta de capacidad sin sesiones ni efectos

```cpp
#include <pcinsight/linux_session_control.hpp>
auto capability = pcinsight::query_linux_disconnect_capability(
    pcinsight::LinuxDisconnectProvider::login1);
if (pcinsight::metric_is_known(capability) && !*capability.value) {
    // Deshabilitar Disconnect y mostrar capability.note.
    // NO llamar terminate_user_session como fallback.
}
```

La consulta es estatica: describe los adaptadores incorporados, no interroga
una sesion, un bus, un servidor remoto ni permisos. Funciona incluso con cero
sesiones o captura desactivada. En Linux devuelve false/available y el motivo
del proveedor reconocido (utmp o login1). Un enum desconocido o un build no
Linux devuelve Metric sin valor/unsupported; no inventa un false medido.

UserSession.disconnect_available usa el mismo motivo solo cuando los detalles
del proveedor son conocidos y reconocidos. Datos sin proveedor, unsupported,
denegados o con nombre futuro quedan desconocidos. No se clasifica el proveedor
por un prefijo del ID; Active, Remote, State y logout_token tampoco dan capacidad
de detach. Un token de logout sigue siendo exclusivamente para la accion
destructiva separada y no se usa en Disconnect.

## Fundamento y cobertura

- utmp es un registro de accesos, no un protocolo de control ni un identificador
  de transporte remoto. Su documentacion advierte que el inventario puede no
  estar completo. [utmp(5)](https://man7.org/linux/man-pages/man5/utmp.5.html).
- La interfaz login1 revisada documenta inventario, activacion, bloqueo,
  terminacion y control de dispositivos, pero no detach de un transporte remoto.
  ReleaseControl libera el control del dispositivo, no desconecta conservando
  una sesion; ReleaseSession pertenece al ciclo PAM y no debe ser llamado por
  clientes. Ninguna de ellas se sustituye por Disconnect.
  [Contrato login1](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html).

Estos contratos justifican la falta de capacidad EN ESTOS adaptadores, no una
prueba de imposibilidad para cualquier extension futura o distro. La propiedad
Remote no identifica ni autoriza un controlador RDP/VNC/SSH concreto. Preservar
procesos al cortar una conexion exige la semantica/identidad del servidor de
transporte correspondiente; no se obtiene de un PID/TTY o una fila de logind.
No se agregan dependencias, daemon, CLI ni protocolo remoto experimental.

La interfaz GUI debe mostrar esta limitacion. No presentar la opcion como
operativa en Linux con estos proveedores. Un futuro adaptador de transporte
necesitara su propio contrato, identidad/autorizacion y validacion positiva;
no podra reutilizar este false como deteccion de todas las capacidades del SO.

API adicional0.x: nuevo header/enum/funcion libre, sin modificar layouts,
vtables o formato de historial. Recompilar/enlazar consumidores para usar el
nuevo simbolo. Cambio de comportamiento de disponibilidad: una fila agregada
sin metadatos de proveedor ahora da unknown, no false/available. Las filas
nativas utmp/login1 reconocidas conservan false y reciben una nota especifica.

## Validacion revision50

Windows126/126 y Linux110/110 Release. Repeticion20 de disconnect-safety, users
y session-state por plataforma (60 ejecuciones Windows y60Linux). Windows
verifica stubs/politica portable, no una sesion Linux. Ambos consumidores
instalados se recompilan/relinkan/ejecutan exit0 con la nueva funcion/header.
Builds sin warnings. Evidencia work/disconnect-capability-*; ultima reconstruccion,
suites completas, instalacion y consumidores en verified-{build,suite,install,
consumer}-{windows,linux}.log; repeticiones en repeat-* y lectura nativa en
native-linux. Los registros final-* anteriores tambien se preservan.

La prueba nativa Linux crea un hijo propio en una sesion POSIX privada y un
utmp privado: tras rechazos auditados de ID TTY/login1/token de logout, el hijo
responde, no recibe SIGHUP y sigue vivo. Lecturas del canal usan poll con espera
de2s y CTest limita la prueba completa a20s. Limpieza de hijo y archivo/directorio
exactos, sin utmp real alterado. Inventario login1 real de solo lectura conserva
1sesion/1State/1Active conocidos en este host; no se desconecta ni termina.
Las fixtures prueban ambos proveedores con todas las combinaciones Active/
Remote/State, enum desconocido, proveedor ausente/futuro/denegado y NUL invalido.

Las primeras suites fallaron en el NUEVO caso NUL porque el agregador ya rechaza
esa metadata con invalid_argument, mientras la prueba esperaba un Metric sin
valor. Se corrigio la expectativa para exigir esa excepcion; no se cambio ni
debilito el validador. Logs iniciales preservados como
work/disconnect-capability-suite-{windows,linux}.log. No atribuir ese fallo a la
accion nativa ni a los fallos historicos CPU/app-history de N03.

## Antecedente: remediacion de seguridad

Unidad cerrada: retirar el control legacy `kill(-getsid(ut_pid), SIGHUP)`.
No es la implementación de una nueva API de desconexión real.

## Problema corregido

El código anterior buscaba ut_line en utmp, obtenía un PID y usaba su session ID
como process group ID negativo. Session ID, process group ID y TTY/ut_line no
son identidades intercambiables. La pertenencia puede cambiar, utmp puede quedar
obsoleto y el PID puede reutilizarse. Además, SIGHUP puede terminar aplicaciones;
enviar esa señal no prueba una desconexión preservando sus procesos.

Ahora `SystemActions::disconnect_user` Linux valida entrada y falla cerrada:
invalid_argument para vacío, NUL/bytes de control o más de 4096 bytes;
not_supported para identificadores restantes. No lee utmp, no llama getsid,
kill ni a un session manager, y no cambia privilegios. No acepta prefijos
“logind:” como autorización implícita para cerrar sesión.

`UserSession::disconnect_available` publica false con Support::available en
los proveedores Linux reconocidos: es disponibilidad **del backend implementado**,
no una afirmación de que Linux jamás permita desconectar un proveedor remoto.
El GUI puede ocultar/deshabilitar el botón y mostrar la nota. En proveedores que
no calculan este campo queda unsupported; no convertir ausencia a false medido.
AuditedActions sigue registrando solicitud/rechazo, sin inventar una acción.
El campo se agrego anteriormente en0.x; revision50 no vuelve a cambiar su layout.

## No confundir operaciones

“Disconnect” de una sesión remota puede conservar aplicaciones para reconectar.
Linux no ofrece un detach universal de sesiones gráficas/TTY en todos los
proveedores. Una implementación requiere el protocolo nativo específico de
ese proveedor, con identidad y permisos adecuados; no CLI ni grupos adivinados.

logind Session.Terminate/TerminateSession **termina** la sesión y sus procesos.
No debe ser un reemplazo silencioso de Disconnect. Está pendiente decidir si
se necesita una acción separada de “Cerrar sesión”, explícitamente destructiva,
y luego implementar/verificar ese proveedor. Decisión posterior confirmada:
acción separada implementada y verificada con sesión PAM privada real en
LINUX_LOGOUT.md. No se afirma compatibilidad
universal logind/elogind ni desconexión remota por esta corrección.

## Verificación

2026-10-14: corrección de seguridad verificada inicialmente con Linux20/20 y
Windows31/31; suites finales con nueva acción separada21/21 y32/32. Instalación
y consumidores externos verificados en ambos sistemas.

Lógica portable: límites/bytes de control/Unicode, IDs TTY/numéricos y prefijos
aparentemente válidos no disparan acciones ni producen falso éxito.
Linux: el test crea su propio hijo en una sesión POSIX aislada, instala un
detector SIGHUP y registra una fila utmp **privada** que apunta a ese hijo. Tras
disconnect_user, el hijo confirma que no recibió SIGHUP y sigue vivo. Monitor
expone disponibilidad false; auditoría refleja not_supported.
Se recolecta únicamente ese hijo y se elimina el archivo temporal, restaurando
el selector utmp del proceso de prueba. No se modifica utmp del sistema ni se
desconecta/cierra sesión del usuario. Windows solo prueba la política portable.

Referencias: [sesiones POSIX](https://man7.org/linux/man-pages/man2/setsid.2.html),
[señales a process groups](https://man7.org/linux/man-pages/man2/kill.2.html),
[logind: Terminate no es detach](https://github.com/systemd/systemd/blob/main/man/org.freedesktop.login1.xml).
