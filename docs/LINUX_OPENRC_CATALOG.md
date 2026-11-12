# OpenRC: catálogo explícito init.d/runlevels

Revisión131, unidad de K03. Nueva función pública en `pcinsight/linux_openrc.hpp`:

```cpp
auto catalog = pcinsight::query_linux_openrc_catalog(
    "/etc/init.d", "/etc/runlevels");
// Examinar value, complete, diagnostics y los Metric individuales.
```

Una consulta cubre **un** directorio init.d y **un** directorio runlevels, elegidos
por el llamador. No descubre rutas de librc, variables de entorno, configuración
por usuario, precedencias, directorios locales ni servicios arrancados fuera de
ese scope. Para varios directorios, el host debe consultarlos y decidir cómo
mostrar conflictos; no se simula la resolución efectiva de OpenRC por nombre.

## Datos devueltos

- `scripts`: candidatos regulares o symlinks en init.d, omitiendo ocultos y `.sh`.
  Links normales se resuelven solo mediante O_PATH/fstat; nunca se lee su cuerpo,
  ejecuta el script, abre un FIFO/device para I/O o se invoca shell/librc/CLI.
  `executable_mode` indica cualquier bit de ejecución, no permiso efectivo,
  validez del intérprete, cabecera OpenRC ni capacidad de lanzar. Un link roto o
  destino no regular se conserva con metadata UNKNOWN y censo incompleto.
- `runlevels`: directorios reales directamente bajo el root seleccionado.
  Symlinks de nivel superior no se tratan como aliases soportados: censo parcial.
- `references`: links configurados directamente en cada nivel, omitiendo ocultos
  y `.sh`. Un destino regular tiene forma de referencia a servicio; `script_index`
  solo está disponible si hay **un único** candidato con igual dispositivo/inodo
  en este catálogo completo. El nombre no se usa para fingir asociación. Si dos
  candidatos son hardlinks, no se elige uno arbitrariamente. Un destino externo
  regular conserva la referencia sin inventar un índice local.
- `stacked_level`: un link a directorio se asocia únicamente con un nivel de este
  catálogo mediante dispositivo/inodo. Es arista directa: no hay expansión
  recursiva, herencia calculada, dependencia ordenada ni detección de causalidad.
  Ciclos y enlaces a sí mismo no disparan recorridos infinitos. Destino externo
  o ambiguo queda sin asociación. Referencias rotas/no-link producen UNKNOWN y
  censo parcial, nunca ausencia conocida.

Los índices solo valen para el resultado que los contiene: NO son identidades
persistentes ni autorización para ejecutar. `complete` significa censo acotado
coherente del scope elegido, no que OpenRC esté instalado/activo. Empty no prueba
que el sistema entero carezca de servicios. El resultado describe configuración
filesystem, NO habilitado al boot, runlevel actual, running/PID, readiness o
descripción. Incluso contenido raw de `single` no se presenta como configuración
efectiva: OpenRC tiene reglas especiales que este catálogo no ejecuta.

## Consistencia, límites y seguridad

Raíces explícitas absolutas, máximo 32768 bytes/sin NUL; max_scripts1..4096
(1024 por defecto). Hasta 256 niveles y 16384 referencias. Presupuesto global
de 16384 visitas y 8MiB de nombres por pasada; diagnósticos64 por pasada más
diagnósticos finales. Hay dos pasadas, cada una con descripción de directorio
nueva para no reutilizar offsets de dup. Se comparan listas ordenadas y metadata
de destinos, directorios y raíces; se revalida selección por path/contexto del
observador (boot/procfs/PID/inicio/UID/namespaces/eUID).

Requiere openat2, sin magic links; niveles se abren beneath/no-symlinks. Los
links de configuración pueden apuntar fuera de los roots suministrados pero
solo se inspeccionan por O_PATH. Esto no autentica configuración/supervisor,
no ofrece snapshot atómico, no detecta todo ABA ni impone timeout duro de I/O.
El host serializa fork/setns/credenciales. Raíz faltante/inaccesible, cambios,
errores o bounds: `complete=false`, soporte estimated y asociaciones/metadatos
derivados UNKNOWN, conservando nombres observados. Cambio de contexto descarta
el resultado completo. No se crea ningún directorio o archivo para consultar.

## Validación y pendientes

Fixture nativa privada `/tmp`: raíces ausentes sin crearlas, vacío, orden,
bits ejecutables true/false, archivos ocultos/.sh, directorio/FIFO ignorados en
init.d, alias por inodo, stacks con ciclo, hardlinks ambiguos, destinos externos,
broken/magic links, FIFO en nivel sin bloqueo, nivel alias rechazado, bounds,
repetición y FD sin fuga. No ejecuta scripts ni OpenRC. MSVC solo fallback
no-Linux; consumidor del SDK verifica nuevos símbolos/header. No se afirma
aceptación en un sistema OpenRC real ni pruebas de cada posible carrera.

Se mantiene separado del [lector de marcas](LINUX_OPENRC.md): no se cruzan
nombres para atribuir vida/causa. K03 sigue abierta por control/PID/liveness,
integración y aceptación OpenRC real; runit real/s6/SysV siguen pendientes.
No se escribe una marca de estado para fingir control de un servicio.

Revisión143 añade [vistas explícitas ServiceInfo](LINUX_OPENRC_SERVICES.md):
candidatos y marcas en listas separadas/IDs distintos, sin unir por nombre ni
interpretar marcas como vida. Se conservan ambas fuentes y sus errores propios.

Fuentes primarias revisadas: [librc.c](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/librc.c)
(enumeración, resolución y stacks) y [header público](https://raw.githubusercontent.com/OpenRC/openrc/master/src/librc/rc.h.in).
La API anterior es un catálogo propio, no una reimplementación equivalente de librc.
