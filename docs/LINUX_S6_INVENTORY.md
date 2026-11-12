# s6: inventario explícito de un scan tree

Revisión135, unidad de K03. API pública en `pcinsight/linux_s6.hpp`:

```cpp
auto inventory = pcinsight::query_linux_s6_inventory("/service");
// Comprobar value/support, complete/diagnostics y cada Metric de cada fila.
```

El caller selecciona un árbol de confianza: no autodetecta el manager ni recorre
todos los servicios instalados, repositorios s6-rc, dependencias, namespaces o
runlevels del sistema. La [documentación oficial de scan directories](https://skarnet.org/software/s6/scandir.html)
distingue el árbol escaneado del repositorio de servicios. La API NO replica
el estado interno ni la activación efectiva de s6-svscan.

## Filas y disponibilidad

- Directorios directos visibles y aliases ordinarios a directorios. Nombres
  que empiezan por punto, incluida `.s6-svscan`, y nodos no-directorio se omiten.
  Links rotos/candidatos no resolubles o inaccesibles se conservan como filas
  UNKNOWN y hacen incompleto el inventario, no se presentan como stopped.
- Un único nivel de logger `servicio/log` si se resuelve como directorio. Tiene
  `logger=true` y `parent_name`; no recorre `log/log` ni expande otros hijos.
  Cada path visible conserva su fila aunque varios aliases apunten al mismo
  objeto. No se infiere número de procesos o instancias distintas por filas.
- `name` es relativo, `directory` un path imprimible. No son identidad persistente,
  token para controlar ni prueba de que el path aún apunte al mismo objeto.
- `status_record` es el [registro raw35](LINUX_S6.md), leído desde el directorio
  ya fijado, NO reabriendo el path absoluto de cada fila. Available NO significa
  supervisor/proceso vivo, running o readiness efectiva; puede estar obsoleto.
- `real_control_fifo` describe presencia/tipo de un FIFO real en el path control.
  True NO significa permiso de escritura, reader, capacidad de entregar, peer
  s6 autenticado, versión compatible ni daemon vivo. No se abre para escritura,
  se sondea reader o se adquieren locks. Missing/ENOTDIR da false conocido para
  ese predicado estructural; error de acceso/resolución da UNKNOWN. Symlink
  control no es un FIFO real, aunque pudiera apuntar a uno.

Unmanaged puede aparecer con registro UNKNOWN y control false; ello no equivale
a servicio detenido. Registro faltante/inválido no vuelve incompleto el censo de
nombres: disponibilidad por campo es independiente. Empty es árbol observado
vacío, no cero servicios instalados ni prueba de s6 instalado/activo.

## Selección, consistencia y límites

Root absoluto/sin NUL/máximo32768 bytes; máximo total de filas 1..4096, default1024,
contando loggers. Hasta16384 visitas incluyendo dot/ocultos y probes log; 8MiB de
nombres/paths y diagnósticos hasta64. Bounds/error se informan, nunca truncado
silencioso. Orden final por nombre; al llegar al límite no promete ser el primer
subconjunto alfabético. Se abre una descripción independiente para enumeración.

openat2 NO_MAGICLINKS fija root y candidatos. Servicio y logger parten del mismo
pin de padre; se revalida selección por dispositivo/inodo. Registro usa dos
lecturas/type-size-metadata guards y contexto existentes. Control se inspecciona
por O_PATH/NOFOLLOW/fstat, sin abrir FIFO/device para I/O. Root binding/mtime/
ctime y contexto boot/procfs/PID/inicio/UID/namespaces/eUID se comprueban al final.
Contexto cambiado descarta la consulta; cambios de binding conocidos invalidan
observaciones relacionadas y hacen parcial el censo. Otros campos raw observados
pueden permanecer aun con complete=false: no tratarlos como estado actual ni
como autoridad sobre el path final. No hay snapshot atómico, garantía contra
todo ABA, censo estable frente a cambios externos o deadline duro de I/O.

Host serializa fork/setns/credenciales/FD contexto. No caché/worker/poll implícito.
La consulta no ejecuta s6-svscan/svstat/svc/lib externa, scripts, señales ni
órdenes; no crea supervise/event/control/status/lock/down y no cambia árbol.
No infiere descripción/grupo Windows/PID kernel a partir de nombres/jerarquía.

## Evidencia y pendientes

Fixture privada `/tmp`: root ausente sin crear/vacío, alias de servicio y root,
unmanaged, ocultos/configdir, archivo/FIFO raíz ignorados, logger único con
parent y rechazo de recursión, raw35/ausencia/status FIFO sin bloqueo, control
FIFO real/symlink/ausente y lector sin recibir bytes, broken link parcial,
incluyendo log roto distinguido de log ausente,
bounds visibles, repeticiones y FD sin fuga. Sin supervisor/programas/servicios
reales. MSVC solo argumentos/fallback no-Linux; SDK consume nuevo símbolo.
Guardas de carreras están implementadas, no se afirman fixtures deterministas
para cada intercalado posible. No es aceptación s6 real ni comprobación de vida.

K03 sigue abierta por observación/integración/aceptación real y otros init.
El [control FIFO](LINUX_S6_CONTROL.md) permanece separado: el inventario no
entrega handles accionables ni confirma jobs. Revisión136 añade [observación
fijada al target de control](LINUX_S6_OBSERVATION.md), conservando UNKNOWN
cuando no exista evidencia de vida o causa.
OpenRCscripts132 sigue aplazado sin autorización explícita, I07 observador intacto.

Revisión142 añade [consulta explícita ServiceInfo](LINUX_S6_SERVICES.md) con
etiquetas record-*, PID común desconocido y logger/record originales conservados.
No transforma tipo de FIFO/archivo persistente en vida/capacidad de control.

Referencia primaria para directorios y logger:
[s6-svscan](https://skarnet.org/software/s6/s6-svscan.html). La API describe un
censo filesystem propio, no equivalencia con la lista interna del scanner.
