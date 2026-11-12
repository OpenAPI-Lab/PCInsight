# Identidad y calidad de contadores privados (F03 en curso)

Actualizacion revision64: [sesion interna SELF](LINUX_NETWORK_SESSION.md) extrae
inicio/lectura/detach/stop con propiedad y rollback probados. Lo siguiente
conserva la evidencia historica revision30; aun faltan netns/join/integracion.

Revisión30 añade el modo manual --authorized-quality a la prueba existente.
Usa la autorización explícita del2026-10-18: solo procesos privados WSL,
root temporal, sin configuración permanente/pinning/herramientas instaladas.
No se ejecuta desde CTest ni Monitor. No hay API pública/layout/ABI/historial
nuevos y todavía no es el proveedor de producción.

El programa de identidad RAW_TRACEPOINT no adjuntado se ejecuta localmente con
TEST_RUN. Escribe TGID del kernel, UID real del dominio inicial y start_boottime
del líder en un mapa propio3entradas. Lecturas BTF/helper fallidas o status no
cero impiden aceptar resultados parciales. No traduce PID/UID de procfs/sesiones.

La sonda mantiene el filtro TGID temprano y comprueba group_leader.start_boottime
y UID del kernel antes de leer contexto/socket. Mismo TGID con otra generación
o UID no recibe bytes; la identidad rechazada se registra como fallo de calidad,
no como tráfico0 conocido. Threads del mismo líder comparten el token correcto.
Offsets provienen del BTF, no de conversiones aproximadas de start_ticks.

Con identidad configurada, el ARRAY propio tiene9entradas:0..3payload TCP/UDP,
4fallos de identidad/lectura,5recepciones positivas excluidas,6flag sticky de
overflow,7escritores activos y8secuencia de actualizaciones completadas.
Cada escritura entra en7, actualiza contador/flag y secuencia, y sale de7 al
final. Operaciones atómicas fetch-add/fetch-OR; no se usa spin_lock de tracing.
Las búsquedas que fallan después de entrar dejan activo>0: fallo cerrado, no
métrica válida. La instancia debe usar el esquema ARRAY9/fresco/privado exacto.

Fetch-add detecta suma uint64 envuelta y fija flag6, también para la secuencia.
**Detecta overflow, no satura numéricamente el valor bruto.** El flag nunca se
limpia en la instancia y el validador rehúsa publicar esos valores aunque el
contador bruto haya envuelto. No se arregla bajando/limpiando el flag en vivo.
El contador de escritores está acotado por concurrencia real, no es acumulado.

El lector manual lee escritor/secuencia, contadores, secuencia/escritor, con
fences seq_cst entre fases. Solo acepta ambos escritores0 y secuencias iguales,
sin flag ni fallos de calidad. Hasta8intentos ante concurrencia; si sigue ocupado
devuelve ausencia, no0. Excluidos PEEK/TRUNC/ERRQUEUE siguen explícitos. La
validación se ha probado en WSL x86_64: no es garantía universal de orden de
memoria/coherencia para otras arquitecturas o kernels.

Fixtures portables amplían tests/linux_network_classified.cpp: descubrimiento
de identidad, UID uint32 alto, tokens/UID/nulos/lecturas fallidas, escritura
siempre bajo guarda, overflow de bytes/secuencia y persistencia del flag;
validador rechaza escritores activos, secuencia cambiada y calidad incompleta.

Prueba real exit0: PID475/TGID1640, start103233269011ns, UID inicial0. Matriz
send/sendto/sendmsg TCP/UDP IPv4/IPv6 más dos threads privados del mismo líder:
TCP6144/6144, UDP384/384,12peeks excluidos,0fallos,36actualizaciones y escritores0.
10lecturas coherentes y0ausencias durante el ensayo concurrente: **no** demuestra
que se haya ejercido el rechazo por solapamiento en vivo; ese caso está cubierto
por fixtures. Hijo excluido y retirada comprobados. Tokens de inicio/UID erróneos
rechazados sin bytes; overflow de bytes y secuencia inyectado en mapas privados
produce ausencia, con guardas balanceadas. No se cambia UID real ni reloj.
Evidencia: work/network-quality-native-linux.log y logs con el mismo prefijo.

Regresión final Release: Linux101/101 y Windows115/115; ambos consumidores
instalados relinkados/ejecutados exit0. Builds finales sin warnings. Logs
network-quality-suite-{linux,windows}.log y network-quality-verified-build-*
bajo work/. No hubo que omitir ni relajar pruebas para obtener el positivo.

F03 sigue E: falta extracción a un productor con ciclo de vida/API adecuados,
cookie netns/ámbitos/join procfs-apps-usuarios, cobertura completa declarada,
integración con reductor/Monitor y más kernels/hardware. No se conecta el mapa
experimental automáticamente ni se cierra J04/H04. N03 previo se conserva.
El usuario pidió saltar etapas que se alarguen; no se amplía esta unidad más allá
de su prueba y regresión, y no se cuenta como subtarea cerrada.

Referencias primarias: [identidad helpers Linux6.6](https://raw.githubusercontent.com/torvalds/linux/v6.6/kernel/bpf/helpers.c),
[ISA atómica](https://docs.kernel.org/bpf/standardization/instruction-set.html),
[restricciones de spin_lock](https://raw.githubusercontent.com/torvalds/linux/v6.6/include/uapi/linux/bpf.h).
