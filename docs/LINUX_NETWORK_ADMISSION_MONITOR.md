# Admision visible y cobertura en Monitor/Sampler

Revision97: [capacidad ampliada](LINUX_NETWORK_SCALE.md),1024 procesos/16384
buckets; la escala>64 citada como pendiente debajo era el estado de revision96.

Revision96, 2026-10-24. Integracion F03: la ruta de admision explicita de
revision94/95 ya llega a Monitor, SystemSnapshot y worker/historial de Sampler.
No cierra F03/H04/J04: escala>64elegibles y validacion/cobertura pendientes.

## Uso publico

```cpp
#include <pcinsight/monitor.hpp>
#include <pcinsight/sampler.hpp>
using namespace pcinsight;
SampleOptions options;
options.include_linux_process_network = true;
Monitor monitor;
// SOLO con consentimiento para todos los elegibles de la vista actual:
auto prepared = monitor.refresh_linux_visible_process_network(
    true, LinuxNetworkAdmissionPolicy::verified_only);
auto snapshot = monitor.sample(options);
// snapshot.linux_process_network: SOLO red de actores admitidos.
// snapshot.linux_network_admission: politica/conteos/exclusiones/epoch/tiempos.
monitor.stop_linux_process_network();

Sampler worker(std::chrono::seconds(1), 61, options);
auto setup = worker.start_linux_visible_process_network(
    true, LinuxNetworkAdmissionPolicy::verified_only); // SOLO parado
worker.start();
// Cada refresh posterior es EXPLICITO y autorizado, con worker corriendo:
auto changed = worker.refresh_linux_visible_process_network(
    true, LinuxNetworkAdmissionPolicy::verified_only);
auto latest = worker.latest();
worker.stop();
```

Comprobar siempre los Metric devueltos; el ejemplo no asegura permisos/capacidad.
Los overloads sin politica usan strict_all EN CADA LLAMADA, no heredan partial.
No nuevo flag: include_linux_process_network solo incluye el informe, NUNCA
enumera/autoriza/rearma por si solo. Monitor no exige flag en setup (sus opciones
se pasan al sample); Sampler exige flag=true en sus opciones de construccion.

## Exclusividad y ciclo de vida

La seleccion manual anterior y la admision visible son rutas distintas.
No se puede iniciar una mientras la otra esta configurada/activa: stop primero.
El rechazo de mezclar rutas NO reemplaza ni detiene la captura existente.
Negar consentimiento tampoco modifica ninguna ruta. Monitor movido transfiere
su modo/propiedad; el objeto movido no obtiene autorizacion al volver a sample.

Monitor.refresh puede configurar o refrescar la ruta visible explicitamente.
Sampler.start_linux_visible_process_network solo configura mientras esta parado;
Sampler.refresh_linux_visible_process_network exige worker corriendo y una ruta
visible previamente preparada con exito (incluso un inventario conocido sin
elegibles). No permite setup implicito desde un worker sin preparacion.

Opt-out con include=false, stop, excepcion general de Monitor y salida del worker
cierran captura/owner y borran modo/preparacion. Reiniciar el worker o volver a
habilitar el flag NO rearma. Un refresh autorizado fallido cierra captura, pero
el worker continua publicando ausencias motivadas. Otra llamada EXPLICITA a
refresh puede reintentar la ruta visible previamente preparada mientras corre;
no hay reintentos ni enumeracion automatica en el loop de muestras.

## Pareja de informes

SystemSnapshot.linux_network_admission es un nuevo Metric de la metadata del
ULTIMO refresh exitoso. NO es un nuevo censo al momento del sample ni cobertura
global del host. En la ruta manual esta AUSENTE. La metadata y red de un sample
visible proceden del MISMO sobre calificado, con epoch correspondiente.

El adaptador valida politica, tiempos, limite, conteos/exclusiones consistentes,
PIDs y motivos, identidad de metadata con el refresh guardado, cobertura observada,
cardinalidad de actores/epoch. Excluidos no pueden aparecer como actores/filas
de red; filas ajenas o capacidad excedida rechazan ambos informes. El proveedor
inferior conserva su proyeccion/validaciones de identidad y contadores completas.

Fallo/exception de muestra rechaza AMBOS informes, cierra recursos y elimina
metadata vieja en lugar de mostrar cobertura obsoleta. Recursos se cierran antes
de construir diagnosticos de excepcion. Publicacion exitosa al snapshot se
prepara antes de asignar y se mueve sin excepciones. Si un refresh valido no
tiene elegibles, metadata queda disponible (puede tener visible>0 y excluidos),
red queda ausente sin inventar0, y no se invoca al proveedor de red al sample.

## Worker e historial

Se reutiliza orden lifecycle -> Monitor -> history y handoff request-before-lock:
el refresh publica que espera antes de competir con worker, evitando inanicion
por un mutex no justo. No se espera al Monitor teniendo history. El sample viejo
se confirma en history ANTES de que refresh pueda terminar; cada item conserva
su propia pareja de metadata/red. History previa NO se borra por un refresh.

Conjunto elegible igual conserva epoch/rate baselines; solo metadata del refresh
cambia. Por tanto un mismo epoch puede tener diferentes tiempos/motivos/politica
de admision, y se debe inspeccionar cada item. Conjunto cambiado recibe epoch
nuevo y reset de todas las tasas; no cruzar epochs en graficas. latest/history
pueden conservar un sample anterior mientras llega el siguiente: comprobar epoch
devuelto cuando cambia la seleccion. Metadata conocida no significa red conocida.

No se modifica persistencia ni se declara historial por app H04 integrado.

## Compatibilidad y verificacion

Se amplia SystemSnapshot0.x con linux_network_admission: RECOMPILAR biblioteca
y TODOS los consumidores. El informe de red existente no cambia layout, ni se
sobrescriben totales legacy de procesos/usuarios. Nuevos metodos enlazados por
consumer instalado con consentimiento denegado, sin captura completa.

Fixtures: ciclo de vida de adaptador/Monitor/move/optout; metadata corrupta,
epochs/cobertura/seleccion inconsistente, PID excluido/ajeno en red y calidad
incorrecta rechazan ambos; estado sin elegibles conserva metadata sin inventar
red; worker en marcha con refresh concurrente/lecturas de history, fallo no
detiene worker, restart sin rearmado y preparacion con flag apagado rechazada.

Prueba nativa SOLO dos hijos privados+marcador sintetico no consultado en el SO:
procfs/pidfds/BPF publicos nativos, Monitor con snapshot pareado/UDP/intervalo
nominal/oracle entero/move/optout; Sampler con AB->unchangedAB->B->AB, refresco
concurrente-history, epoch/metadata pareados, origen retenido/readmision sin gap,
default estricto cierra ambos mientras worker continua, stop/restart no rearma,
FDs limpios y todos hijos terminan normalmente. Guardias45s/40s sin ampliarlas.

La prueba usa inyeccion interna POR OBJETO para restringir SOLO el enumerador;
son los mismos metodos publicos/ciclo de vida/locks de produccion. No hooks
globales ni override publico de proveedor. Contextos de prueba viven mas que
Monitor/Sampler; el worker sale antes de destruirlos. NO se ejecuto captura
de la vista completa de /proc ni se amplió la autorizacion eBPF privada.

Verificacion revision96: Linux133/133 sin skips y cinco fixtures repetidas20veces
(100ejecuciones), consumidor instalado recompilado/enlazado/ejecutado exit0.
Dos fixtures MSVC repetidas20veces (40ejecuciones) pasan sin relajar asserts.
Un fallo inicial assertion MSVC se preserva en targeted-msvc.log; su causa no
quedo aislada, no atribuirlo a objeto viejo como hecho. Distinto del fallo del
consumidor instalado revision75/N03, que sigue abierto. Build Linux registro
avisos temporales clock-skew de archivos generados WSL (0,012-0,12s); logs intactos.
Recompilacion de comprobacion clock-recheck-linux.log exit0 sin avisos. Espejo96
confirmado en ClickUp: comentario90140272936245; TXT es control oficial.
Logs work/network-admission-monitor-*. Resultados finales/espejo en el TXT.
Escala>64elegibles sigue rechazada sin first64/truncamiento/rotacion; no inferir
soporte host universal ni cierre de F03, app history H04 o agregados J04.
