# s6: condiciones raw asociadas al destino de control

Revisión136/K03. API `pcinsight/linux_s6.hpp`: `LinuxS6Control::read_record()`,
`observe_linux_s6_condition()` y `LinuxS6Observation`.

```cpp
auto opened = pcinsight::LinuxS6Control::open("/service/ejemplo");
if (!opened.value) { /* mostrar note/support */ return; }
auto op = std::make_unique<pcinsight::LinuxS6Observation>(*opened.value);
auto delivered = op->start(pcinsight::ServiceAction::restart, consentimiento);
// Si delivered: programar poll() desde el timer propio del GUI y leer report().
// condition_observed NO significa job terminado, vida o readiness.
```

## Target y condiciones

read_record() comprueba contexto y pins del handle antes/después de leer raw35
relativo al servicio fijado. Sustituir control/supervise invalida esa lectura.
Cambiar el alias absoluto original NO redirige al nuevo servicio: igual que
request(), sigue el target fijado. Sin probe de lock/reader, autenticación de
peer, petición adicional, exec/CLI/scripts, señal o elevación. Archivo disponible
sin reader sigue siendo registro histórico, nunca prueba de supervisor vivo.

La función pura valida acción, ambos TAI nanosegundos y PID elegible Linux
(0..INT32_MAX). El decoder raw conserva u64 sin truncado; no implica elegibilidad
de ciclo de vida ni identidad local. Las reglas sobre el registro posterior son:

| Acción | Condición raw |
|---|---|
| start | PID positivo, want_up, sin finishing ni paused |
| stop | PID cero, want_down, sin finishing ni paused |
| restart | Condición start y changed_at_tai64n estrictamente posterior a baseline |

Comparación TAI12 lexicográfica sin conversión Unix/uptime/steady. Cambiar PID
con misma época no basta; misma cifra PID con nueva época puede satisfacerla,
sin probar identidad o causa. Cambiar solo readystamp/ready_reported no confirma
restart. Ready se ignora (puede ser true estando down); finishing impide confundir
el PID de finish con el daemon; el gap down/want_up no satisface stop. Si TAI
retrocede puede no observarse restart: no se inventa una confirmación.

**Condición raw observada NO prueba vida, frescura del archivo, readiness ni
causalidad de la orden.** Start/stop pueden ya cumplir baseline y observar luego
el mismo registro sin cambios. No se presenta como transición causal ni se
equivale a s6-svwait/eventos/job. delivery.completed SIEMPRE UNKNOWN, incluso
cuando target_condition_observed=true.

## Una solicitud, polling del llamador

Constructor/report sin I/O. start exige consentimiento antes de leer, acción
válida/timeout1..60000ms (7s default) y baseline available/elegible. Baseline
ausente/estimated/ineligible no envía nada. El control directo134 sigue separado.
Denegación/argumento inválido deja idle; otra terminalización consume instancia.

steady_clock mide desde antes de baseline; si esta agota ventana no se entrega.
Tras request se retienen conteos sin asignaciones de memoria adicionales; el
source nativo prepara sus resultados antes del write y el seam privado debe
cumplir ese contrato. No retry, worker, timer, sleep, espera activa o undo.

El reloj por sí solo no cambia report(): el llamador debe invocar poll() para
avanzar observación/timeout. poll explícito: registro válido no coincidente -> false/pending; unavailable,
estimated o ineligible -> UNKNOWN sin last-good sustituto. Poll iniciado después
del deadline no lee. Si termina tarde guarda latest pero no confirma en ventana;
positiva terminada exactamente en deadline sí se acepta. Timeout NO prueba que
la orden fracasó y no cancela I/O. Retroceso steady/excepción -> failed con
entrega conservada; parcial/error no se reintenta. condition_observed congela
informe/sampled_at y otro poll no lee. close abandona idle/pending/libera fuentes,
sin lectura final, stop o cancelación del servicio; destructor tampoco actúa.
Host coordina hilos/vida/fork/setns/credenciales/FD; callbacks privados no reentran.
No snapshot atómico, prueba de peer o garantía de detectar todo ABA.

## Evidencia y pendientes

Test determinista por instancia: consentimiento/argumentos/baseline sin entrega,
una orden, completion falso suprimido, épocas/PID/readystamp/flags/gap, u64/TAI
ineligible, unavailable/estimated, deadline exacto/tardío, overflow/retroceso,
excepción/parcial/error con conteos, abandono y reporte congelado.

Fixture Linux FIFO/status privados: start/restart/stop observados como patrones,
alias cambiado a otro árbol no cambia origen de lectura; finish/readiness-only/
gap rechazados; baseline ausente no escribe; control sustituido invalida lectura;
FD sin fuga. **El test cambia registros, no un s6-supervise real.** No aceptación
real, vida o causalidad demostradas. MSVC solo condiciones/reloj/fallback
portables; consumidor SDK enlaza símbolos nuevos.

K03 sigue abierta por integración/aceptación real/otros init. OpenRC scripts132
sin autorización/aplazado e I07 observador intactos. Siguiente: inventario SysV
read-only; no inferir una excepción para ejecutar scripts de control.

Fuente primaria de flags/marcas revisada:
[s6-supervise.c](https://raw.githubusercontent.com/skarnet/s6/master/src/supervision/s6-supervise.c).
Estas condiciones/ventana son contrato propio, no réplica de su FSM.
[Control](LINUX_S6_CONTROL.md), [raw35](LINUX_S6.md),
[inventario](LINUX_S6_INVENTORY.md).
