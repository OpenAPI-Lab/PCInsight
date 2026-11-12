# Usar PCInsight desde C++ en Linux

PCInsight es una biblioteca C++20, no una aplicación que debas ejecutar para
consultar métricas. Incluyes sus encabezados y enlazas `libpcinsight.a` con tu
programa. No necesitas terminar el GUI ni adoptar CMake en tu aplicación.

## 1. Construir el SDK desde el repositorio independiente

Necesitas un compilador C++20, CMake 3.20 o posterior y las herramientas de
compilación de tu entorno. Se han probado GCC 13.3 y Clang 18.1.3 con libstdc++
en Ubuntu WSL x86-64; no es una garantía para toda distribución o arquitectura.
Desde la raíz del repositorio PCInsight:

```sh
cmake -S . -B build/sdk-build -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DPCINSIGHT_BUILD_TESTS=OFF -DPCINSIGHT_BUILD_EXAMPLES=OFF
cmake --build build/sdk-build --parallel 2
cmake --install build/sdk-build --prefix "$PWD/build/sdk"
```

No uses `sudo`: el SDK queda en `build/sdk`, no instalado en el sistema. Para
Clang, añade `-DCMAKE_CXX_COMPILER=clang++` a la primera orden y usa una carpeta
de build nueva. Compila y enlaza tu aplicación con herramientas compatibles.

Si recibiste un paquete binario, comprueba primero su `SHA256SUMS`, extráelo y
usa el directorio `sdk` resultante en vez de estos pasos. El hash no autentica
al publicador. El binario debe ser compatible con tu arquitectura, runtime y
biblioteca estándar; en caso de duda, construye desde el código fuente.

## 2. Compilar un programa sin CMake

El [ejemplo completo](../examples/getting_started.cpp) tiene un `main`, obtiene
CPU, RAM y número de filas de procesos visibles, y termina tras tres muestras.
Desde la raíz de PCInsight:

```sh
SDK="$PWD/build/sdk"
g++ -std=c++20 -O2 -I"$SDK/include" examples/getting_started.cpp \
  "$SDK/lib/libpcinsight.a" -pthread -ldl -o build/mi_monitor
./build/mi_monitor
```

Si CMake instaló la biblioteca en `lib64` u otro libdir, ajusta esa ruta. Puedes
copiar el ejemplo a tu proyecto y usar una ruta absoluta al SDK. Para Clang,
usa `clang++` y el SDK de ese perfil. Si tu aplicación usa varios archivos,
compílalos normalmente y coloca el archivo estático después de sus objetos
en la orden de enlace. No mezcles encabezados y bibliotecas de revisiones
distintas, aunque ambas indiquen 0.2.0.

La parte esencial es:

```cpp
pcinsight::Monitor monitor;
pcinsight::SampleOptions options;
monitor.sample(options); // establece una línea base
std::this_thread::sleep_for(std::chrono::seconds(1));
auto snapshot = monitor.sample(options);
if (pcinsight::metric_is_known(snapshot.cpu.utilization_percent)) {
    double cpu = *snapshot.cpu.utilization_percent.value; // 25 = 25%, no 0.25
}
```

Los includes y `main` están en el ejemplo completo. Un dato ausente no es cero.
Consulta `support` y `note`; `estimated` no es una medición exacta. La primera
muestra normalmente no tiene tasas. No restes marcas de reloj de pared para
calcularlas: el proveedor usa su intervalo monotónico.

## 3. Añadir paneles y acciones

- Para discos, red y GPU, activa `include_disks`, `include_networks` e `include_gpus`; el ejemplo los desactiva para mantener pequeña la primera consulta.
- Servicios, usuarios, startup e historial se solicitan con `include_services`, `include_user_sessions`, `include_startup_entries` e `include_app_history`. Leer no autoriza acciones ni inicia captura privilegiada.
- Para consultar IP con disponibilidad, incluye `pcinsight/linux_network_addresses.hpp` y llama a `query_linux_network_addresses()`. Es una consulta separada de la muestra de Monitor, no una unión atómica.
- Para gráficas, incluye `pcinsight/sampler.hpp` y usa `Sampler::start`, `latest_timed`, `timed_history` y `stop`. Las ventanas son de tiempo, no de exactamente 60 filas. No bloquees el hilo gráfico con `sleep_for` o recolecciones síncronas: el bucle del ejemplo es para consola.
- Las acciones están separadas en `pcinsight/actions.hpp`; no actives start/stop, terminar procesos ni mutaciones solo por mostrar un panel. Revisa [alcance y limitaciones](SUPPORT_STATUS.md) antes de conectarlas a botones.

## 4. Si no funciona

| Síntoma | Qué comprobar |
| --- | --- |
| `pcinsight/monitor.hpp` no encontrado | `-I` debe apuntar a `sdk/include`, no a `sdk/include/pcinsight`. |
| `sdk_headers.hpp` no encontrado | Usa el SDK construido/instalado: ese encabezado se genera; no basta con copiar `include/` del fuente. |
| `undefined reference` | Enlaza la biblioteca estática correcta después del fuente/objetos y conserva `-pthread -ldl` para el perfil probado. |
| Fallo en `pcinsight_require_headers_...` | Encabezados y biblioteca no coinciden. Reconstruye o usa el SDK completo de una sola revisión. |
| CPU/tasas desconocidas al principio | Toma una segunda muestra tras un intervalo. Un contador reiniciado o una pérdida de continuidad también exige nueva base. |
| Una métrica sigue desconocida | Lee `support`/`note`. Puede faltar una fuente, un driver o un permiso; no ejecutes automáticamente como root. |

## Integración opcional con CMake

Solo si tu aplicación ya lo usa:

```cmake
find_package(pcinsight 0.2.0 EXACT CONFIG REQUIRED)
add_executable(mi_monitor main.cpp)
target_link_libraries(mi_monitor PRIVATE pcinsight::pcinsight)
```

Configura con `-DCMAKE_PREFIX_PATH=/ruta/al/sdk`. Esto es opcional para el
consumidor. [Política de versiones](SDK_VERSION_POLICY.md): el SDK 0.x aún no
promete una ABI 1.0 estable.
