# Preparación BTF para contabilidad de red (F03 en curso)

Módulo interno `src/linux_network_btf.hpp/.cpp`: lee y valida metadatos del
kernel para preparar un futuro proveedor opcional de actividad de sockets por
proceso. **No es un capturador de tráfico ni un cargador eBPF.** No hay nueva API
pública de captura, contador, mapa, programa o enlace adjuntado en esta unidad.
No se usa clang/libbpf/bpftool ni otra utilidad externa para leer BTF.

## Qué resuelve y qué no demuestra

`read_linux_network_btf` abre `/sys/kernel/btf/vmlinux`, con descriptor fijado,
O_NOFOLLOW/O_CLOEXEC, lectura acotada y comprobación de identidad/metadatos antes
y después. El parser portable resuelve un LinuxNetworkBtfLayout interno:

- IDs FUNC exactos y no ambiguos de sock_sendmsg y sock_recvmsg, con definiciones
  no extern, retorno int32 firmado y argumentos esperados (socket*, msghdr*;
  recepción añade int32 flags). No acepta un símbolo del mismo nombre con otra
  firma ni adivina índices del contexto de fexit.
- Desde revisión28 también exige __sys_sendto: retorno int32 firmado y seis
  argumentos int32,void*,uint64,unsigned32,sockaddr*,int32. El slot de retorno
  fexit de esta función es distinto. La prueba real mostró que sock_sendmsg NO
  observa send/sendto de este kernel: el camino usa __sock_sendmsg, ausente en
  su BTF (posible inline, no demostrado). Se conserva sock_sendmsg como layout
  de referencia, no se declara proveedor de envío completo. La prueba privada
  usa __sys_sendto exclusivamente para envíos; no cubre sendmsg/sendmmsg.
- Revisión29 añade firmas opcionales inet_sendmsg/inet6_sendmsg: int32,
  socket*,msghdr*,size_t64. Si no existen, sus IDs internos son0 (no disponibles);
  firma incorrecta/ambigüedad se rechaza. La prueba clasificada usa estos hooks,
  no __sys_sendto, y exige ambos para el positivo IPv4/IPv6. No es disponibilidad
  universal de captura. Ver LINUX_NETWORK_CLASSIFIED.md.
- socket.sk debe apuntar a struct sock. Resuelve __sk_common.skc_family como
  uint16 y sk_protocol como uint8/uint16. Guarda offsets/anchura encontrados,
  **no offsets constantes tomados de otra versión del kernel**.
- task_struct.group_leader debe apuntar a task_struct; start_boottime uint64 y
  flags uint32 deben estar alineados y dentro de sus estructuras.

Son tipos, firmas y offsets verificados en BTF, no valores de la memoria del
kernel. No prueba que fexit esté habilitado, que la función tenga trampoline
usable, que el verificador acepte bytecode o que el usuario pueda cargar/adjuntar
sondas. Support::available significa exclusivamente que este layout compatible
se resolvió; no autoriza convertir ausencia de captura en una tasa0.

No se reutilizan estos IDs/offsets en otro kernel/boot ni se exponen como tokens
de identidad de procesos. start_boottime de task_struct tampoco se presenta como
start_ticks de procfs: cualquier conversión/relación con relojes/namespaces y
la identificación del actor de eventos requieren trabajo posterior.

## Validación y límites

BTFv1 en el byte order nativo, header extensible acotado y flags conocidos.
Regiones de tipos/cadenas dentro del blob y sin solapamiento; tabla de cadenas
con NUL inicial/final, offsets y terminación de nombres comprobados. Cada tipo
tiene longitud de payload acorde al kind/vlen; no se salta a ciegas un kind nuevo.
Se interpretan los19kinds estándar para avanzar y comprobar referencias/nombres.
Esto no pretende sustituir el verificador BTF del kernel para todos los tipos.

IDs referenciados deben existir. typedef/const/volatile/restrict/type_tag se
desenvuelven con profundidad máxima32; ciclos no causan bucles infinitos. Los
campos seleccionados no pueden ser bitfields, no alineados, fuera de la
estructura o con anchura/signo inesperados. Tipos/funciones con nombres
seleccionados ambiguos se rechazan, no se elige el primero arbitrariamente.
El layout inicial requiere punteros de kernel64bits: builds nativos32bits no
se declaran compatibles ni se les aplican offsets de64bits.

Máximo64MiB de fuente,1000000tipos, nombres4096B,4reintentos EINTR del lector.
Se conserva memoria acotada por esos límites, sin caché global ni background.
No se promete un deadline duro de I/O/parsing. Falta de fuente/denegación/
incompatibilidad devuelve ausencia motivada; EPERM/EACCES del lector conserva
permission_required. Backends no Linux no leen la fuente y devuelven ausencia.
Se confía en la fuente del sistema; no es autenticación contra root que sustituya
montajes ni una firma criptográfica del kernel.

## Autorización y siguiente paso

Inspección read-only del entorno WSL: usuario1000, CapEff=0,
unprivileged_bpf_disabled=2, kernel6.6.114.1-microsoft-standard-WSL2 y BTF legible.
En la unidad read-only revisión26 no se modificó ese sysctl, no se asignaron
capacidades ni se ejecutó como root. Revisión28 usa un proceso root temporal
autorizado para la prueba privada; no cambia sysctl/capacidades permanentes.
La lectura BTF positiva no implica permiso para BPF.

En revisión26 se solicitó autorización explícita al usuario para una prueba temporal
de sondas filtradas a procesos privados, retiradas al terminar, sin firewall ni
instalar utilidades. Fue concedida explícitamente el2026-10-18. En revisión26 no
se llama bpf/perf_event_open, no se cargan/adjuntan programas ni mapas y no se
capturan contenidos o eventos de procesos. El futuro cargador y sus filtros,
contabilidad TCP/UDP, pérdidas/overflow/ventanas, identidad/UID/namespaces y
limpieza del proveedor de producción siguen pendientes. La prueba manual de
revisión28 se describe en LINUX_NETWORK_PROBE.md; no hay elevación automática.

F03 permanece E, J04/H04 abiertos y red legacy ausente. No se añade una subtarea
ya resuelta para sumar porcentaje por el parser. La validación de esta unidad
es preparación de layout; no validación positiva de captura TCP/UDP por app.

## Pruebas

Revisión28: fixture ampliado a la firma __sys_sendto y punteros void, con
rechazos por count/anchura/pointee/ambigüedad. BTF nativo final Passed (no skip)
y suite Linux100/100,Windows114/114; prueba eBPF manual limitada aparte en
LINUX_NETWORK_PROBE.md. No se transforma compatibilidad BTF en disponibilidad
universal de captura.

Fixtures portables: layout correcto y offsets, typedef y ciclo, otros kinds,
header/byteorder/flags/regiones/cadenas/truncamiento/IDs malformados, tipos,
firmas/linkage, campos fuera de estructura/no alineados/bitfield, nombres de
hook ambiguos, signo/anchura y punteros32bits incompatibles.
La prueba nativa solo lee el BTF real e imprime el layout o skip77 motivado;
skip no cuenta como positivo. Resultado final: Linux98/98 y Windows112/112 en
Release; ambos consumidores instalados relinkados/ejecutados exit0. La prueba
BTF nativa pasó (no skip) en WSL6.6.114.1: send FUNC126968, recv FUNC126963,
socket.sk24, sock.family16, sock.protocol516/2, task.group_leader2480,
task.start_boottime2728, task.flags44. Son observaciones de ese kernel, no
constantes reutilizables ni evidencia de captura.

El fixture válido falló inicialmente en ambos sistemas: `std::string{1,'\0'}`
seleccionaba initializer_list y producía bytes1,NUL, no un único NUL. Se corrigió
a `std::string(1,'\0')`, sin relajar el parser; se añadió comprobación explícita
del NUL inicial. Pruebas enfocadas finales2/2 Linux y1/1 Windows. Logs bajo
`work/network-btf-focused-{linux,windows}.log` conservan los fallos iniciales;
`network-btf-verified-focused-*`, `network-btf-verified-build-*`,
`network-btf-suite-*` y `network-btf-native-linux.log` conservan verificación
posterior y layout. Builds finales sin warnings. N03 sigue abierto: suites
verdes no identifican ni corrigen sus fallos intermitentes históricos.

Referencias primarias: [formato BTF](https://www.kernel.org/doc/html/latest/bpf/btf.html),
[firmas de sockets](https://github.com/torvalds/linux/blob/master/include/linux/net.h),
[implementación de envío/recepción](https://github.com/torvalds/linux/blob/master/net/socket.c),
[tipos de programa BPF](https://www.kernel.org/doc/html/v6.16/bpf/libbpf/program_types.html).
