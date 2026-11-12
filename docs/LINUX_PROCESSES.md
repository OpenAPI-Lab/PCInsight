# Linux: identidad y contadores de procesos

Actualización161: el cálculo CPU histórico por ticks globales citado debajo fue
sustituido por [D04](LINUX_PROCESS_CPU.md): intervalo propio y CPUs visibles,
con contexto global solo como guardia. Para campos/acciones/unidades/soporte y
ejemplo SDK actual consultar [el contrato de consumo](LINUX_PROCESS_REQUIREMENTS.md).
La evidencia inicial siguiente es histórica, no la fórmula vigente.

Correcciones verificadas el 2026-10-13. El lector interpreta `/proc/PID/stat`
mediante un parser propio: PID, PPID, tiempo usuario+kernel, threads y starttime.
La lectura no requiere una utilidad externa. Nombres comm con espacios,
paréntesis o saltos de línea se delimitan con el último paréntesis de cierre.
Se rechazan entradas truncadas, números inválidos/negativos y sumas que desbordan.

El tiempo CPU acumulado cero es válido: esos procesos ahora se conservan en
el inventario. Antes se confundía cero con un fallo de lectura y se omitían.
Las muestras consecutivas se relacionan por PID **y starttime del kernel**;
PID reutilizado, regresión del contador o primera muestra no producen delta.
Un delta cero válido sí produce 0%. CPU se normaliza contra los ticks totales
del sistema; un contador global regresivo o un resultado superior a 100% no
se publica. No se mide utilización ponderada por frecuencia.

El mismo control de identidad condiciona los deltas de disco y la acumulación
de CPU en app history. Desactivar procesos e historial borra las bases de
comparación; al reactivarlos la primera muestra vuelve a carecer de deltas.

La lectura de E/S requiere ambos contadores read_bytes/write_bytes válidos:
ausencia, lectura inaccesible, duplicados o overflow no se transforman en cero.
Un par de ceros leído correctamente sigue siendo válido. Una muestra inaccesible
elimina su base de comparación; al recuperar acceso necesita dos muestras.
Estos contadores pueden incluir hijos esperados y su semántica de almacenamiento
no equivale a bytes de syscalls ni actividad física instantánea de un disco.

La arquitectura usa clase, endian y e_machine de ELF. Reconoce x86/x64,
ARM32/ARM64, RISC-V32/64 y PowerPC32/64; máquina desconocida o combinación
inválida queda vacía. No se afirma x64 solo porque el binario sea de 64 bits.
Este lector identifica el encabezado; no valida el ejecutable completo.

Pruebas de parser y deltas cubren cero, PID reutilizado, contador regresivo,
nombres especiales, campos truncados, números inválidos y overflow. Fixtures
adicionales cubren E/S ausente/cero/duplicada y ELF ARM, RISC-V, PowerPC big-endian,
campos inválidos y encabezados truncados. La suite
Linux se ejecutó en Ubuntu WSL: 6/6. Windows ejecuta también el parser portable,
con suite 13/13; eso no valida el backend Linux. La integración Linux verifica
la invalidación de muestras al desactivar/reactivar procesos.

Límites actuales: los archivos de `/proc` se leen por separado, no forman una
transacción atómica. La verificación starttime entre muestras no elimina todas
las carreras de lectura durante una muestra ni habilita terminación verificada
Linux. El soporte de E/S ausente todavía no distingue permisos de desaparición
del proceso. Nuevas arquitecturas requieren ampliar el mapa ELF.
La contabilidad de red por proceso/usuario permanece pendiente.

Corrección adicional 2026-10-13: los contadores del panel CPU usan PID actuales
visibles, no el acumulado de creación de procesos. Hilos/FD inaccesibles se
marcan parciales o ausentes. Véase [LINUX_COUNTS.md](LINUX_COUNTS.md).
Después de este cambio: Linux WSL 7/7 y Windows Release 20/20.

Referencias: [campos de proc_pid_stat](https://man7.org/linux/man-pages/man5/proc_pid_stat.5.html),
[contadores de E/S](https://man7.org/linux/man-pages/man5/proc_pid_io.5.html),
[encabezado ELF](https://man7.org/linux/man-pages/man5/elf.5.html).
