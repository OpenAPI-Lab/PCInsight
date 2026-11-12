# Politica parcial explicita y procedencia de admision

Revision97: [capacidad ampliada](LINUX_NETWORK_SCALE.md) a1024 procesos/16384
buckets; los limites64 y el pendiente de escala debajo son historicos.

Revision96: [integracion Monitor/Sampler](LINUX_NETWORK_ADMISSION_MONITOR.md)
ya transporta admision/exclusiones junto a red en SystemSnapshot y history del
worker, con refresh explicito y sin rearmado por flag/restart. Los pendientes
de integracion95 debajo son historicos; escala>64/validacion/cobertura pendientes.

Revision95, 2026-10-22. Unidad F03: planificador puro y politica opt-in
`LinuxNetworkAdmissionPolicy::verified_only` integrados en admision standalone.
El overload anterior sigue siendo `strict_all` en CADA llamada; no hereda el
modo parcial de un refresh anterior. F03/H04/J04 siguen abiertas.

## Decidir sin capturar

`plan_linux_network_admission(inventory, policy)` no hace syscalls ni modifica
captura. Valida el inventario ENTERO antes de decidir: metadata, calidad exacta,
PID/inicio/UID, orden, estados/soportes/notas de error consistentes. La politica
parcial no puede ocultar una fila corrupta o un inventario estimado/incompleto.

Un Metric de plan disponible significa que la DECISION es valida, NO que se
pueda capturar. Hay que comprobar ademas `plan.value->candidates.value`:

- strict_all: todas las identidades encontradas han de estar verificadas.
- verified_only: admite todas las filas con identidad procfs verificada y
  excluye explicitamente permission_denied/disappeared/unavailable, con PID,
  estado y motivo original en `excluded`. No ignora otras clases de fallo.
- Mas de64 elegibles: candidates AUSENTE, rechazo TOTAL. No first64, rotacion,
  orden de preferencia ni descarte silencioso. Se conservan conteos y motivos
  para que el GUI explique el rechazo con el inventario original.

`visible_processes` cuenta todas las filas; `verified_processes`, las verificadas.
`all_encountered_verified` expresa solo calidad de identidades, no disponibilidad
de captura (puede ser true para65verificados con candidates ausente). Ninguno
de estos campos convierte una vista procfs restringida en censo global del host.
Plan es una decision sobre el inventario suministrado, no una reserva de capacidad
ni autorizacion; un refresh posterior vuelve a leer la vista actual.

## Capturar con cobertura explicita

```cpp
#include <pcinsight/linux_network_admission.hpp>
pcinsight::LinuxVisibleProcessNetworkSampler network;
// Requiere consentimiento para TODOS los procesos elegibles de la vista actual.
auto changed = network.refresh_visible(
    true, pcinsight::LinuxNetworkAdmissionPolicy::verified_only);
if (changed.value && changed.value->capture_active) {
    auto frame = network.sample_with_admission();
    // frame.value->admission.excluded: PID/estado/motivo de cada exclusion.
    // frame.value->network: SOLO datos de los actores realmente admitidos.
}
network.stop();
```

El informe `LinuxNetworkAdmissionChange` incluye politica, conteos, exclusiones,
epoch y timestamps del refresh; `complete_encountered_set` solo es true cuando
el refresh exitoso no excluye filas encontradas. No afirma cobertura de procesos
ocultos, nacidos despues o del host completo. Si no hay ningun elegible pero si
excluidos, devuelve capture_active=false con todas las exclusiones y detiene:
NO se presenta como inventario visible vacio ni se generan tasas cero.

`sample_with_admission()` une el informe del ULTIMO refresh exitoso al snapshot
de red, con epoch correspondiente. NO vuelve a enumerar ni actualiza esa lista
de exclusiones al momento del sample. El `sample()` anterior sigue devolviendo
solo red seleccionada; su nota indica PARTIAL y numero de excluidos cuando
corresponde, y remite al sobre con procedencia. Ninguna fila excluida recibe
bucket, payload, tasa o suma UID ficticios.

Mismo conjunto PID/start elegible conserva epoch/baselines aunque cambien los
motivos de exclusion o la politica. La procedencia almacenada SI se actualiza.
Cambios de conjunto mantienen retencion/readmision fresca de revision94.
Las copias de reportes se preparan antes del commit y el contexto/clock se
revalida despues. Excepciones de copia/muestreo cierran recursos.

La politica SOLO resuelve calidad del inventario; una identidad procfs verificada
NO garantiza permiso eBPF/pidfd_getfd. Fallos runtime del owner/proveedor, calidad
de contadores, relojes, contexto o callbacks siguen cerrando TODO, sin reintentar
y filtrar actores de captura. No hay elevacion, nuevos permisos, procesos globales
capturados de prueba ni nueva politica predeterminada de seleccion arbitraria.

## Compatibilidad y evidencia

Se amplió el layout0.x de `LinuxNetworkAdmissionChange`: RECOMPILAR biblioteca y
TODOS los consumidores de esa API. No cambia el layout de SystemSnapshot ni el
informe LinuxProcessNetworkSnapshot. Consumer instalado prueba overloads, sobre
calificado y simbolo del planificador con consentimiento denegado/sin captura.

Fixtures:64/65verificados,67filas/64verificados/3excluidos, todos excluidos,
strict/partial/enum invalido,20 corrupciones de inventario (no se esconden),
refresh mixed/unknown/PIDreuse, cambio de motivos con mismo epoch, sample con
procedencia, default estricto no hereda partial; fallos/excepciones del controlador.

Nativo SOLO2hijos privados y una fila sintetica desaparecida: procfs/pidfds/BPF
publicos nativos para los hijos; el marcador no consulta PID ajeno. Refresh parcial
conserva epoch/tasas/censo26FDs propios y el sobre contiene visible3/verified2/
excluded1 y SOLO2actores de red. Datos UDP reales conservan origen y no incluyen
gap del actor reincorporado. Default estricto posterior rechaza esa misma fila
y cierra TODO. Censos previos18->16->26, fork/padre intacto, restart/vacio/FDs e
hijos limpios. Enumerador deliberadamente restringido; NO prueba/permiso de
captura completa de /proc ni de usuarios ajenos. Un netns real, no cobertura64real.

Logs `work/network-policy-*`: Linux Release132/132 sin skips, cinco focales20veces
(100ejecuciones), consumidor nuevoAPI/header instalado-relinkado exit0. MSVC
dos focales20veces(40ejecuciones), no soporte Windows total/N03 resuelto.
Builds finales sin nuevos warnings C++/D9025 esperado. Espejo95 confirmado:
comentario90140272317289; SEGUIMIENTO.txt conserva autoridad permanente.
Warnings iniciales por campos nuevos sin default explicito se corrigieron;
se preserva primer build. No nuevos paquetes, CLI, daemon, configuracion ni GUI.

Pendiente: integrar admision/procedencia en Monitor/Sampler y resolver escala
de mas64elegibles (esta unidad la diagnostica, NO la elimina). Validacion de
vista real bajo autorizacion apropiada y cobertura fisica/cross-distro pendientes;
historia/app y usuarios H04/J04 conservan sus criterios separados.
