# N03: auditoría de entrega del SDK Linux

Revisión 166, 2026-11-06. Se verifica una entrega local del SDK 0.2.0 y
se indexa el alcance Linux de R01–R20. No es liberación 1.0, CI remota,
certificación de hardware ni cierre de N03. `SEGUIMIENTO.txt` sigue siendo
el control oficial.

## Alcance verificable por requisito

| ID | Contrato de consumo y evidencia | Qué no se certifica |
| --- | --- | --- |
| R01 | Biblioteca C++20 y SDK instalado: biblioteca estática, headers públicos y configuración de enlace; prueba reproducible abajo. | ABI 1.0 ni compatibilidad universal. Sin GUI, recorder, servicio, autostart ni helper instalado por el SDK. |
| R02 | [Performance](LINUX_PERFORMANCE_REQUIREMENTS.md): muestra dinámica y `Sampler::timed_history`; [ventana](../tests/sampler_window.cpp). | Exactamente 61 puntos o relleno de intervalos ausentes. |
| R03 | [CPU y conteos](LINUX_PERFORMANCE_REQUIREMENTS.md): utilización, identidad, procesos, threads, FDs y uptime. | Handles Windows o inventario físico exacto. |
| R04 | [Frecuencias, topología, caché y KVM](LINUX_PERFORMANCE_REQUIREMENTS.md). | Virtualización de firmware B08; KVM no reemplaza esa prueba. |
| R05 | [Memoria y composición](LINUX_PERFORMANCE_REQUIREMENTS.md). | Paged/nonpaged pool Windows ni compresión global a partir de zswap. |
| R06 | [SMBIOS y memoria reservada](LINUX_PERFORMANCE_REQUIREMENTS.md). | Datos que firmware no publica ni reserva hardware exacta por resta. |
| R07 | [Tasas de disco](LINUX_DISK_RATES.md) y muestra Performance. | Latencia de aplicación o saturación física. |
| R08 | [Disco, identidad, capacidad y swap](LINUX_PERFORMANCE_REQUIREMENTS.md). | Toda capacidad formateada a partir de montajes; medio a partir de rotación. |
| R09 | [Identidad, tasas e IP de adaptador](LINUX_PERFORMANCE_REQUIREMENTS.md). | Completitud ni fallo individual en listas IP legacy; cobertura física N02. |
| R10 | [Motores GPU y agregados de clientes](LINUX_PERFORMANCE_REQUIREMENTS.md). | Uso global/3D a partir de sumar clientes; prueba física G07. |
| R11 | [Memoria, uso y temperatura GPU](LINUX_GPU_TELEMETRY.md). | Shared Windows por GTT, memoria asignable global o cobertura AMD/Intel/NVIDIA física. |
| R12 | [Versiones API y ubicación GPU](LINUX_PERFORMANCE_REQUIREMENTS.md). | Driver date fiable sin fuente, DirectX Linux ni contexto OpenGL inventado. |
| R13 | [Processes y Details](LINUX_PROCESS_REQUIREMENTS.md), ejemplo y tests por campo. | Red total por PID, identidad global, snapshot atómico ni RSS privado. |
| R14 | [App history](LINUX_APP_HISTORY_REQUIREMENTS.md), fuentes separadas. | Unión automática CPU/red/notificaciones, medición retroactiva o avisos mostrados. |
| R15 | [Startup](LINUX_STARTUP_REQUIREMENTS.md), estado, elegibilidad, Properties y acciones. | Causa automática I07, editor firmado o impacto total del login. |
| R16 | [Usuarios y sesiones](LINUX_USERS_REQUIREMENTS.md), scopes separados. | Atribución de todos los recursos por sesión, total global o unión de UIDs sin dominio. |
| R17 | [Servicios](LINUX_SERVICES_REQUIREMENTS.md), inventarios por proveedor. | Estado vivo por archivos/marcas ni aceptación real K03 en todos los init. |
| R18 | [Run new task](LINUX_LAUNCH.md) es una sola API explícita para todos los paneles. | Shell implícito, readiness o elevación implícita. |
| R19 | [End task y efficiency](LINUX_PROCESS_REQUIREMENTS.md); [control y detalles por proveedor](LINUX_SERVICES_REQUIREMENTS.md). | Efficiency checked en Linux, SysV control sin permiso o helper privilegiado L05. |
| R20 | [Acciones de usuario](LINUX_USERS_REQUIREMENTS.md): integración host, Disconnect y logout separados. | Gestor nativo universal o Disconnect convertido en cierre de sesión. |

Los tests de contrato usan fixtures y no sustituyen a un resultado físico.
Las muestras de entrega leen Linux; no activan red observada, notificaciones,
recorder, lanzamiento, control de servicio ni mutaciones de startup/sesiones.
Una muestra correcta puede informar ausencia o permiso denegado.

La prueba `linux-collection-noexec` usa [seccomp](https://docs.kernel.org/userspace-api/seccomp_filter.html)
solo en sus propios procesos. Primero comprueba que un intento de `execve` se
rechace; luego crea un Monitor y recoge dos muestras con lectura de servicios,
startup, usuarios e historial activada. No activa capturas ni acciones. Un
Sampler creado después del filtro produce al menos dos muestras y se detiene;
los hilos nuevos conservan la restricción de ejecución. Un
intento de ejecutar un programa termina el proceso y falla la prueba. Si el
entorno no permite el filtro, CTest lo registra como omitido, no como positivo.
Esto cubre las rutas que se ejercitan en ese entorno, no todas las extensiones
opt-in ni todos los controladores existentes.

La revisión 225 añade un aviso por pipe desde el manejador de `SIGSYS` para
detectar también un intento en un hijo que conserve ese manejador y descriptor,
aunque el padre ignore su salida. El hijo de calibración verifica ese aviso.
No es una auditoría universal de syscalls: un descendiente que restablezca el
manejador o cierre el canal queda fuera de ese aviso, aunque el filtro heredado
sigue impidiendo ejecutar el programa.

## Consumidor desde un prefijo dedicado

El [verificador externo](../tests/linux_delivery/CMakeLists.txt) comprueba que
la configuración, la biblioteca Release y los includes exportados resuelvan
dentro del prefijo. Exige 0.2.0 exacto, rechaza 0.2.1 exacto y audita el contenido
instalado. Compila los seis ejemplos del repositorio **contra el SDK instalado**
y corre un fixture y una muestra por panel. Los ejemplos no se instalan como
dependencias del usuario. El verificador es para pruebas; no impone CMake al GUI.

```sh
# Desde la raíz del proyecto, con un build Release de Linux ya compilado.
stage=$(mktemp -d work/sdk-audit.XXXXXX)
cmake --install work/build-linux --config Release --prefix "$PWD/$stage"
cmake -S outputs/pcinsight/tests/linux_delivery -B work/build-linux-delivery-166 \
  -DCMAKE_BUILD_TYPE=Release -DPCINSIGHT_AUDIT_PREFIX="$PWD/$stage"
cmake --build work/build-linux-delivery-166 -j4
ctest --test-dir work/build-linux-delivery-166 --output-on-failure
```

Usar un directorio de build del verificador nuevo si se cambia de prefijo;
no reutilizar una caché `pcinsight_DIR` que apunte a otra instalación.

Para una aplicación sin CMake, el equivalente para esta configuración Linux
es pasar el include y el archivo estático al compilador. Ejemplo con el
layout `lib` verificado aquí (ajustar si se instaló en otro libdir):

```sh
g++ -std=c++20 -I"$stage/include" outputs/pcinsight/examples/linux_services.cpp \
  "$stage/lib/libpcinsight.a" -pthread -ldl -o work/linux-delivery-direct
work/linux-delivery-direct --self-test
```

Usar headers y archivo estático de la misma compilación. La aceptación de la
versión exacta en CMake no detecta diferencias de layout entre revisiones 0.x;
recompilar todos los consumidores al cambiar el SDK. A05 sigue pendiente.

## Límites de cierre

- B08, G07 y N02 requieren evidencia de firmware, GPU y distribuciones/hardware
  reales; WSL y fixtures no completan esos criterios.
- I07 conserva el alcance observador y causa no verificable; no se amplía a
  iniciador propio. K03 requiere aceptación real; SysV sigue sin permiso de
  ejecución. L05 sigue aplazado para despliegue y aceptación privilegiada.
- N01 requiere ejecuciones reales de CI. Windows sigue aplazado, incluido
  el riesgo histórico de Monitor MSVC. Tampoco se reclaman corregidos los fallos
  históricos de integración Linux por una regresión local positiva.
- La parte local de empaquetado y consumo Linux de N03 se comprueba aquí; la
  auditoría final del proyecto y las verificaciones pendientes siguen abiertas.

## Evidencia de esta revisión

22 enlaces locales verificados. Prefijo `work/sdk-audit.tff64S`: 48 headers,
una biblioteca estática y cuatro archivos CMake, sin ejecutables. Exacto
0.2.0 aceptado y 0.2.1 rechazado. Seis consumidores, 12/12 pruebas del SDK
instalado, enlace directo con g++ y self-test exit0, suite Linux 200/200.
Logs `work/linux-delivery-*-verified.log`. El intento previo con variable de
shell perdió el sufijo de prefijo al cruzar PowerShell/WSL; su registro indica
solo archivos ya actualizados en el workspace. Se repitió con ruta literal y
se añadió comprobación temprana del prefijo. N03 queda abierto; 60/75=80%.
