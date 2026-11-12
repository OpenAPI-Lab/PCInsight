# PCInsight 0.2: entrega local para Linux

SDK de lectura y acciones explícitas para integrar en un programa C++20. Esta
entrega local no incluye GUI, no declara ABI 1.0 y no se ha publicado en GitHub.
El perfil ensayado es Ubuntu WSL x86-64 con GCC 13.3 y biblioteca estática Release.

**Entrega consolidada de referencia: revisión 230.** Los paquetes
`pcinsight-sdk-0.2.0-linux-x86_64-gcc13-r230.tar.gz` y
`pcinsight-sdk-0.2.0-linux-x86_64-clang18-r230.tar.gz` sustituyen los paquetes
anteriores descritos más abajo. Cada uno tiene su propio `SHA256SUMS`.
En WSL: GCC 201/201 con recorder de prueba activado y Clang 198/198 sin él;
ambos pasaron las 13 pruebas de entrega, el marcador y los consumidores directos.
La extracción y el manifiesto se verificaron para ambos paquetes. Se conservaron
avisos de marcas de tiempo y un aviso GCC de retorno de `write` ignorado en el
test noexec; no se declara compilación libre de advertencias ni CI remota.

**Actualización Clang 236:** `pcinsight-sdk-0.2.0-linux-x86_64-clang18-r236.tar.gz`
incluye los diagnósticos posteriores de consulta IP y permisos. Su manifiesto,
extracción y SHA-256 se verificaron; el consumidor normal y la prueba de
denegación se ejecutaron contra la copia extraída. No sustituye la evidencia
de suite completa de r230. La revisión GCC237
(`pcinsight-sdk-0.2.0-linux-x86_64-gcc13-r237.tar.gz`) incorpora los mismos
diagnósticos; la consulta normal y la denegación se probaron desde su extracción.
Los 50 encabezados instalados coinciden entre GCC237 y Clang236.

**Verificación local, revisión 181:** desde un build Release existente, instalación
en un prefijo nuevo; suite Linux 200/200, ejemplos instalados 12/12, prueba de
marcador 1/1, mezcla de marcadores rechazada, manifiesto comprobado y consumidor
directo con g++ ejecutado correctamente.

## Integrar y verificar

1. Para integrar el SDK sin CMake en tu aplicación, usa el [ejemplo con g++](LINUX_SDK_SURFACE.md#consumidor-mínimo-de-lectura-verificado). Usa los headers y la biblioteca de la misma entrega y recompila al cambiar de SDK.
2. Para mostrar métricas, sigue el [contrato de disponibilidad](METRIC_AVAILABILITY.md) y la [guía de unidades e identidad](LINUX_SDK_UNITS_IDENTITY.md). Un dato ausente no es cero; una acción rechazada no autoriza un fallback.
3. Para repetir la verificación desde un árbol Release ya compilado, sigue la [guía de pruebas de entrega Linux](LINUX_CI.md). El script instala solo en un prefijo local nuevo, comprueba el manifiesto y prueba consumidores del SDK.

### Usar el SDK comprimido

Para crear un paquete local desde un prefijo ya verificado por el script de
entrega, usa su `sdk-payload.txt` y una carpeta de salida nueva, fuera del SDK:

```sh
bash scripts/package-linux-sdk.sh /ruta/sdk /ruta/sdk-payload.txt /ruta/salida-nueva \
  pcinsight-sdk-0.2.0-linux-x86_64-gcc13-r216.tar.gz
```

El script solo empaqueta los archivos del manifiesto, compara el contenido tras
extraerlo y genera `SHA256SUMS`. No instala ni publica.
Rechaza escribir dentro del SDK de origen, incluso si la carpeta padre se
alcanza mediante un enlace simbólico; la salida debe ser una carpeta nueva.

Si recibes el paquete local de revisión 194, coloca el `.tar.gz` y `SHA256SUMS`
en el mismo directorio. Verifica el hash y extrae en un directorio nuevo:

```sh
sha256sum -c SHA256SUMS
mkdir pcinsight-extracted
tar -xzf pcinsight-sdk-0.2.0-linux-x86_64-gcc13-r194.tar.gz -C pcinsight-extracted
```

Usa `pcinsight-extracted/sdk` como prefijo. El archivo contiene solo el SDK
instalado, incluido su aviso MIT; no contiene GUI, logs ni informes de hardware.
El hash comprueba integridad, no autenticidad. Que el paquete se pueda mover de
carpeta no implica compatibilidad binaria con todas las distribuciones.

El paquete local `pcinsight-sdk-0.2.0-linux-x86_64-clang18-r206.tar.gz` se
verifica del mismo modo con su propio `SHA256SUMS`. Incluye la corrección de
identidad de IP y se probó con el ejemplo Performance compilado con Clang.
La revisión 210 (`pcinsight-sdk-0.2.0-linux-x86_64-clang18-r210.tar.gz`) añade
lectura directa de direcciones por índice y sustituye a la 206 para ese perfil.
Se comprobó su propio SHA-256 y el ejemplo Performance contra el SDK instalado.
La revisión 214 (`pcinsight-sdk-0.2.0-linux-x86_64-clang18-r214.tar.gz`) añadió
la comprobación del nombre e índice de
red; el manifiesto y el ejemplo Performance se verificaron tras extraerlo en
otro prefijo.

La revisión 228 (`pcinsight-sdk-0.2.0-linux-x86_64-clang18-r228.tar.gz`) sustituye
a la 214 como paquete Clang más reciente e incluye `linux_network_addresses.hpp`.
El consumidor de esa API se compiló y ejecutó contra la copia extraída.

Para GCC 13, `pcinsight-sdk-0.2.0-linux-x86_64-gcc13-r216.tar.gz` sustituye al
paquete 194. Incluye lectura por índice, comprobación de identidad y etiquetas
CAN/IP/GRE. Usa su propio `SHA256SUMS` y no mezcles prefijos GCC y Clang.

## Alcance que puede consumir el GUI

| Panel | Contrato y límites |
| --- | --- |
| Rendimiento | [CPU, memoria, discos, red y GPU](LINUX_PERFORMANCE_REQUIREMENTS.md) |
| Procesos y detalles | [Identidad, métricas y acciones](LINUX_PROCESS_REQUIREMENTS.md) |
| Historial | [Contadores observados y límites de atribución](LINUX_APP_HISTORY_REQUIREMENTS.md) |
| Inicio | [Entradas, cambios explícitos e impacto](LINUX_STARTUP_REQUIREMENTS.md) |
| Usuarios | [Sesiones, recursos y controles](LINUX_USERS_REQUIREMENTS.md) |
| Servicios | [Inventario y operaciones por proveedor](LINUX_SERVICES_REQUIREMENTS.md) |

## Lo que esta entrega no valida

- **Hardware físico:** WSL y las pruebas simuladas no validan firmware, drivers ni todas las topologías. La [guía de contribución](LINUX_HARDWARE_EVIDENCE.md) permite comprobar y reportar un campo o dispositivo sin `sudo`. Si no hay fuente implementada, el dato sigue desconocido.
- **Servicios alternativos:** las pruebas privadas no equivalen a [aceptación con supervisores reales](LINUX_K03_ACCEPTANCE.md). No se ofrece control SysV por scripts como si estuviera implementado y autorizado.
- **Versiones y publicación:** el ensayo local no demuestra CI remota, compatibilidad binaria 1.0 ni cobertura Windows. No se instalaron servicios ni se activó un ayudante privilegiado para esta entrega.

## Decisión de salida

| Hito | Estado | Condición restante |
| --- | --- | --- |
| Entrega local 0.2 para integrar el GUI | Verificada | Usar el SDK y el perfil comprobados; no requiere equipo físico adicional. |
| Publicación en GitHub | No realizada; [licencia MIT](../LICENSE) elegida | Confirmar destino y autorización de publicación. Revisar los archivos que se compartirán; no incluir automáticamente prefijos locales, logs ni informes privados. |
| Validación en otros equipos Linux | Seguimiento posterior | Recibir evidencia real de colaboradores; no es requisito para publicar el alcance local verificado. |
| Auditoría global N03 y versión 1.0 | Abiertas | No confundir el hito Linux 0.2 con CI remota, cierre global, ABI estable ni cobertura Windows. |

El alcance por requisito R01–R20 está en la [auditoría Linux](LINUX_DELIVERY_AUDIT.md#alcance-verificable-por-requisito).

Antes del primer commit público, revisar la lista exacta de archivos a compartir.
El `.gitignore` excluye árboles de compilación, evidencia local y coordinación
privada; no elimina archivos ya rastreados ni sustituye revisar los documentos,
fixtures y cualquier adjunto. No subir el workspace completo: el repositorio
debe partir de la raíz de PCInsight, donde están `CMakeLists.txt` y `README.md`.

La licencia de ese repositorio cubre el SDK de PCInsight, no el GUI ni otros
proyectos separados. Las instalaciones nuevas incluyen [LICENSE](../LICENSE)
en `share/licenses/pcinsight/LICENSE` bajo el prefijo por defecto. Si copias solo
la biblioteca o parte del SDK para redistribuirla, conserva también ese aviso.
