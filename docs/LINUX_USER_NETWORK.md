# Red observada por usuario Linux (J04)

Revision101, 2026-10-24. La parte de red de J04 se publica en
`SystemSnapshot::linux_user_network`, independiente del inventario de sesiones.
El disco conserva `UserSession::linux_io_details` (LINUX_USER_IO.md).

`pcinsight/linux_user_network.hpp` expone la funcion pura
`summarize_linux_user_network(frame)` y los tipos `LinuxUserNetworkReport` y
`LinuxUserNetworkSample`. Proyecta una muestra F03 de modo observado, sin iniciar
captura, instalar herramientas, ejecutar comandos, consultar NSS ni acceder al
bus. No hay nuevo requisito de libsystemd/logind ni de paquetes runtime.

## Identidad: UID observado no es una sesion

Cada fila representa UN UID real en el user namespace del OBSERVADOR de F03,
no el UID inicial del kernel. La clave de dominio incluye el boot token y el
device/inode del user namespace del observador; no usar UID aislado entre hosts,
boots o namespaces. El reporte conserva observador, productor, epoch y brackets
F03. En Monitor/Sampler corresponde al MISMO frame que linux_process_network.
Para cobertura del censo leer linux_network_admission pareado, cuando exista;
una seleccion manual no demuestra cobertura de todos los procesos del equipo.

Se utiliza `row.process.real_uid`, cuya asociacion procfs/kernel F03 autentica
con pidfds y joins actuales antes/despues del sweep y entre ventanas. La lista
`initial_real_uids` conserva los UIDs iniciales distintos mapeados a ese UID
observado. Puede haber varios (por ejemplo, vista restringida/UID de overflow):
no se afirma que el grupo equivalga a una cuenta global unica. No se traducen
automaticamente los totales F03 `users` del dominio inicial.

El vector users es ordenado y unico por UID. Dos sesiones del mismo UID no
crean dos filas, ni se suman dos veces sus bytes. No se necesita que el usuario
tenga una sesion registrada (root, servicios y tareas sin login pueden aparecer).
No identifica sesiones activas/conectadas ni propietario causal/exclusivo de sockets.

Los campos de red legacy de UserSession/linux_io_details siguen ausentes:
login1 reporta un UID y utmp un nombre, pero eso no prueba automaticamente el
mismo dominio que procfs (bus de host expuesto a un contenedor, por ejemplo).
No se hace join por username ni por coincidencia numerica sin esa evidencia.
El nuevo informe ES la API autoritativa de red por usuario; el GUI debe mostrar
su UID/dominio/cobertura separados, no presentar una asociacion de sesion
verificada donde no la hay. El nombre de cuenta o etiqueta que añada un GUI no
cambia la clave numerica ni justifica sumar filas de sesiones.

## API y unidades

```cpp
#include <pcinsight/monitor.hpp>
#include <pcinsight/linux_user_network.hpp>

pcinsight::Monitor monitor;
// Configurar ANTES una captura F03 observada con consentimiento explicito.
// Manual: observed_namespace_buckets; o admision visible autorizada.
pcinsight::SampleOptions options;
options.include_linux_process_network = true; // solo lectura, nunca autoriza BPF
options.include_linux_user_network = true;   // NO exige inventario de sesiones
options.include_processes = false;
options.include_user_sessions = false;
auto snapshot = monitor.sample(options); // llamar periodicamente desde el GUI
if (pcinsight::metric_is_known(snapshot.linux_user_network)) {
    for (const auto& user : snapshot.linux_user_network.value->users) {
        // user.real_uid, partial_send/receive/total_bytes_per_second,
        // selected_processes, idle_processes, observed_buckets, known/missing.
        // Comprobar Metric/Support y cobertura; no convertir ausencia a0.
    }
}
```

Tambien `include_user_sessions=true` solicita esta proyeccion si la lectura F03
esta habilitada, sin rellenar los campos de red de cada sesion. Para evitar el
censo de sesiones y sus dependencias opcionales usar el flag dedicado. Ninguno
inicia BPF ni eleva permisos. Captura detenida/flag off/fallo de F03 -> informe
ausente, sin conservar una tasa antigua como actual. Desactivar solo el informe
no cierra la captura F03 que el consumidor haya solicitado para otros usos.

Con un `LinuxProcessNetworkSampler` independiente se puede pasar su resultado
directamente a `summarize_linux_user_network`. Es proyeccion, no autenticacion de
datos fabricados por un llamador: debe recibirse un frame F03 nativo confiable.
Los modos strict/declared quedan explicitamente sin esta proyeccion por usuario.

Cuatro lanes: TCP send, TCP receive, UDP send, UDP receive. Cada valor es bytes/s
de payload de operaciones instrumentadas, no bytes wire, ACKs, active time ni
trafico universal. Se suman las tasas de buckets conocidas, YA redondeadas hacia
abajo por F03. Por eso sumar tasas redondeadas puede diferir de redondear una
suma de bytes; no se declara precision inexistente. Send+receive cuentan ambos
extremos de loopback; no eliminan duplicados causales entre procesos distintos.

Por usuario se publican:

- selected_processes: actores distintos, no numero de buckets ni sesiones.
- idle_processes: actores del registro completo que aun no tienen buckets;
  metadata sin trafico NO se convierte en tasa cero.
- observed_buckets: buckets distintos por proceso/cookie, cada uno sumado una vez.
- known/missing[4]: particionan observed_buckets por lane; permission_denied es
  subconjunto de missing. No cuantifican procesos invisibles/no seleccionados.
- partial_payload_bytes_per_second[4]: suma conocida de cada lane, con overflow[4].
- partial_send/receive/total_bytes_per_second: sumas parciales independientes
  de lanes conocidas, con send/receive/total_overflow. Una lane ausente no se
  rellena con cero; si ninguna es conocida, la tasa agregada permanece ausente.

Un cero MEDIDO de F03 sigue conocido. Primer bucket/baseline, actor sin buckets
o permiso ausente no es cero medido. Si algunas tasas faltan, se conserva solo
la suma parcial conocida y su cobertura, sin extrapolacion. Un overflow deja la
suma afectada ausente (sin wrap/saturacion), conserva contadores de cobertura y
no elimina resultados independientes validos. UID0 es valido.

## Validacion y limites

La proyeccion valida el frame antes de publicar: modo observado/boot/observer/
epoch/productor, brackets acotados y ordenados, identidades unicas de actores,
PID/start/UID/inode, asociacion kernel completa, cookies unicas, numero de
buckets/seq, presencia/Support de contadores y tasas, intervalo nominal igual
al del bracket anterior. Malformacion global rechaza TODO, no publica grupos
parciales. Soporte de permiso del frame ausente se conserva.

Capacidad estructural F03:1024 actores,16 cookies cada uno,16384 buckets. No es
garantia de latencia ni frecuencia. Agregacion acotada en memoria, sin nuevos
baselines ni estado persistente. Repetir el mismo frame no acumula tasas; un
epoch nuevo muestra la cobertura/baseline del frame nuevo, no tasas anteriores.
Cambios de UID/proceso/contexto de las ventanas los valida F03; sus fallos no se
reemplazan con tasas del adaptador, socket_uid ni contadores de otro namespace.

Nuevo layout0.x de SystemSnapshot/SampleOptions: recompilar biblioteca y TODOS
los consumidores. Puede enlazarse desde el IDE sin adelantar el CMake del GUI.
Sin cambios de archivos de historial, ABI virtual de acciones o funciones Windows.

## Verificacion

`tests/linux_user_network.cpp`: UID0/UIDs distintos/muchos-a-uno desde dominio
inicial, cero medido vs unknown/idle, tasas parciales y permiso, send/receive/
total independientes, overflow, dos cookies/un actor, sesiones repetidas no
sumadas/joins no inventados, replay,15frames invalidos y capacidad1024/16384,
flags sin autorizacion ni captura implicita. Release conserva asserts.

`tests/linux_network_admission_native.cpp --authorized-private-admission`:
solo dos hijos privados UDP, Monitor/Sampler/joins/pidfds/BPF reales. UID del
observador, dos actores/dos buckets, cuatro lanes y tasas send/receive/total
comparadas con oraculo exacto de tasas redondeadas; epoch/observador pareados,
primer bucket/idle sin cero inventado, reporte disponible SIN inventario de
sesiones. Historial del Sampler conserva metadatos/numero de actores durante
AB->B->AB, refresh concurrente, fallo/stop/restart. Sin captura global, sondas
retiradas, hijos normales y FDs restaurados. La prueba tambien conserva la
regresion H04 de bytes reales/almacenamiento explicito.

Resultado final y conteo oficial en SEGUIMIENTO.txt, logs work/linux-user-network-*.
No certifica multiples namespaces reales ni todas las distros; las variantes
de mapeo/dominios se cubren por fixtures, no se presentan como hardware validado.
La compatibilidad MSVC cubre la proyeccion pura, no un Monitor Linux en Windows.
Un ensayo inicial ejecuto tambien Monitor Windows y aborto0xC0000409 despues
de pasar los fixtures puros (log diagnostic); no se afirma causa ni correccion.
Los checks de integracion nativa del fixture se limitan ahora a __linux__,
como su objetivo exige; no se eliminaron asserts Linux ni cambio codigo Windows.
La auditoria N03 conserva ese riesgo, aparte de esta funcion pura portable.
