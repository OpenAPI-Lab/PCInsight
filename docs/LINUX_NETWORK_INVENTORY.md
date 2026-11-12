# Inventario visible para seleccion de red (sin captura)

Revision88: [consulta con compuerta](LINUX_NETWORK_QUERY_SESSION.md) PASA nativo17
completo/tasas/limpieza. Sin productor compartido ni64actores reales certificados.

Revision87 (historica): [capacidad estructural64 y coste real](LINUX_NETWORK_CAPACITY64.md).
El limite16/256 de procesos/filas citado debajo es historico; el limite16 cookies
por actor NO cambia. Nativo17 falla por coste/vida acotada; NO captura escalable
certificada. Se conserva la evidencia original de cada revision.

Revision86 agrega pcinsight/linux_network_inventory.hpp:
read_linux_network_process_inventory(consentimiento) y el puente estricto
linux_network_inventory_candidates. No BPF, sockets, senales, comandos externos,
procesos auxiliares, privilegio elevado ni descriptores persistentes. F03 abierta:
un inventario no es captura general ni elimina el limite16del recolector actual.

## Vista y disponibilidad

Se enumera SOLO el /proc de la vista actual del llamador, en un directorio fijado
y comprobado como procfs por fstatfs y dev/inode del observador. Numeric entries
validos se ordenan por PID, sin dedup silencioso; limite100000, exceso/fallo readdir
da ausencia global, NO vector parcial. No seguir arbitrariamente otra raiz/procfs.
enumeration_completed significa que ese barrido alcanzo EOF, no snapshot atomico,
inventario del host entero ni garantia de que no existan procesos ocultos.

La vista puede estar restringida por namespaces/montaje/permisos. Procesos invisibles
no generan filas y NO se declaran inexistentes. Un proceso puede aparecer/salir
durante el barrido; no se combinan dos enumeraciones para inventar un censo estable.
Identidad boot/procfs/PID-user namespaces del observador se comprueba por extremos,
con bracket CLOCK_MONOTONIC y presupuesto cooperativo de5s. Reloj estimado/regresivo,
cambio de observador, presupuesto agotado, fuente incompleta o excepcion rechazan
TODO el inventario. No plazo duro/cancelacion mientras un syscall esta ejecutando.

## Una fila por PID encontrado, incluso cuando falla

Para cada PID se abre pidfd temporal, se obtiene stat/start y se verifica mediante
el lector pidfd/procfs existente: proyeccion Pid de fdinfo en la tablaFD del hilo,
directorio de proceso fijado, stat/status UID y namespaces coherentes. poll antes/
despues de inspeccion detecta terminacion. Se cierra pidfd antes de siguiente fila;
no duplica sockets ni lee contenido de red. Los resultados NO garantizan vida futura.

Estados publicos por fila:

- verified: candidate PID/start y observer_real_uid conocidos/available.
- permission_denied: Metric ausentes/permission_required, motivo preservado.
- disappeared: pidfd_open indica proceso ausente o poll verifica terminacion;
  Metric ausentes/unsupported, NO ticks/UID0 inventados.
- unavailable: otra identidad/fuente incompatible o fallo no clasificable como
  terminacion/permiso, con motivo; no presentar todo fallo como proceso salido.

UID real esta en dominio del OBSERVADOR procfs, no UID inicial kernel, usuario
local resuelto ni sesion. UID0 conocido es valido. No comparar numericamente con
informe BPF. candidate contiene solo PID/inicio local; el gestor/recolector vuelve
a verificar vida e identidad al preparar/capturar, sin convertir ticks en ns.

Snapshot global available significa que el recorrido visible/comprobaciones de
contexto terminaron. NO implica que TODAS las filas tengan identidad disponible.
Cada fila tiene sus propios Metric. Un error por PID no se omite ni invalida
otros PIDs verificados; una incoherencia estructural del proveedor SI rechaza todo.

## Puente sin filtrado silencioso

linux_network_inventory_candidates exige inventario presente/available, enumeracion
completa, metadata/brackets validos, orden/identidades exactas y TODAS las filas
verified. Rechaza cualquier fila desconocida/denegada/salida; no selecciona solo
las que pudo leer. Rechaza MAS DE16procesos con motivo de capacidad, aun cuando
el inventario pueda informar muchos mas. No devuelve los primeros16ni rota sin
avisar. Vacio conocido devuelve lista vacia; detener captura, no start/update vacio.

```cpp
#include <pcinsight/linux_network_inventory.hpp>

auto inventario = pcinsight::read_linux_network_process_inventory(true);
// Mostrar estado y motivo de CADA fila, sin sustituir ausencias por0.
auto candidatos = pcinsight::linux_network_inventory_candidates(inventario);
if (candidatos.value) {
    pcinsight::LinuxProcessNetworkSelectionOwner owner;
    auto preparado = owner.refresh(true, *candidatos.value);
    // Si conocido y no vacio: owner.selections() da pidfds PRESTADOS/cookie0.
    // Setup/update BPF requiere consentimiento SEPARADO y copia esos pidfds.
    // No crear owner temporal si sus filas se usaran despues de destruirlo.
}
// Si fuente/puente falla: no llamar a esto cobertura total; detener o mantener
// EXPLICITAMENTE otra politica seleccionada, sin etiquetarla como inventario nuevo.
```

No captura ni auto-update desde include_linux_process_network o esta lectura.
No integra aun una politica de admision automatica en Monitor/Sampler. El consumidor
puede definir un subconjunto declarado, con cobertura explicita distinta; no alterar
el inventario y seguir llamandolo conjunto completo de visibles. Preparar/capturar
siempre revalida PID/start; una muestra antigua puede estar obsoleta inmediatamente.
Llamar desde worker del GUI. Enlazar desde IDE sin exigir CMake de aplicacion antes
del GUI; build interno de biblioteca se mantiene. Fuera de Linux autorizado da
unsupported; consentimiento falso devuelve permission_required sin inspeccion.

## Evidencia

Nuevo linux-network-inventory con asserts activos Release: orden canonico/UID0/
metadata/vacio,16->puente positivo,17->inventario positivo pero puente rechaza,
estados denegado/salido/unavailable y mezcla conocido-desconocido conservando filas.
21fallos de fuente/clock-presupuesto/contexto/duplicados/rango/100001entradas antes
de inspect/identidad extrana/estimated/excepciones; filas de error incompatibles
rechazadas.15inventarios alterados rechazados por puente (metadata, calidad,
incompleto, identidad, orden/duplicados, UID, estado). No BPF en CTest.

Manual --native-readonly: barrido REAL de /proc visible (tambien procesos ajenos en
modo solo identidad/lectura), dos hijos privados IPC verificados por PID/start,
entregados como subconjunto EXPLICITAMENTE privado al owner pidfds y liberados.
Puente de inventario real devuelve TODOS dentro de capacidad o ausencia motivada,
nunca subconjunto silencioso. Ambos hijos terminan normalmente/FDs limpios; sin
root/BPF/senales al objetivo/trafico capturado. Watchdogs parent20s/child15s para
la prueba; no plazo duro de syscalls. Casos denegado/salido positivos son fixtures,
no se afirma haber forzado hidepid/exit race real ni multiples namespaces.

Primer build tuvo1warning de indentacion compacta; corregido/log conservado.
Linux128/128 Release sin skips (nuevo test inventario),2tests20repeticiones,
consumidor instalado recompilado/enlazado/ejecutado exit0/build final sin warnings.
MSVC2/2 focalizadas de codigo compartido sin nuevos C++ warnings (D9025 esperado
asserts activos). No suite/consumidor Windows completos nuevos: fallo consumidor75
sigue N03/no compatibilidad total. Se ajustaron motivos de stat invalido para no
reutilizar una nota positiva como explicacion de ausencia.
Logs work/network-inventory-*. Nuevo header/tipos/funciones y test aditivos, no
layout existente/GUI/persistencia/dependencia/CLI/config permanente/captura global.

Siguiente: admision/retiro automaticos y arquitectura de captura que escale MAS
DE16actores con capacidad/calidad explicitas. No cerrar F03 por un inventario
read-only; H04/J04 siguen abiertas. No ampliar pruebas de trafico fuera de los
procesos privados autorizados sin autoridad nueva.
