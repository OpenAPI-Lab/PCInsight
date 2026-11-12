# Backend Windows: alcance verificado

Actualizado 2026-10-21 (validacion MSVC revision51). C++20, sin subprocess ni
instalaciones adicionales en runtime. Se enlazan las bibliotecas del sistema
`psapi`, `advapi32`, `powrprof`, `pdh` y `version`, entre otras bibliotecas del SO.
Probado con MinGW-w64 x64 y ahora MSVC19.51 x64:127/127 Release en ambos,
consumidores instalados separados exit0; Linux111/111. Ver
[MSVC_VALIDATION.md](MSVC_VALIDATION.md) para correcciones, logs y limites.
ARM64/x86, equipos multigrupo y otras versiones de Windows requieren validacion.
Los recuentos/afirmaciones MSVC en verificaciones historicas de documentos
anteriores describen su fecha original; la evidencia actual esta en ese informe.

## Disponible

- Inicio por registro Run HKCU/HKLM opt-in, con vistas WOW64 explícitas y comandos
  raw, sin ejecutarlos. Estado efectivo unknown e impacto not_measured; no es
  inventario completo. Ver [WINDOWS_STARTUP_REGISTRY.md](WINDOWS_STARTUP_REGISTRY.md).
  Monitor añade archivos de carpetas Startup del usuario/sistema, sin resolver
  accesos directos: [WINDOWS_STARTUP_FOLDERS.md](WINDOWS_STARTUP_FOLDERS.md).
  Lectura opt-in acotada de metadatos guardados .lnk (no resolución del shell):
  [WINDOWS_SHORTCUT.md](WINDOWS_SHORTCUT.md).
  El mismo opt-in lee el destino guardado .url sin abrirlo:
  [WINDOWS_INTERNET_SHORTCUT.md](WINDOWS_INTERNET_SHORTCUT.md).
  Opt-in independiente de etiqueta CompanyName estimada (NO firma verificada):
  [WINDOWS_STARTUP_PUBLISHER.md](WINDOWS_STARTUP_PUBLISHER.md).

- Sesiones de usuarios locales WTS cuando include_user_sessions=true: cuenta,
  dominio y estado de conexión. CPU/RAM visibles por sesión+cuenta se agregan
  como estimated, incluso con procesos ocultos. Ver [WINDOWS_USERS.md](WINDOWS_USERS.md)
  y [WINDOWS_USER_RESOURCES.md](WINDOWS_USER_RESOURCES.md). Usa wtsapi32 del sistema.

- Inventario de servicios Win32 por SCM cuando `include_services=true`: nombre,
  estado, PID válido, descripción y grupo de orden de carga. Solo lectura;
  ver [WINDOWS_SERVICES.md](WINDOWS_SERVICES.md).

- Procesos individuales: PID/PPID, nombre, threads, usuario, ruta, arquitectura,
  descripción VERSIONINFO, memoria residente y CPU entre muestras cuando hay acceso. Estado active/exited/
  unknown; no detecta todavía suspensión. Ver [WINDOWS_PROCESSES.md](WINDOWS_PROCESSES.md).

- Nombre/fabricante desde el registro de hardware (vacío si no se expone).
- Núcleos físicos, sockets, procesadores lógicos y sumas L1/L2/L3 desde
  `GetLogicalProcessorInformationEx` y `GetActiveProcessorCount`.
- Utilización como tiempo ocupado del scheduler entre dos muestras. La primera
  muestra no tiene porcentaje. Un intervalo sin avance tampoco produce cero
  artificial. No equivale al indicador de utilidad ponderado por frecuencia de
  algunas versiones del Administrador de tareas.
- En equipos multigrupo se utiliza PDH `Processor Information(_Total)` mediante
  `PdhAddEnglishCounterW`, sin depender del idioma de Windows. Se valida tanto
  el resultado de cada llamada como el estado del contador. Fallos, NaN y valores
  fuera de 0–100 no se convierten en cero; se descarta la consulta y se exigen
  dos muestras nuevas al recuperarse. Cambios en cantidad de CPUs/grupos también
  reinician la línea base. No se cambia la afinidad del proceso llamador.
- Uptime, indicador de virtualización de firmware, cantidad global de procesos,
  threads y handles. El indicador de virtualización no afirma que haya un
  hipervisor activo.
- RAM visible al SO, disponible, en uso, porcentaje, commit/límite y pools
  paged/nonpaged. Se convierte de páginas a bytes. RAM en uso se define como
  total menos disponible. El contenido comprimido tiene un proveedor opcional
  experimental, no habilitado por defecto; ver [WINDOWS_COMPRESSION.md](WINDOWS_COMPRESSION.md).
  Se añade MemoryInfo.reported_total_bytes con disponibilidad explícita y producto
  PSAPI del total comprobado; total_bytes legacy se conserva. API0.x: recompilar
  consumidores. Ver LINUX_MEMORY.md para contrato compartido del nuevo campo,
  sin afirmar que esta unidad revalida todos los otros productos PSAPI.
- Tipo de RAM (DDR/LPDDR), formato, velocidad configurada en MT/s y posiciones
  ocupadas/representadas por firmware mediante `GetSystemFirmwareTable(RSMB)`.
  No usa WMI, comandos externos ni archivos temporales. El inventario se lee
  una vez por `Monitor`; recrea el monitor después de cambios de hardware.
  Ver [SMBIOS.md](SMBIOS.md) para incertidumbres y validación del parser.

## Estimaciones expresamente etiquetadas

- `current_ghz`: media de `CurrentMhz` de Windows. Es una frecuencia reportada
  considerando throttling, no una medición instantánea del turbo.
- `base_ghz`: SMBIOS Current Speed al arrancar, solamente como estimación
  cuando está disponible. No se sustituye con Max Speed ni se afirma que sea
  la frecuencia base nominal exacta.
- `cached_bytes`: `SystemCache` de PSAPI (standby más working set del sistema),
  no una reproducción exacta del campo Cached del Administrador de tareas.
- `hardware_reserved_bytes`: RAM instalada según firmware menos RAM visible al
  SO. Puede incluir límites de arranque además de reservas de hardware.

## Pendiente, sin valores inventados

Frecuencia base nominal exacta y memoria comprimida estable/equivalente al
Administrador de tareas. Ya existe lectura experimental opt-in de contenido
comprimido mediante una interfaz interna, etiquetada estimada. La frecuencia dinámica
estimada aún está limitada a equipos de un grupo; la utilización multigrupo
ya tiene proveedor PDH, pendiente de validación en hardware multigrupo real.
Si el proveedor está deshabilitado o faltan permisos se devuelve ausencia
explícita, nunca el porcentaje parcial de un grupo como total.
Red ya dispone de proveedor IP Helper: interfaces, direcciones y tasas de
envío/recepción; ver WINDOWS_NETWORK.md. Discos dispone de inventario,
modelo/bus, capacidad, rendimiento PDH y mapa de volúmenes/system disk/page file;
ver WINDOWS_DISKS.md, WINDOWS_DISK_PERFORMANCE.md y WINDOWS_VOLUMES.md.
El medio reportado HDD/SSD/SCM dispone de proveedor WMI local; requiere los
permisos del proveedor y puede desactivarse con include_disk_media=false.
Ver WINDOWS_DISK_MEDIA.md; no implica instalar utilidades externas.
GPU dispone ahora de inventario DXGI no-software, modelo/IDs, capacidades
dedicated/shared/dedicated-system y versión UMD cuando se expone; ver
WINDOWS_GPU_INVENTORY.md. Memoria dinámica dedicada/compartida por adaptador PDH
implementada (WINDOWS_GPU_MEMORY.md); motores y máximo de utilización observada
estimados mediante PDH (WINDOWS_GPU_ENGINES.md). Temperatura y demás campos GPU pendientes.
Historial observado CPU por ruta ejecutable Windows implementado, opt-in
include_app_history e informe windows_app_history con disponibilidad por campo:
WINDOWS_APP_HISTORY.md. Persistencia explícita separada preservando disponibilidad:
WINDOWS_HISTORY_STORE.md; Windows78/78, Linux58/58 y consumidores instalados exit0.
Red/notificaciones pendientes; no reconstruye actividad anterior a la captura.
Medición CPU positiva del propio test nativo verificada en la unidad de captura.
Startup ahora enumera
registros Run y archivos de carpetas Startup. Los metadatos guardados .lnk/.url,
CompanyName autodeclarado y firma Authenticode local tienen opt-ins separados;
ver WINDOWS_SHORTCUT.md, WINDOWS_STARTUP_PUBLISHER.md y
WINDOWS_STARTUP_SIGNATURE.md. Properties de valores Run implementado por consulta
read-only específica (WINDOWS_STARTUP_PROPERTIES.md), sin expansión ni ejecución.
Properties de archivo fuente Startup añade tamaño/atributos/fechas y metadatos
guardados .lnk/.url del mismo handle (WINDOWS_STARTUP_FILE_PROPERTIES.md).
Control reversible propio HKCU Run separado de aprobación Windows:
WINDOWS_RUN_CONTROL.md. Modelo probado; validación nativa positiva pendiente
por gestor transaccional inactivo6801. Sin fallback destructivo/no transaccional.
Regresión tras esta unidad parcial: Windows73/73, Linux54/54 y consumidores
instalados exit0; no equivale a validación positiva del control Run nativo.
Lectura de estados propios Run/respaldo ahora nativa sin KTM, doble observación
estimated sin mutación. El error6801 transaccional se reprodujo también en lectura
HKCU real; no se asumió que fuera un problema exclusivo de fixture. La consulta
ordinaria funciona y no da por disponible la acción de retirada/restauración.
Listado Run/respaldo HKCU implementado: WINDOWS_RUN_LISTING.md, sin KTM ni IDs
retenidos por el caller; conflictos por nombre case-insensitive y resultados
estimated/no atómicos. Prueba nativa privada y lectura HKCU real, Windows74/74,
Linux54/54, consumidores instalados exit0. No cierra la escritura pendiente.
Informe Run/respaldo integrado en Monitor/Sampler por opt-in defaultfalse:
WINDOWS_RUN_MONITOR.md. Windows75/75/Linux55/55 y consumidores instalados exit0;
Monitor nativo16filas, latest/history y Linux unsupported verificados, sin mutación
ni cambio del StartupStatus genérico. Tener informe no habilita la acción KTM.
Control Run explícito integrado en SystemActions/AuditedActions, distinto de la
acción startup genérica: WINDOWS_RUN_AUDIT.md. Windows76/76/Linux56/56 y consumidores
instalados exit0; privacidad, solicitud/resultado, políticas sink y dispatch inválido
verificados. 6801 nativo reconfirmado, no habilita escritura al envolverla en auditoría.
Firma sin revocación vigente/catálogos ni garantía de ejecución segura;
Windows72/72 y Linux54/54, consumidores instalados exit0.
Resolución efectiva de accesos directos, tareas/paquetes, aprobación
enabled/disabled, impacto y acciones siguen pendientes. No equiparar registro
Run con autorun habilitado.
Los usuarios ya tienen inventario WTS y sumas visibles CPU/RAM por sesión+cuenta;
disco/red por proceso y acciones de sesión siguen pendientes. No usar el ID WTS
de inventario como token persistente de control.
Las acciones start/stop/restart de servicios están implementadas con los permisos
del proceso llamador y pruebas inyectadas, sin mutaciones nativas de prueba.
Run new task también está implementado sin shell mediante CreateProcessW; ver
WINDOWS_LAUNCH.md. End task forzado está implementado con pruebas inyectadas;
ver WINDOWS_TERMINATE.md para límites de seguridad e identidad del PID.
Servicios/acciones soportados y detalles verificados en M07:
WINDOWS_SERVICE_VIEW.md. El helper UAC productivo sigue pendiente en L05;
recursos completos de procesos/sesiones permanecen M02/M06.
EcoQoS se puede consultar y solicitar mediante APIs nativas, incluida acción
con identidad verificada; ver WINDOWS_EFFICIENCY.md. La prioridad combinada
del modo de eficiencia del Administrador de tareas sigue pendiente.
Ver WINDOWS_SERVICES.md para control de servicios.

Los errores de API dejan métricas opcionales sin valor. `total_bytes` aún es un
entero no opcional en la API pública y queda en cero cuando falla la lectura;
comprueba también `available_bytes.support` y su valor antes de mostrar RAM.

## Verificación

Prueba `windows-integration` con assertions activas también en Release: dos
muestras, rango de porcentaje, independencia entre monitores, consistencia
RAM total/disponible/en uso, topología, commit, pools y contadores. Se ejecutó
la suite normal completa: Windows 19/19 y Ubuntu WSL 6/6. La variante de compresión
experimental ON fue verificada previamente con 7/7 pruebas. Esto no sustituye CI ni pruebas
en distintos equipos. `windows-pdh` inyecta permisos denegados en apertura,
alta, muestreo y formato; datos inválidos, recuperación, cambios de topología
y cierre de recursos. También ejecuta el proveedor PDH real en el equipo local
de un grupo. Hotplug real y errores de las otras APIs aún requieren validación.
El parser SMBIOS sí tiene pruebas sintéticas de truncamientos, campos extendidos,
valores desconocidos, dispositivos heterogéneos y envoltorio RawSMBIOSData.
Lectura comprobada en el equipo local: DDR5 DIMM, 5200 MT/s, 2/4 posiciones.

## Referencias de las APIs

Revision54: [memoria framebuffer libre NVIDIA](WINDOWS_NVML_MEMORY.md), lectura
opcional de drivers System32 por identidad CUDA LUID/UUID->NVMLroot UUID/KMTcount1.
No SDK/CLI, no restaDXGI-PDH ni garantia allocatable. Windows129/129 ambas
toolchains,Linux111/111; falta positivo fisico NVIDIA y free AMD/Intel, M04 abierta.

Presupuesto GPU por proceso llamador/nodo0, LOCAL/NON_LOCAL independientes:
[WINDOWS_GPU_PROCESS_BUDGET.md](WINDOWS_GPU_PROCESS_BUDGET.md).
Windows55/55, Linux39/39. Campos nuevos0.x requieren recompilar consumidores.
No sustituye memoria libre/uso global ni modifica reservas.

Sensor principal GPU por KMT/LUID exacto y count físico1:
[WINDOWS_GPU_TEMPERATURE.md](WINDOWS_GPU_TEMPERATURE.md). AMD real43°C;
Windows54/54 y Linux39/39. No SDK/CLI ni BIOS; linked adapters/callers32 pendientes.

Asociación OpenCL alternativa D3D11 verificada en AMD real:
[WINDOWS_OPENCL_INTEROP.md](WINDOWS_OPENCL_INTEROP.md), siempre estimated,
no identidad física LUID. RX7900XT: OpenCL2.0 AMD-APP(3679.0).
Windows53/53, Linux39/39; no dispositivo D3D/cola/contexto OpenCL ni SDK adicional.

OpenCL por LUID válido DXGI: [WINDOWS_GPU_OPENCL.md](WINDOWS_GPU_OPENCL.md).
Driver OpenCL AMD local sin cl_khr_device_uuid: vía LUID sin valor, ahora
complementada por asociación D3D11 estimated, no versión global ni identidad exacta.

Vulkan por GPU DXGI mediante deviceLUID válido (sin match por nombre), consulta
apiVersion física. Ver [WINDOWS_GPU_VULKAN.md](WINDOWS_GPU_VULKAN.md).
Windows51/51, Linux35/35 y build sin headers Vulkan opcionales verificados.

Capacidad D3D12 por el adaptador DXGI exacto: campo `direct3d12_feature_level`,
consulta sin crear dispositivo, no versión del DirectX instalado. Ver
[WINDOWS_GPU_D3D12.md](WINDOWS_GPU_D3D12.md). Windows 50/50, Linux 35/35.

Actualización GPU: fecha INF del controlador y ubicación PCI vinculadas por LUID
KMT a la misma interfaz PnP, verificadas con GPU AMD real y pruebas de fallos.
Ver [WINDOWS_GPU_DEVICES.md](WINDOWS_GPU_DEVICES.md). No lectura BIOS ni match
por nombre. Regresión actual Windows 49/49, Linux 35/35.

- [GetSystemTimes y límite por grupo](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getsystemtimes)
- [Topología del procesador](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformationex)
- [Semántica de PSAPI](https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-performance_information)
- [Frecuencia reportada](https://learn.microsoft.com/en-us/windows/win32/power/processor-power-information-str)
- [Contadores y grupos de procesadores](https://techcommunity.microsoft.com/blog/itopstalkblog/tough-questions-answered-how-to-use-more-than-64-logical-processors-on-a-single-/338402)
- [Validación de resultados PDH](https://learn.microsoft.com/en-us/windows/win32/api/pdh/nf-pdh-pdhgetformattedcountervalue)
