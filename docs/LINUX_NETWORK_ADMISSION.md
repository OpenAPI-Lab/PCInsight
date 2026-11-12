# Admision estricta del inventario visible Linux

Revision97: [capacidad ampliada](LINUX_NETWORK_SCALE.md) a1024 procesos, lotes
RAW de hasta64; los limites previos debajo son evidencia historica.

Revision95: [politica parcial opt-in y procedencia](LINUX_NETWORK_ADMISSION_POLICY.md)
anade verified_only con exclusiones explicitas/planificador/sobre de muestreo.
Default sigue estricto; mas64elegibles sigue rechazado sin truncar. Layout de
LinuxNetworkAdmissionChange ampliado0.x: recompilar biblioteca y consumidores.
Los pendientes de politica parcial de revision94 debajo quedan historicos.

Revision94, 2026-10-22. Nueva ruta publica standalone
`LinuxVisibleProcessNetworkSampler`, cabecera `pcinsight/linux_network_admission.hpp`.
Conecta inventario -> politica estricta -> owner pidfds -> proveedor observado
compartido. No seleccion manual de PID ni ejecucion de utilidades externas.
No cierra aun F03/H04/J04 ni activa admision automatica en Monitor/Sampler.

## Uso y consentimiento

```cpp
#include <pcinsight/linux_network_admission.hpp>

pcinsight::LinuxVisibleProcessNetworkSampler network;
// SOLO con consentimiento para capturar TODO el inventario visible encontrado:
auto admission = network.refresh_visible(true);
if (admission.value && admission.value->capture_active) {
    auto frame = network.sample(); // Metric<LinuxProcessNetworkSnapshot>
}
network.stop();
```

Construccion no enumera, no abre pidfds ni carga eBPF. Cada refresh requiere
consentimiento nuevo; `false` no consulta fuentes ni cambia captura previa.
Sample NUNCA enumera/autoriza/rearma. No hay hilo ni polling automatico: el
consumidor decide cuando refrescar membresia y cuando muestrear. Stop cierra
owner/captura; mover transfiere propiedad, objeto movido queda sin muestreo.

## Politica completa o ausencia

La fuente nativa recorre el /proc visible del observador. Se conservan los
motivos de filas no verificadas en el inventario anterior; el puente estricto
rechaza TODOS si hay denegados, desaparecidos o no disponibles. Mas de64actores
rechaza sin filtrar/truncar/rotar. Ningun PID se retiene sin inicio conocido.
Un inventario conocido vacio detiene captura con exito y epoch0, NO demuestra
ausencia de procesos ocultos/en otros namespaces ni ausencia global del host.

Un refresh autorizado fallido, incluso por capacidad, detiene TODA la captura:
no mantiene una seleccion vieja como si representara la vista nueva. Cambios
de observador/boot/procfs/namespaces y PID reutilizado se comprueban; el owner
autentica pidfds por extremos, y el proveedor autentica asociaciones kernel.
No se infiere UID inicial a partir de UID local. Contexto y reloj se comprueban
antes y despues de admision. El inventario y commit han de caber en5s desde
el inicio del inventario. Es presupuesto cooperativo, no interrupcion del
syscall/hard deadline ni cancelacion asincrona.

## No destruir las tasas al refrescar sin cambios

El owner compara PID/start en orden canonico y revalida procesos/contextos.
Si el conjunto no cambia, NO se llama a update/sample oculto del proveedor:
se conservan epoch y todas las baselines, incluso si cambia el orden de entrada.
La comprobacion eBPF/ptrace actual del proveedor sigue ocurriendo al sample;
revalidacion procfs del owner no se presenta como un permiso eBPF cacheado.

Si cambia, altas/bajas/reincorporaciones usan el update publico compartido:
origen de retenidos intacto, nuevas capturas para altas/readmisiones, sin bytes
del gap, epoch nuevo y reset de todas las tasas. PID igual/inicio diferente es
reemplazo, nunca retencion por PID solo. Mantener el owner cuesta N pidfds extra:
despues de cada operacion nativa,2N+6+8G FDs (2N+14 inicial), G lotes vivos.
Propuestas, consultas y BTF pueden requerir FDs transitorios adicionales.

Sample valida epoch/cobertura y devuelve el informe publico estimated. Cualquier
ausencia/error de sample detiene TODO en esta ruta estricta, a diferencia de
la recuperacion de algunas ausencias transitorias del sampler bajo nivel.
Otro refresh con consentimiento puede iniciar una captura NUEVA; nunca ocurre
rearmado por sample/flag ni continuacion silenciosa de los contadores anteriores.

## Cobertura y evidencia

La vista /proc es dinamica y puede estar restringida. Un proceso nacido durante
el barrido o despues del commit no queda garantizado hasta otro refresh. El
limite64 no garantiza rendimiento/cobertura de una maquina completa: un desktop
con mas procesos o permisos insuficientes obtendra un error estricto, no red0.
El muestreo sigue siendo payload TCP/UDP instrumentado,16cookies/actor, parcial
por UID inicial, no total causal de app/sesion/usuario ni trafico de enlace.

Fixtures: inicio/unchanged/reordenacion/conjunto cambiado/PID reutilizado/vacio/
reinicio,64 aceptados y65rechazados, denegados/desaparecidos/unknown, boot/procfs/
namespace/caller cambiado, inventario stale, fallo/excepcion en CADA operacion
de inicio/update, sample calidad/epoch/exception/fork. Sin consentimiento no
consultas; conjunto igual no update ni sample oculto.

Nativo: enumerador de prueba deliberadamente limitado a DOS hijos privados,
pero inspeccion procfs/pidfds y proveedor PUBLICO eBPF nativos. AB->unchangedAB
->B->AB verifica que refresh igual no resetea tasa/epoch, conservados mantienen
origen y reincorporado excluye gap. Censos18->16->26FDs propios (owner incluido),
fork rechazado/padre intacto, fila sintetica no disponible detiene TODOS, reinicio explicito
fresco, vacio detiene, todos los hijos terminan normalmente y FDs limpios.
SO_NETNS_COOKIE solo inicializa el oracle del unico namespace real.
La fila no disponible es un marcador inyectado que NO consulta un PID ajeno
del SO; la captura positiva nunca admite procesos fuera de los hijos privados.

NO se ejecuto `refresh_visible(true)` sobre la sesion completa del usuario:
la autorizacion previa de eBPF era SOLO para procesos privados. La ruta publica
de vista completa se verifica con inyeccion/fixtures y consumidor de consentimiento
denegado. Este ensayo NO es validacion de captura global ni amplía el permiso.
No instalaciones/daemon/configuracion permanente ni nuevas dependencias.

Logs `work/network-admission-*`: final2 Linux Release131/131 sin skips, cuatro
focales20veces(80ejecuciones), consumidor nuevoheader/API instalado-relinkado
exit0. MSVC nueva fixture20veces pasa; native-final privado positivo. Build final
sin nuevos warnings C++/D9025 esperado, no compatibilidad Windows total/N03
resuelto. Recursos se cierran ANTES de formatear diagnosticos de excepcion.
Espejo94 confirmado: comentario90140272312863; SEGUIMIENTO.txt siempre manda.
El primer build pidio el target antes de regenerar CMake (sin regla), corregido
configurando. Un build inicial detecto skew0,75s de timestamps WSL; se conserva
log y se verifica otro build final, sin atribuirlo a fallo de codigo/N03.

Pendientes F03: politica util para vistas grandes/inaccesibles sin ocultar
cobertura y validacion de la vista real bajo autorizacion apropiada, integracion
de admision con Monitor/Sampler si corresponde. H04/J04 mantienen sus criterios
de historia/sesion; no se cierran por esta ruta standalone.
