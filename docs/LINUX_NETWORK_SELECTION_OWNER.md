# Seleccion por PID/inicio sin gestionar pidfds manualmente

Revision88: [consulta con compuerta](LINUX_NETWORK_QUERY_SESSION.md) PASA nativo17
completo/tasas/limpieza. Sin productor compartido ni64actores reales certificados.

Revision87 (historica): [capacidad estructural64 y coste real](LINUX_NETWORK_CAPACITY64.md).
El limite16/256 de procesos/filas citado debajo es historico; el limite16 cookies
por actor NO cambia. Nativo17 falla por coste/vida acotada; NO captura escalable
certificada. Se conserva la evidencia original de cada revision.

Continuacion revision86: [inventario visible con calidad por PID](LINUX_NETWORK_INVENTORY.md)
y puente estricto sin filtrado silencioso. No amplia capacidad16ni activa captura.

Revision85 agrega LinuxProcessNetworkSelectionOwner en
pcinsight/linux_process_network_selection.hpp. Prepara y POSEE los pidfds de una
lista completa de candidatos PID/start_ticks suministrada por la politica del
consumidor. No enumera procesos, captura red ni autoriza sondas. Es un paso para
gestionar altas/retiros, NO proveedor general terminado. F03 permanece abierta.

## Contrato

refresh(consentimiento,candidatos) acepta0..16procesos distintos, inicio conocido
y PID valido. No fallback PID-only, inventar ticks, truncar a los primeros16 ni
rotar procesos ocultamente.17candidatos rechaza ANTES de abrir descriptores. La
politica que quiera todos los visibles tiene que detectar/expresar capacidad
insuficiente; no puede informar cobertura global sobre esta seleccion limitada.

Se ordena por PID y se retienen pidfds de PID/start exactos. Lista identica o solo
reordenada se revalida pero no cambia generation ni changed: evita reiniciar tasas
sin necesidad. Alta abre pidfd, retiro cierra referencia propia al confirmar la
nueva lista. Reemplazo por mismo PID/start distinto cuenta retiro+alta, nunca
reutiliza descriptor por PID solamente. Primero/alta/retiro/reemplazo cambian
generation, que NO es selection_epoch del recolector. Ambos son tokens distintos.

refresh valida boot/identidad local del observador antes, durante y al terminar,
inspecciona procesos fijados y repite inspeccion de TODOS despues de adquirir la
lista para detectar al primero que sale mientras se prepara el ultimo. El lector
nativo verifica procfs real, proyeccion Pid del pidfd en fdinfo de la tablaFD del
hilo, directorio fijado, stat PID/start, estado UID y namespaces; poll antes y
despues rechaza pidfd terminado. No traduce IDs locales a dominio kernel: esa
autenticacion BPF sigue haciendola el recolector downstream al recibir seleccion.

Consentimiento falso/forma invalida/capacidad no mutan una lista preparada. Fallo
runtime, lectura estimated/denegada, cambio de proceso/contexto, fallo adquisicion
o excepcion limpia TODAS las referencias propias, sin publicacion parcial. Las
referencias nuevas tienen RAII incluso si un actor posterior falla. Fork rechaza
refresh/selections validos y cierra la copia heredada, no las del padre. API movable,
no copiable. Contextos internos inyectados deben vivir mas que el owner.

Lista vacia es VALIDA, devuelve cambio/retiros y selections conocido vacio. No
pasar ese vacio al start/update del recolector (estos exigen1..16): detenerlo.
clear/destructor liberan recursos; nueva preparacion exige consentimiento otra vez.
Fuera de Linux preparacion autorizada da unsupported, sin nuevo backend Windows.

## Prestamo y captura: no confundir propiedad con autorizacion

selections devuelve copia del vector, pero sus fd son PRESTADOS hasta proximo
refresh/clear/destruccion. Filas llevan cookie0 para modo observado. Serializar
owner/uso del prestamo en misma tablaFD; no cerrar/reutilizar esos fd desde fuera.
El resultado no revalida liveness futura: proceso puede salir inmediatamente.
El recolector vuelve a autenticar identidad/liveness y duplica esos pidfds durante
start o altas de update, manteniendo sus propias referencias independientes.

Por ello clear/fallo del OWNER NO detiene una captura ya iniciada. Si falla la
politica/refresh, el consumidor debe detener o actualizar EXPLICITAMENTE el
recolector; nunca presentar su seleccion vieja como lista nueva completa. Permiso
para preparar pidfds no es permiso para iniciar/actualizar BPF. Cada setup/update
de captura conserva las reglas de consentimiento existentes.

## Ejemplo de reconciliacion del consumidor

```cpp
#include <pcinsight/linux_process_network_selection.hpp>

bool reconciliar(pcinsight::LinuxProcessNetworkSelectionOwner& owner,
                 pcinsight::LinuxProcessNetworkSampler& red,
                 std::span<const pcinsight::LinuxNetworkProcessCandidate> lista,
                 bool permiso_preparar, bool permiso_capturar, bool& activa)
{
    auto preparada = owner.refresh(permiso_preparar, lista);
    if (!preparada.value) { red.stop(); activa = false; return false; }
    auto filas = owner.selections();
    if (!filas.value) { red.stop(); activa = false; return false; }
    if (filas.value->empty()) { red.stop(); activa = false; return true; }
    if (activa && !preparada.value->changed) return true;
    auto estado = activa
        ? red.update_selection(permiso_capturar, *filas.value)
        : red.start(permiso_capturar, *filas.value);
    activa = estado.value.has_value();
    if (!activa) red.stop();
    return activa;
}
```

Construir red con observed_namespace_buckets. Lista procede de una fuente cuya
identidad/disponibilidad y cobertura se conocen: no omitir silenciosamente un
ProcessInfo.linux_start_ticks ausente ni pasar un inventario parcial como completo.
Llamar reconciliacion y muestreo desde worker del consumidor, no bloquear GUI.
Puede enlazarse desde IDE, sin exigir CMake del consumidor antes de su GUI.
No se integra automatico en Monitor/Sampler ni se enumera /proc en esta revision.

## Evidencia y limites

Nuevo test linux-network-selection con checks activos en Release: consentimiento,
forma/duplicados/inicio0/PIDfuera de rango/17capacidad sin tocar fuente;16candidatos
positivos, fd0 valido, retencion/reorden/generation estable, retiro/readd/reemplazo,
vacio/clear,13fallos runtime (adquisicion, segundo actor, primero en recheck, contextos,
calidad estimated, excepciones/boot/namespace) y cierre completo; fork simulado/
move/sin APIs.16actores son fixtures, no16procesos reales medidos.

Manual --private-children, sin BPF/root: dos hijos privados esperando IPC,
adquisicion de2pidfds, retencion/reorden sin generation nueva, move activo/fork
rechazado/padre intacto, retiro/readd, inicio obsoleto y actor salido invalidan
owner entero, recuperacion con actor restante/vacio. Sin fugaFD. No senales al
objetivo en recorrido exitoso, duplicacion de sockets ni trafico inspeccionado.
Watchdogs parent20s/child15s; limpieza de hijos privados si falla la prueba.

Manual autorizado --authorized-private-owned-selection conecta preparacion a API
PUBLICA BPF en dos hijos privados: cierra pidfds originales, entrega filas del owner,
start duplica, clear owner no corta captura. Reutiliza oraculos de revision83:
metadata vacia/tardia, cuatro tasas/UID, A TCP12288/12288 UDP768/768 peek24 y B doble,
dos hilos por actor, padre excluido/fork/update/retiro-readd-gap/exit/restart/FD limpios.
SOLO1netns real compartido; hijos inicializan SO_NETNS_COOKIE para oracle, no cookie
al productor. No captura general ni multiples netns reales. Watchdogs60s/50s.

Primer build emitio3warnings de indentacion enganosamente compacta; corregidos sin
cambiar contrato, log inicial conservado. Logs work/network-selection-*.
Linux127/127 Release sin skips (nuevo test de seleccion),2tests20repeticiones,
consumidor instalado recompilado/enlazado/ejecutado exit0 y build final sin warnings.
MSVC2/2 focalizadas sin nuevos C++ warnings (D9025 esperado asserts activos).
No suite/consumidor Windows completos nuevos: fallo consumidor75 sigue N03,
no compatibilidad total. Positivos de BPF limitados a los dos hijos privados.
Sin nuevo layout de tipos existentes, GUI, persistencia, dependencia, CLI, cambio
de namespace/config permanente ni captura global. F03/H04/J04 siguen abiertas.

Pendiente para seleccion GENERAL: fuente de inventario con cobertura explicita,
politica automatica de admision/retiro y arquitectura que escale por encima de16
actores sin ocultar perdidas/capacidad. No cerrar F03 por esta gestion de pidfds.
