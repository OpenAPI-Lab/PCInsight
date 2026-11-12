# Ventanas de red SELF conectadas al reductor

Revision66 conecta `LinuxPrivateNetworkSession` con `LinuxNetworkAccounting`
mediante `src/linux_network_sampler.{hpp,cpp}`. Captura, validacion y tasas ya
funcionan juntas para **un actor SELF y un cookie de namespace de sockets
declarado**. Es interno/experimental, no API instalada, Monitor ni red general
por proceso/app/usuario. F03 sigue en curso; H04/J04 no se cierran.

Revision67 agrega [asociacion SELF con procfs](LINUX_NETWORK_SELF_JOIN.md),
opt-in por metodo separado, sin convertirla en join arbitrario de actores.

## Ambitos sin conversiones inventadas

El contrato privado de scope distingue `namespace_inode` de `kernel_initial`.
El segundo exige IDs numericos de namespace0 (no aplicables), no un inode0
conocido. Helpers BPF entregan TGID y UID en el dominio inicial del kernel;
el scope los etiqueta asi. Nunca usa getpid/getuid o inode de procfs como si
tradujeran esos IDs. No hace join con procfs, nombres de usuario ni sesiones.

`self_single_socket_namespace` exige exactamente una fila con TGID/start del
creador coincidentes. `enumeration_complete=true` certifica **solo esa fila
declarada**, no todos los procesos/sockets/namespaces del sistema. Los cuatro
lanes representan payload de los hooks instrumentados y conservan exclusiones
y limites documentados; no datos de interfaz, ACKs ni trafico fisico global.
Los acumulados pueden incluir operaciones durante el attach secuencial de los
hooks; no certifican cobertura completa de ese arranque. El primer baseline de
tasas se toma solo despues de start, con los tres enlaces ya activos.

El scope contiene boot token observado por procfs, creador TGID/start y epoch
serial no cero por instancia de captura. El serial atomico es compartido por
los samplers de esta copia enlazada de la biblioteca; nunca se envuelve. La
identidad del creador evita colisiones al heredar el serial con fork. No es un
UUID global entre copias de biblioteca/DSO independientes ni un canal de fusion
de productores externos. Cada sampler posee su propio reductor.

El lector nativo abre `/proc/sys/kernel/random/boot_id` sin seguir el symlink
final, verifica procfs mediante fstatfs, lee hasta38bytes y admite solo UUID
hexadecimal canonico minusculo, opcional newline, no cero. No autentica el host
contra un administrador/contenedor que sustituya procfs. Ese token delimita el
observador; no debe promoverse a prueba de identidad global del host.

## Ciclo y muestreo

Sin autorizacion explicita no consulta fuentes ni carga sondas. Cookie0/API
incompleta/boot ausente o estimated impiden iniciar. La captura sigue limitada
a SELF; no recibe PID externo ni filtro global. Mantiene las tres sondas
anteriores, mapa9, BTF y guardas, sin herramientas/paquetes/configuracion nueva.

Cada sample lee CLOCK_MONOTONIC antes y despues del mapa. Ambos puntos deben
ser exactos/no cero, ordenados, no solaparse con la lectura valida anterior y
acotar una duracion maxima1s. Conserva los dos tiempos en salida. Las tasas
usan diferencias entre finales de lectura: **estimated**, no instante atomico
de operacion ni tasa fisica exacta. El contador es coherente bajo las guardas;
el timestamp no se inserta atomicamente desde BPF en el mismo snapshot.

Primera muestra: acumulados conocidos, tasas ausentes. Siguiente intervalo
limpio <=60s: floor(delta*1e9/ns) por TCP envio/recepcion y UDP envio/recepcion,
con el reductor probado. Reposo medido produce0. La suma por UID es siempre
parcial/estimated, con cobertura known/missing: solo el actor capturado, no
todos los procesos ni sesiones de ese UID. No se copia a UserSession.

Ausencia/denegacion/calidad/overflow/reloj invalido/excepcion descartan todos
los baselines del sampler. Recuperar la siguiente lectura no genera una tasa
que cruce el hueco. No se inventa un contador de eventos perdidos: el mapa es
directo y los fallos observados invalidan la muestra, no se recuperan con0.
Los limites de caminos no instrumentados siguen fuera de esta certificacion.

Stop es idempotente y cierra la captura. Reinicio obtiene nuevo epoch y no
hereda tasas. getpid se usa **solo** para reconocer una copia heredada por fork:
si no coincide el proceso local creador, sample cierra la copia de descriptores
y rechaza la lectura, sin publicar al padre como SELF del hijo. No traduce IDs.
La API no es thread-safe; serializar llamadas y no hacer fork durante una llamada.
Los contextos inyectables son prestados y deben sobrevivir al sampler.

## Evidencia

Fixtures nuevos `tests/linux_network_sampler.cpp`: UUID, opt-in, boot/support,
dominios, acumulados/tasas/UID, cero, permiso, calidad/overflow/escritores,
excepciones, tiempo0/retroceso/limite1s, huecos, limite60s, epoch/reinicio y fork.
El reductor amplia pruebas para rechazar scopes SELF invalidos/filas ajenas y
enums desconocidos; conserva pruebas previas/aritmetica/oraculo10000.

Modo manual `pcinsight-linux-network-session-native --authorized-self-window`,
NO CTest, usa la autorizacion temporal WSL privada previa. Prueba real exit0:
TCP4096/4096B,UDP256/256B,8peeks excluidos, las cuatro tasas y suma UID coinciden
con oraculo entero segun intervalo646362ns medido; reposo0, fork rechazado,
reinicio epoch/baseline limpio y sin fuga de FD. Intervalo dependiente de la
ejecucion, no una frecuencia garantizada. Watchdog20s, sockets timeout2s;
sin setns/unshare, contenido retenido, daemon ni objetos persistentes.
Log `work/network-window-native-linux.log`.

Regresion Release Linux117/117;9pruebas de red repetidas20veces cada una;
consumidor instalado reconstruido y ejecutado exit0. Build inicial aviso de
clock-skew0.27s en dependencia startup-impact; build verificado sin warnings.
Logs `work/network-window-{build,verified-build,focused,suite,repeat,install,consumer}-linux.log`.

Pendiente F03: multiactor y buckets netns dinamicos, join exacto de identidades
con procfs/apps/usuarios, cobertura de otros caminos e integracion publica.
No se usa la coherencia SELF como certificado de captura general. Sin cambios
API/ABI/historial publico ni avance Windows en esta unidad.

Fuente primaria para los dominios:
[helpers de identidad Linux6.6](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/bpf/helpers.c).
Ver tambien [sesion](LINUX_NETWORK_SESSION.md), [cookie](LINUX_NETWORK_NAMESPACE.md)
y [reductor](LINUX_NETWORK_ACCOUNTING.md).
