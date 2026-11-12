# A05: comprobar la pareja de headers y biblioteca del SDK

Revisión 167, 2026-11-06. El SDK sigue en 0.2.0. El número de paquete y
`find_package(... EXACT)` no distinguen dos builds 0.x que hayan cambiado un
tipo público sin cambiar ese número. El header generado `pcinsight/sdk_headers.hpp`
añade una comprobación de enlace explícita y opcional, sin cambiar layouts,
vtable, semántica de las APIs existentes ni formatos persistidos.

```cpp
#include <pcinsight/sdk_headers.hpp>

int main() {
    pcinsight::require_matching_sdk_headers();
    // A continuación, crear Monitor y el resto de los objetos del SDK.
}
```

Compilar con el include del SDK instalado y enlazar su archivo estático.
El header no es un archivo del árbol fuente: usa los headers del prefijo
instalado. En el árbol de build, el header también está disponible bajo
`generated/include`, como include explícito para quien quiera usar el marcador
antes de instalar. No se requiere CMake para tu GUI. El código existente
que no incluye ni usa el marcador sigue sin ese requisito.

La huella SHA-256 combina nombres relativos ordenados y bytes de los `.hpp`
públicos más el template del marcador. La generación registra los archivos
como dependencias de configuración. La función referencia un símbolo C
específico de esa huella. Un archivo estático sin ese símbolo produce un
error de enlace; si coincide, la llamada no hace I/O ni acciones. La función
envolvente tiene linkage interno para que otro TU no sustituya su marcador.
Colocar la llamada en código alcanzable que realmente se enlace. No es una
comprobación global automática de todos los objetos y plugins de un proceso.

## Límites

- Compara el marcador de un **conjunto empaquetado**, no relee los headers
  en el consumidor. Copiar solo el marcador correcto sobre headers ajenos,
  o mezclar objetos sin comprobación, puede ocultar una mezcla.
- Es conservadora: un cambio de comentario o final de línea puede cambiar
  la huella sin cambiar ABI. No sirve de firma ni autenticación del paquete.
- No detecta opciones de packing, ABI de la biblioteca estándar, flags,
  toolchains, arquitecturas o dependencias distintas con los mismos bytes.
  Seguir usando toolchain/configuración compatibles y recompilar al cambiar
  el SDK. No sustituye un análisis de ABI ni declara estabilidad 1.0.
- Un SDK anterior sin el marcador no se puede comprobar con esta llamada;
  al adoptar el header nuevo, reinstalar/enlazar también la biblioteca nueva.

El [consumidor de prueba](../tests/sdk_headers_consumer/CMakeLists.txt) usa
solo el SDK instalado. Prueba compilación/enlace del marcador real, compila
un marcador sintético distinto y exige rechazo de **enlace** en ese símbolo.
No confunde un fallo de compilación con un rechazo ABI ni afirma haber
validado todas las revisiones históricas. A05 sigue abierto.

La prueba también enlaza dos unidades de traducción con marcadores correctos y
repite con un marcador sintético distinto en la segunda. Ambas llamadas son
alcanzables. El caso mixto debe fallar por el símbolo de la segunda unidad:
una comprobación correcta en `main` no debe ocultar la huella del otro objeto.
Esto no detecta objetos que no incluyan y llamen a su propia comprobación.

Verificación: SDK instalado en Linux y MSVC Release, ambos consumidores
positivos ejecutaron exit0 y ambos alternativos compilaron pero fallaron en
el símbolo esperado al enlazar. En Linux, el archivo anterior de la auditoría
166 también rechazó el marcador nuevo, el enlace directo con g++ funcionó y
la suite pasó 200/200. Se conservaron avisos de clock skew del montaje WSL,
sin cambiar el reloj. Logs `work/sdk-headers-*`. A05 en curso; oficial 60/75=80%.

Como comprobación complementaria, la [línea base Linux](LINUX_ABI_BASELINE.md)
registra firmas seleccionadas y tamaños/alineación en un perfil concreto.
La [política de versiones](SDK_VERSION_POLICY.md) distingue ambas comprobaciones
del manifiesto de bytes de una entrega y de una garantía ABI 1.0.
