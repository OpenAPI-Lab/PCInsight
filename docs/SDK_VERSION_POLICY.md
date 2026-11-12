# A05: política de versiones y criterios para congelar ABI

Revisión 169, 2026-11-06. El paquete sigue en **0.2.0**, C++20, biblioteca
estática. No se ha publicado ni declarado 1.0. Esta política aplica a futuras
entregas etiquetadas; un checkpoint de desarrollo no es una nueva versión.

## Qué debe cambiar con cada entrega

| Tipo de cambio | Ejemplos | Regla para una entrega 0.x |
| --- | --- | --- |
| API de fuente o contrato incompatible | Renombrar un método, cambiar unidades, significado de cero/ausencia, identidad, permisos o alcance de una acción | Incrementar minor; anotar contrato previo, nuevo, migración y prueba. La firma por sí sola no define compatibilidad. |
| ABI incompatible | Agregar/reordenar miembros de un struct público, cambiar tipos/align, firmas enlazadas, vtable o convenciones | Incrementar minor y exigir recompilar todos los consumidores. Un miembro agregado al final también puede cambiar tamaño y protocolo de llamada. |
| Adición compatible | API opt-in separada sin alterar tipos, firmas ni comportamiento previo | Minor para nueva funcionalidad pública. Verificar el contrato previo; no inferir compatibilidad de que la adición compile. |
| Corrección compatible | Restaurar conducta documentada sin cambiar contrato/ABI | Patch, registrar evidencia y límites. Si obliga a migrar datos/código, no clasificarla como compatible solo porque sea una corrección. |
| Formato persistido | Cambiar estructura o semántica de un archivo de historial o startup | Revisar y versionar el formato por separado; documentar lectura, escritura, migración, pérdida y rollback según el formato. La versión del SDK no sustituye el número del archivo. |

Un paquete publicado no se reemplaza por otros bytes bajo la misma identidad
de distribución. Para checkpoints no publicados que todavía anuncian 0.2.0,
identificar los artefactos con revisión, perfil de compilación y manifiesto.
El paquete CMake mantiene `ExactVersion`; no se promete que todos los builds
0.2.0 sean intercambiables. Headers y biblioteca del mismo SDK, toolchain y
opciones compatibles; reconstruir al cambiar de SDK. No es necesario usar
CMake en la aplicación GUI.

## Tres verificaciones, tres alcances

1. [Marcador de headers](SDK_HEADER_COMPATIBILITY.md): comprueba por enlace el
   conjunto de headers marcado. No identifica cambios solo en implementación.
2. [Línea base Linux](LINUX_ABI_BASELINE.md): compara contratos seleccionados y
   tamaño/alineación bajo el perfil registrado. Igualdad no demuestra ABI completa.
3. [Manifiesto del paquete](../tests/linux_delivery/sdk_payload_manifest.cmake):
   SHA-256 de cada archivo instalado, por ruta relativa ordenada. Identifica
   los bytes de una entrega local incluso si conserva versión y headers. No es
   firma, autenticación, prueba de compilación reproducible ni garantía ABI.

Para comprobar el manifiesto con el mismo paquete, usando un prefijo de SDK
Linux dedicado y una carpeta de trabajo ya existente:

```sh
cmake -DSDK_PREFIX="$PWD/work/sdk-headers-167-linux" \
  -DAUDIT_OUTPUT="$PWD/work/linux-release-169" \
  -P outputs/pcinsight/tests/linux_delivery/check_payload_manifest.cmake
```

La [prueba](../tests/linux_delivery/check_payload_manifest.cmake) registra el
contenido, vuelve a compararlo y cambia el hash del `.a` **en una copia del
manifiesto**, no en la biblioteca. La comparación debe rechazar esa fila. El
script de lectura no instala ni ejecuta componentes. Un manifiesto anterior
solo sirve como referencia si se conserva por un canal de confianza.

## Cambios conocidos que motivan la política

- [Historial de red Linux, revisión 100](LINUX_NETWORK_APP_HISTORY.md): se
  amplió `SystemSnapshot`. Recompilar; no tratar el informe separado como parte
  del acumulado CPU ni cambiar por esto el formato legacy.
- [Selección de servicios, revisión 144](LINUX_SERVICE_SELECTION.md): se añadió
  un campo a `SampleOptions`. Recompilar incluso si el caller no lo usa.
- [Marcador opcional, revisión 167](SDK_HEADER_COMPATIBILITY.md): se añadió un
  header y un símbolo; no cambió el layout existente. Usar el archivo estático
  nuevo cuando se opta por la comprobación.
- [Persistencia](HISTORY.md): no confundir el formato legacy con los almacenes
  observados; ausencia y contador medido cero siguen siendo distintos.

## Criterios antes de declarar 1.0

| Puerta | Evidencia exigida | Estado en esta revisión |
| --- | --- | --- |
| Contrato público | Inventario de headers/símbolos estables, unidades, identidad, disponibilidad y efectos; cambios incompatibles con instrucciones de migración. Separar APIs experimentales sin prometerles estabilidad. | N03 tiene trazabilidad Linux, pero la auditoría global sigue abierta. |
| Matriz ABI | Fijar perfiles soportados (arquitectura, compilador, STL, runtime y opciones). Baselines y comparaciones para esos perfiles: layouts relevantes, símbolos y vtables; pruebas con consumidores compilados contra la base anterior. | Hay sonda parcial de un perfil Linux y verificación del marcador Linux/MSVC; no hay matriz completa. |
| Entrega y actualización | Instalar en prefijo limpio, enlazar consumidores, comprobar versión, huella y manifiesto, y probar compatibilidad de formatos/migraciones que se anuncien. | Auditoría Linux local comprobada; no equivalente a publicar una entrega. |
| Alcance y aceptación | Validación real de lo anunciado, CI de los perfiles soportados, disposición explícita de requisitos abiertos y riesgos históricos. | N01, B08/G07/N02, K03/I07/L05 y los pendientes Windows no se cierran aquí. |

No fijar el ABI ni aflojar `ExactVersion` solo porque todas las pruebas
locales pasen. Una futura compatibilidad binaria se anunciará únicamente para
perfiles y superficies comprobados. A05 sigue en curso; no cambia el alcance
Linux primero ni se infieren autorizaciones nuevas.

Revisiones 174–176: verificados [errores y disponibilidad](METRIC_AVAILABILITY.md),
[unidades e identidad Linux](LINUX_SDK_UNITS_IDENTITY.md) y el rechazo de mezcla
con comprobaciones en dos objetos del mismo ejecutable. La versión de paquete
rechaza una versión diferente, pero no distingue dos artefactos del mismo
0.2.0. El marcador tampoco cubre objetos sin la llamada, y la línea base de un
perfil no equivale a comparar consumidores históricos en todos los perfiles.
Estas comprobaciones no sustituyen el inventario de superficie que se prometerá
estable ni la validación con las herramientas y opciones que se decida soportar.

Revisión 179: [inventario del SDK Linux](LINUX_SDK_SURFACE.md), 49 encabezados
conciliados con fuente y paquete instalado. La clasificación por encabezado
orienta al GUI; no congela todos los símbolos de la base candidata.

Verificación local: 54 archivos del SDK instalado, comparación positiva y
rechazo del digest alterado, marcador 1/1, sonda ABI 3/3 y Linux 200/200.
Evidencia `work/linux-release-169/`. Oficial 60/75=80%.
