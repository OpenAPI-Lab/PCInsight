# Sesiones reportadas y consumo por usuario Linux

Actualización J04 parcial: [LINUX_USER_IO.md](LINUX_USER_IO.md) sustituye la
atribución de disco por username por UID numérico/intervalo con cobertura tipada.
Los campos disco/red legacy reflejan ese informe; utmp sin UID queda sin E/S.
CPU/RAM conservan username. Las referencias a sumar disco/red por username más
abajo describen el contrato histórico previo, no el actual. Red F03 sigue pendiente.

Actualización J01: [LINUX_SESSION_STATE.md](LINUX_SESSION_STATE.md) completa
estado/actividad/origen por campo de login1 y cobertura explícita de la fuente
seleccionada, incluso inventario vacío/fuente ausente. El contrato siguiente de
agregación utmp se mantiene; no se transforma reported_login en sesión activa.

Unidad funcional terminada: agregación de recursos visibles por username y
conservación de múltiples registros de sesión utmp. La versión anterior
reemplazaba la sesión previa del mismo usuario, sumaba métricas ausentes como
si fueran cero y podía mostrar todo a cero con include_processes=false.

## Contrato

- Una fila por ut_line único, con username del registro USER_PROCESS. Registros
  duplicados idénticos se deduplican; el mismo ID con username/status distinto
  falla explícitamente, no elige un usuario arbitrariamente.
- status=`reported_login`: utmp informa un login, no prueba que esté activo,
  unlocked, conectado, ni que el PID registrado siga siendo la misma identidad.
- `resource_scope="linux-visible-processes-by-username"`: recursos agregados de
  procesos visibles de ese username, **no del login/TTY individual**. Dos sesiones
  del mismo usuario repiten el mismo total; el GUI debe agrupar/deduplicar por
  usuario antes de calcular totales. No sumar esas filas como usuarios distintos.
- Si se solicitan usuarios, Monitor recopila procesos internamente aunque
  include_processes=false; la lista de procesos no se devuelve si está desactivada.
  La primera muestra CPU sigue desconocida hasta disponer de deltas. El costo
  de procfs no desaparece por ocultar la lista de procesos.
- Cada métrica se calcula por separado. Cero con una lectura válida se conserva.
  Ausencia de valores accesibles -> nullopt, nunca cero; permission_required se
  conserva si no hay valores y alguna lectura requería permiso.
- Suma conocida -> estimated con conteos conocidos/ausentes. La enumeración
  procfs puede ser parcial, oculta por permisos/hidepid/namespaces, y no es un
  snapshot atómico. Incluso todos los valores visibles disponibles no certifican
  el total de todo el sistema. Datos parcialmente conocidos conservan su suma
  explícitamente estimada; no se extrapola la parte desconocida.
- NaN/infinito/CPU negativa o >100 por proceso y valores con soporte
  unsupported/permission_required se omiten como ausentes/invalidos. Suma CPU
  >100 o overflow uint64 -> nullopt; no hay clamp, wrap ni saturación inventada.
- Procesos con PID duplicado/cero invalidan la entrada, para no contar dos veces.
  Username vacío/desconocido no se asocia a un login. El collector ya no inventa
  UID 0/root cuando /proc/PID/status no proporcionó Uid.

## Límites de atribución

La asociación usa username publicado por NSS y utmp, no un UID global validado.
Nombres truncados en utmp, aliases de UID, namespaces o cambios NSS pueden
impedir asociación o representar la misma cuenta bajo distintos nombres.
Se usa el UID real publicado por status, no se determina responsabilidad causal
de cada recurso ni procesos de una sesión concreta. La suma de RSS puede contar
páginas compartidas varias veces; no es RAM física única ni PSS.
Disco suma tasas procfs por proceso; no prueba active time físico. Red por
proceso Linux sigue sin proveedor universal; no se presentan ceros falsos.

La presencia de USER_PROCESS no certifica todos los usuarios conectados.
Contenedores/distro/sesiones gráficas pueden no mantener utmp; archivo ausente
devuelve lista vacía de registros reportados, **no prueba que no haya sesiones**.
Fallos de lectura distintos de EOF/not-found se propagan y no devuelven una
enumeración parcial como completa. La actualización J01 añade metadatos públicos
de la fuente seleccionada, NO prueba de cobertura global del host. login1 es
proveedor opcional separado, con su estado nativo y límites; ver LINUX_SESSION_STATE.md.

getutxent es estado global de libc; el mutex serializa este proveedor solamente.
El host debe coordinar otras lecturas/cambios utmp/NSS. El proveedor no llama
utmpxname, no cambia la ruta ni escribe utmp real. Ningún CLI externo ni helper.
No hay deadline global para libc/procfs/NSS. Límites: 10000 sesiones, 100000
registros utmp examinados/procesos recibidos, cadenas 4096 bytes y payload de
identidades hasta 16 MiB. Exceso/identidad inválida provoca excepción explícita.

Esta unidad no valida una desconexión real, gestión de cuentas ni control de
sesiones. Corrección posterior documentada en LINUX_DISCONNECT.md retira el
camino legacy utmp/SIGHUP; disconnect_available=false en este proveedor.
No utilizar esta prueba de inventario como evidencia de detach/logind funcional.
Campo público UserSession ampliado en 0.x: reconstruir librería/consumidores.

## Verificación

2026-10-14: Linux Release 19/19 y Windows Release 30/30. Paquetes instalados
y consumidores externos reconstruidos/ejecutados en ambos sistemas.

Tests portables: múltiples sesiones, deduplicación/conflicto, usuario sin
procesos, métricas ausentes/parciales/cero, permisos, PID duplicado, nombres
inválidos, NaN/infinito, sumas fuera de rango y límites. Windows verifica solo
esta lógica inyectada, no utmp/procfs.
Test Linux: utmp privado con dos sesiones del usuario caller y una sin procesos,
DEAD_PROCESS ignorado; Monitor con procesos ocultos devuelve RAM conocida,
primera CPU desconocida y segunda CPU calculada e igual en ambas sesiones.
Red ausente permanece desconocida. Se restaura el selector utmp exclusivamente
del proceso de prueba y se eliminan sus archivos privados; ninguna sesión real
se crea, edita ni desconecta.

Referencias: [utmpx/libc y estado compartido](https://man7.org/linux/man-pages/man3/getutxent.3.html),
[utmp](https://man7.org/linux/man-pages/man5/utmp.5.html).
