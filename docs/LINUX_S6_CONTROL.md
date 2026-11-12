# s6: control nativo con entrega explícita

Revisión136 añade [observación de condiciones](LINUX_S6_OBSERVATION.md) y
read_record() ligado al mismo handle. No confirma vida/readiness/causal completion.

Revisión134, unidad de K03. API `LinuxS6Control` en `pcinsight/linux_s6.hpp`:

```cpp
auto target = pcinsight::LinuxS6Control::open("/service/mi-servicio");
if (!target.value) { /* mostrar disponibilidad/note */ return; }
auto delivery = (*target.value)->request(pcinsight::ServiceAction::stop, consentimiento);
// bool(delivery) describe ENTREGA; delivery.completed permanece UNKNOWN.
```

Sin s6-svc/CLI, lib externa, exec/shell/script, sudo, instalación, worker o espera.
Solo el supervisor existente puede ejecutar sus hooks/run/finish al procesar una
orden autorizada; no se promete que esos efectos sean inocuos o aislados. No es
la excepción OpenRC132: esa sigue sin autorizar, ni un iniciador autostart I07.

## Órdenes y significado

| Acción pública | Bytes de protocolo |
|---|---|
| start | u |
| stop | d |
| restart | cru |

Restart expresa reanudar, solicitar reinicio configurado y dejar deseado-up.
No se usa t como sustituto del down-signal configurado de s6. Es un contrato
propio sobre la secuencia c/r/u, no una operación transaccional ni una promesa
de restart terminado. Efectos dependen del estado, configuración, otros actores
y hooks; el supervisor puede rechazar/no poder arrancar, o finish decidir no
reiniciar. No se crean/eliminan archivos down con comandos persistentes D/U/Q.
Se requiere un árbol de confianza con el protocolo revisado; no se negocia
versión, ni se identifica un supervisor por el nombre de carpeta/FIFO/PID.

`request(action, authorized)` comprueba autorización antes de cualquier I/O de
control. Action inválida no escribe. Validación previa fallida/no reader/no
permiso -> entrega cero. Una llamada nunca reintenta una entrega positiva
parcial: como máximo hace un write de 1/3 bytes, reintentando solo EINTR sin
bytes entregados. Es pequeño/atómico para FIFO Linux, pero tres comandos NO son
un job atómico del supervisor. No hay retransmisión implícita por EPIPE/EAGAIN.

`requested_bytes/written_bytes` describen el intento de write; si no se alcanza
esa fase quedan cero. Se preparan mensajes/resultados antes del write, sin
asignaciones de memoria posteriores que oculten una entrega ocurrida. bool true
significa que el write completo se aceptó, NO que alguien leyó/procesó las
órdenes, que existe supervisor vivo/autenticado, que el daemon cambió de estado
o que esté ready. `completed` SIEMPRE UNKNOWN, incluso con entrega total. No se
interpreta el [registro raw35](LINUX_S6.md) como confirmación causal.

## Identidad y seguridad

`open()` fija servicio/supervise/control con descriptores CLOEXEC. Root absoluto
explícito, sin NUL/hasta32768 bytes; directorios vía openat2 NO_MAGICLINKS, links
ordinarios permitidos. Control debe ser un FIFO real, no un link/file/device.
No se crea nada y no se abre para escritura al construir el handle. Puede
abrirse aunque no haya reader o status/lock; no implica capacidad de entrega.

Se conserva contexto boot/procfs/PID/inicio/UID/namespaces/eUID. Cada solicitud
revalida contexto, supervise respecto al servicio fijado y dispositivo/inodo de
control antes de escribir. Después de fork/contexto incompatible no se reutiliza
el handle. Es target fijado, no redirección al nuevo destino de un alias/path
original que cambie luego. Host serializa fork/setns/credenciales/FDs y vida del
objeto; las llamadas del mismo handle están protegidas por mutex.

Para escribir se reabre SOLO el FD propio ya validado por /proc/self/fd, con
WRONLY/NONBLOCK, verificando identidad y selección/contexto antes de write.
FIFO sustituido no redirige la petición a otro objeto. No se abre un path
unchecked para I/O. Requiere openat2/procfs; no fallback inseguro. No hay
snapshot atómico, garantía contra todo ABA ni autenticación del reader; el host
no debe tratar los pins como token de identidad de un supervisor/proceso.

SIGPIPE se bloquea temporalmente SOLO en el hilo llamador. Si write devuelve
EPIPE se consume la señal nueva solo cuando no estaba pending antes; se conserva
una pending preexistente y se restaura la máscara. No handler global ni SIG_IGN.
El host coordina generación concurrente de SIGPIPE: no se garantiza distinguir
una señal externa que llegue durante la ventana. El helper privado acepta solo
FIFO NONBLOCK WRONLY y u/d/cru; no es una API para escribir comandos arbitrarios.

## Evidencia y pendientes

`tests/linux_s6_control.cpp`, solo árbol/FIFOs privados: ausencia sin creación,
tipo/link inválido, consentimiento/enum inválida/no reader, u/d/cru con bytes
exactos/completedUNKNOWN, sin down/status/lock creados, handle tras fork rechazado,
backpressure/EAGAIN sin retransmisión, control/supervise sustituidos rechazados,
EPIPE sin terminar proceso, pending SIGPIPE conservada/máscara restaurada,
descriptor RDWR rechazado, alias de directorio y FD sin fuga. No se ejecuta
s6-supervise ni servicio real. MSVC solo valida argumentos/fallback no-Linux;
el consumidor SDK enlaza símbolos públicos. No se simula aceptación de s6 real.

Revisión135 añade [inventario explícito](LINUX_S6_INVENTORY.md), sin handles
accionables ni equivalencia a servicios vivos. K03 sigue abierta por observación/integración, aceptación real
s6/runit y otros init. Las fixtures demuestran entrega al kernel/lector privado,
no interpretación de comandos, readiness ni liveness real de s6. El destructor
solo cierra descriptores, no envía stop/undo. No hay cambios a ServiceInfo o
autodetección Monitor. El inventario no convierte un registro raw en servicio
vivo ni una ruta en prueba del manager.

Fuentes primarias del protocolo:
[s6-svc](https://skarnet.org/software/s6/s6-svc.html),
[s6-supervise.c](https://raw.githubusercontent.com/skarnet/s6/master/src/supervision/s6-supervise.c),
[writectl](https://raw.githubusercontent.com/skarnet/s6/master/src/libs6/s6_svc_writectl.c).
Contrato de seguridad y conteos anterior propio de PCInsight, no copia de s6-svc.
