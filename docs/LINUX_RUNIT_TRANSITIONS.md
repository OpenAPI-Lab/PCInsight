# Observación posterior a solicitudes runit

Revisión129, parte de K03. API pública en `pcinsight/linux_runit.hpp`;
no se integra automáticamente con Monitor ni sustituye el proveedor systemd.

`LinuxRunitTransition` conserva un handle explícito y permite **una solicitud**.
`start(action, authorized, timeout)` exige consentimiento y una lectura inicial
válida/available antes de escribir. Acepta 1..60000ms (7s por defecto), medidos
con steady_clock desde antes de la lectura inicial. Si esta consume la ventana,
no envía nada. Una denegación o argumentos inválidos no consumen la instancia.
No reintenta solicitudes ni entregas parciales. El constructor no hace I/O.

Después, el llamador programa `poll()` a su ritmo, por ejemplo desde su GUI.
No hay hilo, timer, espera activa ni ejecución de scripts/CLI. Las lecturas
nativas siguen el contrato de [runit](LINUX_RUNIT.md). El deadline limita qué
muestras se aceptan; NO cancela llamadas de I/O ni garantiza latencia máxima.
El host debe seguir serializando cambios de contexto/fork como exige el handle.

| Acción | Predicado sobre el estado informado por el supervisor |
|---|---|
| start | run, PID no cero, want_up, sin paused ni term_sent |
| stop | down, PID cero y want_down; el hueco down/want_up no basta |
| restart | predicado start y TAI64N de ejecución estrictamente posterior al inicial |

La comparación TAI es lexicográfica sobre los 12 bytes validados, sin convertir
a Unix ni mezclarla con steady_clock. Otro PID con la misma época NO confirma
restart. Una época posterior con el mismo PID sí satisface el predicado; el PID
reportado no es identidad del kernel local. Si el reloj TAI retrocede, puede no
observarse el predicado: no se inventa una confirmación. `finish` con PID cero
es válido, pero no es down. `observe_linux_runit_target()` expone esta misma
regla pura sin I/O; usarla no envía solicitudes.

`baseline_satisfied` describe solo el estado inicial. Incluso si start/stop ya
lo satisfacen, se entrega la solicitud y hace falta una muestra posterior.
`target_state_observed=true` significa **estado informado observado en ventana**,
NO causalidad de la solicitud, readiness, ejecución del hook check, éxito de
trabajo ni garantía de que el proceso siga vivo después. `delivery.completed`
permanece UNKNOWN incluso cuando la observación es positiva. Tampoco autentica
al peer FIFO ni convierte el PID reportado a namespace/identidad del observador.

Una lectura válida no coincidente produce false conocido/pending; una lectura
no disponible produce UNKNOWN, sin reutilizar el último dato bueno. Una lectura
que termina después del deadline puede conservarse en latest, pero no confirma
el objetivo en ventana. Una positiva terminada exactamente en deadline sí vale.
Un poll iniciado después del deadline no lee. Timeout no prueba fracaso del
servicio y no revierte bytes entregados. Retroceso de steady_clock o excepción
de observación terminan failed, conservando los datos de entrega disponibles.

`report()` solo copia el informe, sin I/O. `observed` queda congelado, y otro
poll no relee ni reenvía. `close()` abandona idle/pending y libera referencias,
sin lectura final, señal, cancelación o undo del servicio; el destructor tampoco
actúa sobre el servicio. El llamador coordina la vida del objeto con sus hilos.

Uso conceptual, objeto retenido mientras el GUI programa sus consultas:

```cpp
auto opened = pcinsight::LinuxRunitService::open("/service/mi-servicio");
if (!opened.value) { /* mostrar disponibilidad/note */ return; }
auto operation = std::make_unique<pcinsight::LinuxRunitTransition>(*opened.value);
auto result = operation->start(pcinsight::ServiceAction::restart, consentimiento);
// Solo si result es positivo: consultar operation->poll() desde el timer propio.
// Inspeccionar report().state; no interpretar delivery como readiness.
```

## Evidencia y límites

`tests/linux_runit_transition.cpp`: reloj inyectado privado por instancia,
consentimiento/argumentos/baseline sin escritura, entrega única, conteos retenidos,
supresión de completion falso, épocas/PID/flags, finish y gap autorrestart,
deadline exacto/tardío, unavailable/retroceso, abandono e informe congelado.
Fixture Linux con directorio/FIFOs/status privados: start/restart/stop observados
desde el handle nativo y recuento de descriptores sin fuga. No ejecuta runsv.

La validación de un supervisor runit real sigue pendiente (no instalado en el
entorno comprobado). Estas fixtures NO equivalen a aceptación real. K03 sigue
abierta por esa validación, otros init (OpenRC/s6/SysV) e integración pendiente.
No instala utilidades, cambia servicios reales, lanza apps de inicio ni amplía I07.

Referencias primarias del protocolo; el predicado anterior es un contrato propio
de observación, no una reproducción de los checks/readiness de sv:
[runsv.c](https://raw.githubusercontent.com/void-linux/runit/master/src/runsv.c),
[sv.c](https://raw.githubusercontent.com/void-linux/runit/master/src/sv.c).
