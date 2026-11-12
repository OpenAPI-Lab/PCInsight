# PCInsight

**Idioma:** Español · [English](README.en.md)

Biblioteca/SDK C++20 independiente para consultar información del sistema,
con prioridad en Linux. Este repositorio **no contiene el GUI ni el proyecto
principal que consume la API**. Puedes modificar, probar y proponer mejoras
de PCInsight sin trabajar en esos otros proyectos.

**Estado: 0.2.0 en desarrollo, utilizable para pruebas e integración; no está
completo ni certificado para cualquier Linux.** Las métricas dependen de las
fuentes y permisos del entorno. Un dato ausente no se sustituye por cero.
No hay promesa de ABI estable 1.0. La validación disponible es local; los
workflows preparados no equivalen a ejecuciones remotas ya comprobadas.

## Empieza aquí

- [Cómo usarla desde C++ y compilar sin CMake en tu aplicación](docs/GETTING_STARTED_CPP.md).
- [Qué funciona, qué es parcial y qué falta implementar](docs/SUPPORT_STATUS.md).
- [Cómo contribuir y ejecutar pruebas](CONTRIBUTING.md).
- [Cómo mantener y publicar este repositorio separado del GUI](docs/INDEPENDENT_REPOSITORY.md).
- [Cómo validar tu hardware y reportar resultados](docs/LINUX_HARDWARE_EVIDENCE.md).
- [Cómo reportar problemas de seguridad sin exponer datos](SECURITY.md).

La [licencia MIT](LICENSE) corresponde a PCInsight y su SDK, documentación y
ejemplos; no licencia el GUI ni otros proyectos separados que lo utilicen.
No se necesita ejecutar programas como `dmidecode` o `systemctl` para recopilar
datos. Las acciones explícitas y sus requisitos son una superficie distinta;
consulta la matriz de soporte antes de conectarlas a tu GUI.

## Referencias detalladas

El SDK incluye una [comprobación opcional de headers y biblioteca al enlazar](docs/SDK_HEADER_COMPATIBILITY.md).
No es una garantía de ABI 1.0 ni exige CMake en el GUI.

La [auditoría de entrega Linux](docs/LINUX_DELIVERY_AUDIT.md) enlaza los requisitos
R01–R20 con las guías de consumo, pruebas y límites del SDK.

Para Servicios Linux, consulta la [guía de proveedores, campos y acciones](docs/LINUX_SERVICES_REQUIREMENTS.md)
y el [ejemplo C++ sin control de servicios](examples/linux_services.cpp).

Para Usuarios y sesiones Linux, consulta la [guía de alcance y acciones](docs/LINUX_USERS_REQUIREMENTS.md)
y el [ejemplo C++ de solo lectura](examples/linux_users.cpp).

Para Startup Apps Linux, consulta la [guía de campos y acciones](docs/LINUX_STARTUP_REQUIREMENTS.md)
y el [ejemplo C++ de solo lectura](examples/linux_startup.cpp).

Para App history Linux, consulta la [guía de CPU, red y notificaciones](docs/LINUX_APP_HISTORY_REQUIREMENTS.md)
y el [ejemplo C++](examples/linux_app_history.cpp), que no activa captura de red
ni notificaciones y no escribe archivos de historial.
 
Para Processes/Details Linux, consulta el
[contrato de campos, identidad y acciones](docs/LINUX_PROCESS_REQUIREMENTS.md)
y el [ejemplo C++ sin acciones ni captura](examples/linux_processes.cpp).
Su `--sample` recopila inventario visible pero muestra solo SELF.
 
Para integrar el bloque Performance Linux en tu GUI, consulta la
[matriz de campos, unidades y límites](docs/LINUX_PERFORMANCE_REQUIREMENTS.md)
y el [ejemplo C++ de solo lectura](examples/linux_performance.cpp).
Usa solo el SDK público: diferencia desconocido de cero y no exige crear el
CMake de tu GUI ahora. `--self-test` no consulta el SO; `--sample` toma dos
observaciones Linux sin acciones ni captura eBPF. Sin argumentos solo ayuda.

El [grabador opcional de startup](docs/STARTUP_RECORDER.md) ofrece
`LinuxStartupRecorder` para un host anterior al GUI, con consentimiento,
horarios/cierre explicitos y guardado historico solicitado. El ejecutable propio
es opt-in de compilacion, sin reglas de instalacion/activacion. No certifica
autostart por marcadores ni captura universal de apps breves; I07 sigue abierta.

La [red por usuario Linux](docs/LINUX_USER_NETWORK.md) se consulta en
`snapshot.linux_user_network`: una fila por UID del observador, con dominio,
cobertura y tasas parciales de envio/recepcion/total. No duplica sesiones ni
asocia usuarios por nombre. `include_linux_user_network` evita exigir inventario
de sesiones; la captura F03 sigue requiriendo autorizacion previa explicita.

El [historial de red observado por ejecutable Linux](docs/LINUX_NETWORK_APP_HISTORY.md)
ya integra F03 en `Monitor`/`Sampler` mediante `snapshot.linux_network_app_history`.
Requiere captura observada autorizada y ambas opciones de historial/red; publica
bytes acumulados estimados, con identidad/ventanas y gaps explicitos. CPU y red
mantienen periodos/archivos independientes; [LinuxHistoryStore](docs/LINUX_HISTORY_STORE.md)
persiste disponibilidad sin duplicar snapshots. Recompilar TODOS por layout0.x.

La [API publica de red por procesos Linux seleccionados](docs/LINUX_PROCESS_NETWORK.md)
se usa con `#include <pcinsight/linux_process_network.hpp>` y
`pcinsight::LinuxProcessNetworkSampler`. Es opt-in con pidfds, identidades y
cookies declarados o [cookies observados opt-in](docs/LINUX_PROCESS_NETWORK_OBSERVED.md), ventanas nominales/tasas estimadas y sumas UID parciales;
no activa BPF automaticamente desde Monitor ni informa trafico total del equipo/proceso/usuario.
La [integracion Monitor/Sampler](docs/LINUX_PROCESS_NETWORK_MONITOR.md) exige
setup autorizado separado de include_linux_process_network; desactivar o detener
el worker cierra captura, reiniciar no reautoriza. Cambio de layouts0.x:
recompilar biblioteca y todos los consumidores.
La [actualizacion de seleccion](docs/LINUX_PROCESS_NETWORK_UPDATE.md) permite
anadir/retirar procesos en el proveedor y Monitor con consentimiento nuevo,
conservando capturas retenidas y reiniciando todas las tasas; no auto-descubrimiento.
Tambien puede [actualizarse con el Sampler de fondo activo](docs/LINUX_PROCESS_NETWORK_WORKER.md),
serializado con coleccion/parada; historial conserva epochs, no mezclar sus tasas.
El [modo de buckets declarados por namespace](docs/LINUX_PROCESS_NETWORK_BUCKETS.md)
admite varias cookies por proceso sin duplicar trafico. Puede
[seleccionarse explicitamente en Monitor/Sampler](docs/LINUX_PROCESS_NETWORK_BUCKET_MONITOR.md)
al iniciar la sesion; las llamadas anteriores siguen estrictas. No descubre namespaces.
El [motor interno de cookies observados](docs/LINUX_NETWORK_DISCOVERY.md) ya cuenta
buckets sin recibir cookies manuales. Las [ventanas internas seleccionadas](docs/LINUX_NETWORK_DISCOVERED_WINDOWS.md)
ya verifican identidad pidfd/procfs y calculan tasas por cookie observado.
La [coleccion interna multiactor](docs/LINUX_NETWORK_OBSERVED_MULTI.md) agrega
ventana nominal comun, registros vacios y reset grupal; ya esta conectada al
proveedor publico y a [Monitor/Sampler en modo observado](docs/LINUX_PROCESS_NETWORK_OBSERVED_MONITOR.md),
aun no al descubrimiento general de procesos. Nuevo layout0.x de revision83:
recompilar TODOS si se viene de headers anteriores.
La [gestion de selecciones por PID/inicio](docs/LINUX_NETWORK_SELECTION_OWNER.md)
prepara/retiene/retira pidfds sin apertura manual y rechaza exceso de capacidad;
no enumera procesos ni inicia captura automaticamente (limite estructural1024actores).
La [consulta pidfd reutilizable con compuerta](docs/LINUX_NETWORK_QUERY_SESSION.md)
ya supera la prueba nativa17, con tasas/UID y limpieza verificadas. La
[ampliacion a selecciones mayores](docs/LINUX_NETWORK_SCALE.md) conserva lotes
RAW de hasta64 y hasta16 cookies por proceso, sin truncamiento. No implica
captura automatica de la sesion. Los [fallos de coste previos](docs/LINUX_NETWORK_CAPACITY64.md)
se conservan como evidencia historica, no como resultado vigente.
El [backend compartido publico](docs/LINUX_NETWORK_SHARED_PUBLIC.md) ya atiende
el modo de namespaces observados de LinuxProcessNetworkSampler, Monitor y
Sampler; la seleccion publica ya probo65actores privados en dos lotes, pidfds propios/joins por extremos,
cuatro tasas y UID parciales. [Updates autenticados](docs/LINUX_NETWORK_SHARED_UPDATES.md)
conservan retenidos sin reinicio y admiten/reincorporan con captura fresca,
sin contar el gap; cada exito renueva epoch/reset tasas. Usa lotes disjuntos
acotados, no tres sondas constantes tras todo update. No captura general ni
admision automatica. Estricto predeterminado y cookies declaradas no cambian.
El [inventario visible de procesos](docs/LINUX_NETWORK_INVENTORY.md) ya conserva
identidades verificadas y motivos por fila; su puente estricto rechaza desconocidos
o exceso de capacidad, sin reducir silenciosamente la lista ni iniciar captura.
La [admision estricta standalone](docs/LINUX_NETWORK_ADMISSION.md) ya conecta
ese inventario al proveedor compartido con refresh explicitamente autorizado:
conjunto igual conserva tasas/epoch, cambios retienen origen y readmisiones
empiezan frescas. Rechaza desconocidos o mas de1024, nunca captura un subconjunto
oculto. No se probo captura de tu sesion completa; solo procesos privados.
Una [politica parcial opt-in](docs/LINUX_NETWORK_ADMISSION_POLICY.md) permite
excluir filas no verificadas con PID/estado/motivo junto al snapshot; el modo
predeterminado sigue estricto y mas de1024elegibles rechaza TODO. El reporte de
admision0.x cambio de layout: recompilar biblioteca y consumidores.
La [admision en Monitor/Sampler](docs/LINUX_NETWORK_ADMISSION_MONITOR.md) ya
publica red y metadata/exclusiones juntas en cada SystemSnapshot e historial
del worker, con refrescos explicitamente autorizados y sin rearmado por flags.
SystemSnapshot0.x tiene un campo nuevo: recompilar TODOS los consumidores.
La [validacion de F03 en vista real](docs/LINUX_NETWORK_ACCEPTANCE.md) ya pasa
en WSL con30/30procesos en modo estricto; corrige el caso SELF del recolector.
La fuente opcional queda implementada con cobertura/limites declarados, sin
implicar red total universal ni historial H04/agregacion de sesiones J04 terminados.

Los [contadores de E/S general por proceso Windows](docs/WINDOWS_PROCESS_IO.md)
incluyen tasas y contadores de64 bits, separados de disco/red especificos.
El nuevo campo ProcessInfo0.x requiere reconstruir biblioteca y consumidores.

La [consulta experimental AMD Windows](docs/WINDOWS_AMD_RUNTIME_MEMORY.md) es
una llamada explicita separada de Monitor/Sampler. Expone estimaciones del
runtime OpenCL, no memoria global libre; puede preparar recursos GPU internos.

La [memoria framebuffer libre NVIDIA en Windows](docs/WINDOWS_NVML_MEMORY.md)
usa opcionalmente bibliotecas del controlador e identidades CUDA/NVML/KMT, sin
SDK obligatorio ni comandos externos. Falta positivo fisico NVIDIA; no promete
memoria allocatable ni cubre AMD/Intel con esa fuente.

La [consulta OpenGL Windows del contexto actual](docs/WINDOWS_CURRENT_OPENGL.md)
es opt-in por hilo y usa LUID para atribuirlo a un adaptador DXGI. No crea un
contexto ni informa un maximo hardware; sin identidad probada deja unknown.

La [capacidad de desconexion Linux](docs/LINUX_DISCONNECT.md) se consulta por
proveedor sin efectos: utmp/login1 no ofrecen detach que preserve aplicaciones.
El GUI debe mostrarla como no disponible; cerrar sesion es otra accion y puede
terminar procesos. No se afirma imposibilidad para servidores remotos externos.

Windows ofrece una accion explicita para retirar/restaurar archivos Startup del
usuario mediante rename comprobado, independiente de KTM. No cambia la aprobacion
de Windows ni se conecta automaticamente a botones genericos. Ver
[contrato y limites](docs/WINDOWS_STARTUP_FILE_CONTROL.md).
El [informe de respaldos en Monitor/Sampler](docs/WINDOWS_STARTUP_FILE_MONITOR.md)
es opt-in e independiente de acciones/aprobacion. Esta adicion0.x cambia los layouts
de SystemSnapshot/SampleOptions: recompilar biblioteca y consumidores juntos.
El [alcance Startup comun](docs/WINDOWS_COMMON_STARTUP_FILE_CONTROL.md) se consulta
separadamente; sus acciones pueden afectar a todos los usuarios y nunca elevan
permisos automaticamente. No equivale a HKLM/StartupApproved.

La integración explícita de «Manage user accounts» para el GUI consumidor está
documentada en [ACCOUNT_MANAGEMENT.md](docs/ACCOUNT_MANAGEMENT.md). No incluye
un gestor de cuentas ni abre ajustes automáticamente.

El editor declarado de startup Linux se puede consultar sin comandos externos,
con AppStream opt-in y la biblioteca XML del sistema si ya existe. No instala
dependencias ni verifica firmantes. Ver [LINUX_STARTUP_PUBLISHER.md](docs/LINUX_STARTUP_PUBLISHER.md).

La captura opcional de notificaciones Linux requiere consentimiento explicito:
cuenta solicitudes aceptadas por el servicio, no avisos mostrados ni historial
retroactivo, y no conserva titulos/contenido. Ver [LINUX_NOTIFICATIONS.md](docs/LINUX_NOTIFICATIONS.md).

Para regiones ext2/3/4 seleccionadas, la consulta read-only de
[capacidad declarada](docs/LINUX_FILESYSTEM_CAPACITY.md) funciona sin montarlas,
sin comandos externos y con opt-in para dispositivos de bloque. No equivale
al espacio util/libre ni a un agregado fisico universal.

PCInsight es una biblioteca C++20 para construir monitores de sistema y administradores de tareas. Su API separa:

1. **Inventario estático**: modelo, fabricante, capacidad, cachés y topología.
2. **Snapshots dinámicos**: utilización, velocidades y contadores. Las tasas requieren dos muestras.
3. **Historial**: Sampler conserva una ventana monotónica de60s; el GUI dibuja las muestras. El historial acumulado por aplicación es un informe separado con disponibilidad y límites.
4. **Acciones privilegiadas**: finalizar procesos, controlar servicios o sesiones, en una interfaz distinta de la telemetría.

La versión 0.2 en desarrollo incluye un backend Linux para CPU, memoria, procesos, adaptadores, discos, sesiones, startup, servicios, historial observado y telemetría GPU publicada por DRM/hwmon. También incluye un lector binario propio de SMBIOS para RAM y frecuencia de firmware. No ejecuta ni requiere `dmidecode`, `lspci`, `systemctl`, `nvidia-smi` u otra aplicación externa para recopilar datos. El backend Windows es parcial y secundario en esta entrega; consulta [sus límites](docs/WINDOWS.md). En los demás sistemas devuelve un snapshot marcado como no implementado.

## Compilar

Para integrar y verificar el SDK en Linux, empieza por la
[guía de entrega Linux 0.2](docs/LINUX_02_HANDOFF.md).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/pcinsight-cli
```

## Uso

```cpp
#include <pcinsight/monitor.hpp>
#include <chrono>
#include <thread>

pcinsight::Monitor monitor;
monitor.sample();                         // establece contadores base
std::this_thread::sleep_for(std::chrono::seconds(1));
auto snapshot = monitor.sample();         // porcentajes y bytes/s válidos
```

Para muestreo continuo y gráficas, `pcinsight::Sampler` usa un intervalo de1s por defecto y conserva los puntos iniciados en los últimos60s. `timed_history()` expone timestamps monotónicos de inicio/fin para el eje temporal. La capacidad automática (`0`) admite hasta61 puntos a1s o601 a100ms; una capacidad explícita puede recortar esa ventana. Intervalo y grupos recopilados son configurables. Ver [contrato y límites](docs/SAMPLER_WINDOW.md).

La API con disponibilidad usa `Metric<T>` (`value`, `support`, `note`). Comprueba `metric_is_known(m)`: un cero medido es válido; ausencia no es cero. Para historial usa `app_history_view(snapshot)` y `AppHistorySample` de `pcinsight/app_history.hpp`. `AppHistoryEntry` antiguo es payload legacy sin disponibilidad, también llamado `LegacyAppHistoryEntry`; no promover sus números a mediciones conocidas. Ver [contrato, unidades y compatibilidad](docs/METRIC_AVAILABILITY.md).

## Decisiones importantes

- La gráfica0–60s la dibuja el GUI usando los timestamps del Sampler, no contando60 elementos. Las ausencias son huecos, no ceros; no se garantiza cadencia exacta ante recolecciones lentas.
- El porcentaje de CPU y todas las tasas se calculan con deltas entre dos muestras; una lectura instantánea aislada no puede producirlos correctamente.
- “Logical processors” equivale a hilos de hardware, no a los threads de software. Ambos se exponen por separado.
- La frecuencia base no se inventa a partir de la frecuencia actual. Linux solo la informa si firmware/kernel expone `base_frequency` o si un backend SMBIOS puede leerla.
- “Handles”, pools paged/non-paged, memoria comprimida y DirectX son conceptos de Windows. En Linux se usan descriptores, métricas de kernel/zswap y Vulkan/OpenGL/OpenCL.
- Tráfico por proceso no existe de forma fiable en `/proc`. Requiere eBPF/cgroups, nftables u otro componente con privilegios.
- “App history” no es un dato universal del SO. PCInsight observa CPU/red/notificaciones desde que se habilita cada proveedor; instalar la biblioteca no inicia una captura ni reconstruye el pasado.
- “Compatible con cualquier distro” significa depender de interfaces del kernel, no de su gestor de paquetes ni de comandos instalados. Algunas funciones siguen dependiendo de que el kernel tenga el driver correspondiente y monte procfs/sysfs.
- SMBIOS es inventario de firmware, no una fuente de telemetría viva. Puede estar ausente en ARM, máquinas virtuales y equipos cuyo firmware no lo publique; `/sys/firmware/dmi/tables/DMI` también puede requerir root.

## Cobertura Linux actual

La cobertura de abajo describe qué implementa PCInsight, no qué se ha verificado
en todos los equipos. Parte de la validación se hizo en WSL o con datos simulados.
Si tienes Linux nativo, puedes ayudar a comprobar CPU, RAM/firmware, discos, red
o GPU: sigue la [guía para verificar hardware y reportar resultados en GitHub](docs/LINUX_HARDWARE_EVIDENCE.md).

- CPU: modelo, fabricante, utilización, frecuencia actual/base cuando se expone, topología, cachés, uptime, procesos, threads y descriptores visibles.
- Memoria: total, uso, disponible, commit, caché, zswap y datos físicos SMBIOS.
- Procesos: PID/PPID, usuario, estado, CPU, RSS, E/S, threads, ejecutable y arquitectura ELF.
- Discos: tipo/bus, capacidad física y montada, disco raíz, swap, actividad, lectura, escritura y latencia media.
- Red: interfaces, direcciones, bytes totales y throughput por adaptador.
- GPU: PCI/DRM, ubicación, driver, uso/VRAM/temperatura cuando el driver crea esos atributos.
- Usuarios: sesiones logind/utmp y consumo visible agregado por usuario, con límites explícitos.
- Startup: entradas XDG globales y por usuario; habilitar/deshabilitar mediante escritura transaccional.
- Servicios: inventario systemd y fallback parcial por archivos; [consulta SysV explícita con ServiceInfo](docs/LINUX_SYSV_SERVICES.md) y descripciones LSB declaradas, sin inferir PID/vida/enablement. Start/stop/restart systemd mediante D-Bus cargando `libsystemd` si el propio sistema la incluye; esta consulta SysV no añade control de scripts.
- Acciones: ejecutar sin shell y terminar con identidad/pidfd; logout explícito logind. Preset CPU/I/O Linux solo como flujo avanzado unchecked con reconocimiento explícito del riesgo PID/TID y recibo; acción genérica desactivada, sin fallback automático (docs/LINUX_EFFICIENCY.md). Disconnect Linux falla cerrado, no equivale a logout.
- Historial: CPU acumulado por ejecutable desde que se crea `Monitor`; [LinuxHistoryStore](docs/LINUX_HISTORY_STORE.md) conserva las filas Linux con disponibilidad/notas mediante guardado y retencion explicitos. `HistoryStore` sigue siendo solo legacy; la red F03 aun no alimenta H04. Consulta [HISTORY.md](docs/HISTORY.md).

Consulta [docs/CAPABILITIES.md](docs/CAPABILITIES.md) para el alcance completo y la hoja de ruta.
Consulta [runit en ServiceInfo](docs/LINUX_RUNIT_SERVICES.md) para la vista explícita
de estados reportados (no prueba de vida/PID de host ni capacidad de control).
Consulta [s6 en ServiceInfo](docs/LINUX_S6_SERVICES.md) para la vista explícita
de records históricos y loggers, sin inferir supervisor/proceso vivo.
Consulta [OpenRC en ServiceInfo](docs/LINUX_OPENRC_SERVICES.md) para candidatos
y marcas en vistas separadas, sin asociar identidades por nombre ni ejecutar scripts.
Consulta [selección de servicios en Monitor](docs/LINUX_SERVICE_SELECTION.md) para
elegir proveedor/vista explícitos sin fallback silencioso. Recompilar clientes por
la ampliación de SampleOptions; el modo predeterminado mantiene la ruta anterior.
Consulta [auditoría de init alternativos](docs/LINUX_K03_AUDIT.md) para pendientes
reales de K03. [L05/Linux](docs/LINUX_PRIVILEGED_PREFLIGHT.md) está en validación
interna: no helper instalado, autenticación o elevación operativa.
El [coordinador interno L05](docs/LINUX_PRIVILEGED_AUTHORIZATION.md) añade pruebas
de binding/rechazo y ya dispone de adaptadores nativos internos probados con fixtures; aún no hay ejecutor.
La [pieza nativa de identidad](docs/LINUX_PRIVILEGED_IDENTITY.md) verifica mensaje/
conexión y consultas de identidad; sigue pendiente el despliegue seguro.
La [consulta nativa CheckAuthorization](docs/LINUX_PRIVILEGED_NATIVE_AUTHORIZATION.md)
está implementada con pruebas aisladas. El [receptor cerrado](docs/LINUX_PRIVILEGED_RECEIVER.md)
ya se prueba con libsystemd y broker privados reales, Authority simulada y sin ejecutor;
siguen pendientes configuración/despliegue seguros y aceptación polkit real.
La [política interna protegida](docs/LINUX_PRIVILEGED_POLICY.md) ya admite lectura
anclada a FD, recarga sin fallback y rechazo de generaciones invalidadas durante
la consulta. Aún requiere bootstrap seguro; no instala ni ejecuta un helper.
El [ancla absoluta y conector propio](docs/LINUX_PRIVILEGED_CONNECTION.md) ya
verifican rutas/peer y revocan la conexión ante cambios detectados; aún falta
endurecer el proceso de despliegue y validar polkit/ejecución reales.
El [flujo de ejecución interno](docs/LINUX_PRIVILEGED_EXECUTION.md), OFF por defecto,
ya une autorización fresca y una operación cerrada al gestor simulado en pruebas.
Un job encolado no equivale a servicio iniciado; un envío incierto no se reintenta.
El [seguimiento asíncrono de jobs](docs/LINUX_PRIVILEGED_JOBS.md) ya distingue
completado, no exitoso y desconocido, sin convertir timeout en cancelación.
El [presupuesto por solicitud](docs/LINUX_PRIVILEGED_BUDGET.md) comparte plazo y
cuota de RPC entre fases; no equivale a un deadline absoluto del proceso o transporte.
La [admisión interna](docs/LINUX_PRIVILEGED_ADMISSION.md) ya limita ráfagas por
receptor/remitente y rechaza presión de colas locales; no limita bytes antes de recibir.
La [prueba de frontera de transporte](docs/LINUX_PRIVILEGED_TRANSPORT.md) verifica
rechazo previo al handler en un broker privado limitado, no en el bus del host.
No instalar su perfil en el bus compartido; despliegue/arquitectura siguen abiertos.
Consulta [docs/STATUS.md](docs/STATUS.md) para distinguir lo implementado, lo parcial y lo pendiente.

El ejemplo opcional [STARTUP_CAPTURE.md](docs/STARTUP_CAPTURE.md) une candidatos,
muestreo y guardado histórico de startup sin instalar ni activar autostart.
No certifica origen automático ni sustituye la integración temprana del consumidor.

## Licencia

La biblioteca y API PCInsight, incluidos su SDK y la documentación y ejemplos
que lo acompañan, se ofrecen bajo la [licencia MIT](LICENSE). Esto no licencia
el GUI ni otras aplicaciones o proyectos independientes que usen el SDK.
Si redistribuyes la biblioteca o una parte sustancial de ella, conserva el
aviso de copyright y el texto de la licencia junto a esa distribución.
