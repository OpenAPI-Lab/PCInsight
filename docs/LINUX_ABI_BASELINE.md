# A05: línea base de compatibilidad del SDK Linux

Revisión 168, 2026-11-06. El SDK se mantiene en 0.2.0. Esta línea base
permite revisar cambios en tipos públicos de uso habitual sin afirmar que
el ABI esté congelado ni que una comparación positiva baste para declararlo.

## Qué controla

- La [sonda del SDK instalado](../tests/linux_abi/probe.cpp) comprueba firmas
  seleccionadas y tipos de campos de unidades/identidad al compilar. Incluye
  muestreo, historial, acciones, detalles de servicio y propiedades de inicio.
  No invoca esas acciones. Al enlazar también exige el [marcador de headers](SDK_HEADER_COMPATIBILITY.md).
- Registra `sizeof` y `alignof` de 28 tipos seleccionados. Compara el perfil
  antes de comparar filas: Linux x86_64, GCC o Clang, fecha de headers libstdc++, ABI
  de `std::string`, modo de lenguaje, tamaño de puntero y flags debug/assertions
  de libstdc++. Un perfil no coincidente no se califica como pase.
- Un cambio de tamaño/alineación falla e indica la primera fila distinta.
  Dos fixtures verifican el rechazo de un tamaño alterado y de un perfil
  distinto, con códigos y diagnósticos separados.

Desde la revisión 204, también se comparan ocho offsets de `ActionResult` y
`LaunchRequest` en los perfiles libstdc++ sin modo debug. Un fixture modifica
solo un offset, dejando tamaño y alineación intactos, y exige el rechazo.

Esto no inspecciona todos los offsets, vtables, convenciones de llamada,
símbolos completos, ni todos los flags o semánticas que pueden
cambiar ABI. Dos layouts distintos pueden tener el mismo tamaño y alineación.
La comparación caracteriza al consumidor compilado en el perfil registrado,
no demuestra la equivalencia de todos los objetos de la biblioteca ni de
plugins de terceros. No usar el archivo como norma portable para otro GCC,
otra libstdc++, otra arquitectura o MSVC.

## Revisión de cambios 0.x

Cuando cambia un header público, revisar la huella y recompilar consumidores
con el mismo SDK. Cuando cambia una firma, un campo, su unidad o significado,
anotar la ruptura o cambio de contrato incluso si los tamaños no cambiaron.
Mantener incompatibilidades de archivos persistidos como una revisión aparte;
esta sonda no lee ni migra esos formatos. No actualizar la línea base para
hacer pasar un test sin revisar y registrar primero la diferencia. Para un
perfil nuevo, conservar evidencia aparte; no sustituir el perfil existente
por uno distinto y llamarlo compatible.

Revisión 201: [Clang 18.1.3 con libstdc++](../tests/linux_abi/baseline-linux-x86_64-clang18.txt)
tiene su propio archivo. Los 28 tamaños y alineaciones observados coinciden
con las filas de GCC 13.3 en este entorno, pero los perfiles no son
intercambiables. Para reproducir con el SDK Clang, añade
`-DCMAKE_CXX_COMPILER=clang++` y selecciona el archivo Clang en
`PCINSIGHT_ABI_BASELINE`. Esto no añade un perfil libc++ ni congela ABI 1.0.

Revisión 229: ambas líneas base incorporan `LinuxInterfaceAddressList` (56 bytes,
alineación 8 en estos perfiles) y la sonda comprueba la firma de la consulta.
Son 29 tipos medidos; las filas de los tipos existentes no cambiaron.

Revisión 202: prueba independiente de reenlace. Por perfil, se compilaron una
sola vez los objetos de `gui_read_consumer` y `result_contract` usando los
headers de un SDK anterior. Cada objeto se enlazó y ejecutó con su biblioteca
original y con la actualizada: GCC 13.3 (185 → 194) y Clang 18.1.3 (197 → 200),
ocho ejecuciones exitosas. Solo cubre esas interfaces, perfiles y pares de
revisiones; no sustituye la recomendación de recompilar al actualizar el SDK.

Para repetir la prueba desde la raíz de PCInsight, usa el mismo compilador
que se usó para ambas bibliotecas y una carpeta de evidencia nueva:

```sh
bash scripts/check-linux-sdk-upgrade.sh g++ /ruta/sdk-anterior /ruta/sdk-nuevo /ruta/evidencia-nueva
```

El script conserva el objeto compilado con los headers anteriores, los dos
ejecutables por consumidor, la versión del compilador y los hashes de ambas
bibliotecas. Rechaza usar el mismo prefijo dos veces y no sobreescribe evidencia.
Si el SDK anterior ya incluye `linux_network_addresses.hpp`, también verifica
su consumidor; si no, indica que esa API queda fuera de la comparación. Un
consumidor que no puede consultar direcciones devuelve77 y no cuenta como pase.

## Reproducción

Desde la raíz del proyecto, después de instalar el SDK Release en un prefijo
local, configurar el verificador externo. Esto no impone CMake al GUI.

```sh
cmake -S outputs/pcinsight/tests/linux_abi -B work/build-linux-abi-168 \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PWD/work/sdk-headers-167-linux" \
  -DPCINSIGHT_ABI_BASELINE="$PWD/outputs/pcinsight/tests/linux_abi/baseline-linux-x86_64-gcc13.txt"
cmake --build work/build-linux-abi-168
ctest --test-dir work/build-linux-abi-168 --output-on-failure
```

`linux-abi-probe --dump` imprime el perfil y los valores para revisión; no
reescribe la línea base. No se ejecutan servicios, acciones ni captura. A05
queda en curso; esto no es una declaración de ABI 1.0 ni CI remota.

Verificación: SDK instalado 0.2.0, GCC 13.3.0, libstdc++ 20240904 y 15
comprobaciones de firma/tipo compiladas. La [línea base registrada](../tests/linux_abi/baseline-linux-x86_64-gcc13.txt)
contiene 28 filas. Pasaron las tres pruebas repetidas diez veces y la suite
Linux 200/200. Logs `work/linux-abi-*`. Oficial 60/75=80%.
