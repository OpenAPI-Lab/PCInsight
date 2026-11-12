# Asociacion directa SELF entre kernel y procfs

Revision67 anade `sample_joined_self()` al sampler privado. Devuelve una ventana
de red junto con un token local PID/start_ticks/UID y contexto procfs/namespaces.
Es una asociacion **del propio caller**, no una traduccion general de PID/UID ni
un join de procesos ajenos. No cambia API instalada, Monitor ni historial.

## Fuente procfs nativa

`read_linux_network_self()` no ejecuta utilidades, carga BPF ni consulta otro
proceso. Abre /proc como directorio sin seguir el symlink final y verifica
procfs con fstatfs. `readlinkat(root,"self")` debe coincidir con getpid del
caller: si el montaje pertenece a otro dominio visible de PID, no se adivina.
Abre el directorio numerico y el del thread llamante de forma relativa a ese
root fijado; valida procfs en directorios y datos. No recibe rutas/PID externos.

Lee stat antes/despues y status del lider y del thread; exige PID/Tgid/UID
consistentes con getpid/gettid/getuid, start_ticks no cero/estable. Parser acotado
a64KiB, campos duplicados, NUL, desbordes, signos, Uid incompleto y datos extra
se rechazan. NStgid/NSpid no se interpretan como identificadores iniciales del
kernel. No convierte start_boottime en ticks, ni ticks en nanosegundos.

Los magic links de ns/pid y ns/user se abren intencionadamente desde procfs,
validan nsfs y conservan device/inode en el token. Caller y lider deben tener
el mismo contexto; se verifica nuevamente al final. Los archivos normales
usan NOFOLLOW/CLOEXEC/NONBLOCK. Read acotado a64KiB y hasta4reintentos EINTR;
todos los FD son propios y se cierran por RAII, incluido error/excepcion.
No se cambia namespace, UID, permisos, montaje o configuracion.

El token tambien conserva device/inode del root procfs y el inode del directorio
de proceso. Son contexto de la observacion, no identidad autentica del host
contra un administrador capaz de sustituir montajes, ni handles permanentes
que impidan reutilizacion futura de inodes. No se exporta un mapa universal de
namespaces ni se interpreta un numero de inode como cookie de socket.

## Asociacion con la captura

Solo tras inicio autorizado y con el guard de proceso creador valido:

1. Leer identidad procfs local, validar su token y PID del caller.
2. Ejecutar el programa de identidad no adjuntado, TEST_RUN local con BTF
   almacenado al iniciar, mapa/programa temporales propios. Exigir TGID/UID
   iniciales y start_boottime del lider iguales al token fijo de captura.
3. Verificar boot token, muestrear la ventana y repetir identidad del kernel,
   procfs y boot token. Exigir identidad local completa estable en ambos extremos.

Si falta fuente/permiso, cambia identidad/UID/namespace/boot, o hay excepcion,
no se devuelve la ventana asociada y se descarta baseline. Un fallo despues
de que el reductor haya calculado una tasa tampoco deja esa tasa utilizable.
Se conserva la propagacion permission_required y nunca se fabrica0.

Entre muestras asociadas se exige el mismo token local completo; cambio de
contexto empieza baseline sin tasa. Una llamada raw `sample()` invalida la
continuidad local: la proxima asociada tambien empieza sin tasa. Stop/reinicio/
fork conservan las guardas anteriores. El mismo objeto no admite llamadas
concurrentes ni fork durante una llamada; contextos inyectados son prestados.

La comprobacion es por extremos, **no** un snapshot atomico de credenciales ni
prueba de ausencia de cambios transitorios ABA entre lecturas. La sonda sigue
exigiendo su UID/start fijo en cada operacion instrumentada positiva y falla
sticky ante discrepancia. Los contadores acumulados pertenecen al token del
kernel; no se convierten retroactivamente a totales de una cuenta local.
El token local acompana la ventana, no autoriza asignar todos los bytes pasados
a un username, app/exe o sesion actual. Las tasas siguen estimated por finales
de lectura y la suma UID sigue parcial, segun LINUX_NETWORK_WINDOW.md.

## Pruebas y limites

`tests/linux_network_self.cpp`: parser/limites/duplicados y lector nativo solo
lectura --native, con20observaciones iguales y comparacion de FD antes/despues.
El nativo retorna77 cuando no se puede verificar; no se cuenta como positivo.
Los fixtures ampliados del sampler usan deliberadamente PID local7 frente a
TGID kernel123 y UID local42 frente a UID kernel1000: no igualdad numerica
como sustituto de la asociacion directa. Prueban10campos locales cambiados,
estimated/denegacion, identidad kernel distinta en3campos, cambio boot,
recuperacion sin tasa cruzando el fallo, cambio de contexto entre ventanas y
alternancia raw/joined. No cambian credenciales/namespaces reales del usuario.

Modo manual `--authorized-self-joined-window` del ejecutable de sesion: conserva
la prueba temporal eBPF WSL autorizada y solo loopback privado. Comprueba identidad
local real, cuatro tasas/UID contra oraculo, reposo0, fork, reinicio y limpieza.
No es CTest ni se activa en Monitor. No requiere herramientas/paquetes externos.

Verificado2026-10-21: Linux Release119/119, sin skips;11pruebas de red20repeticiones
cada una, consumidor instalado reconstruido y ejecutado exit0. Lector procfs
nativo no privilegiado PID950/UID1000/start18067,20lecturas estables y sin fugaFD.
Captura asociada WSL root exit0: PID local390 frente a TGID kernel4965, UID0/0,
TCP4096/4096B,UDP256/256B,8peeks,4tasas/UID contra oraculo, intervalo793949ns,
idle0/fork rechazado/reinicio limpio y sin fugaFD. Son datos de esas ejecuciones,
no IDs/intervalos constantes. Logs `work/network-self-join-*-linux.log`.
Warning inicial de indentacion corregido sin cambio de semantica; builds con
avisos transitorios de timestamp WSL hasta0.27s preservados. Build final sin
warnings. No se omitieron pruebas para obtener estos resultados.

Pendiente F03: asociacion de actores externos con identidad/credenciales al
evento, buckets multiactor/netns dinamicos, cobertura adicional e integracion
publica; descripcion/exe/apps y agregados completos siguen pendientes. H04/J04
no se cierran por demostrar SELF. Windows sigue aplazado.

Fuentes primarias: [procfs](https://docs.kernel.org/filesystems/proc.html),
[status y dominios NStgid](https://man7.org/linux/man-pages/man5/proc_pid_status.5.html),
[helpers de identidad Linux](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/bpf/helpers.c).
