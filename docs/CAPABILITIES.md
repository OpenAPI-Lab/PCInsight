# Matriz de capacidades y hoja de ruta

| Área | Linux | Windows | Estrategia |
|---|---|---|---|
| CPU, uptime, procesos, threads | Parcial en 0.1 | Inicial implementado; ver WINDOWS.md | `/proc`, `/sys`; Windows GetSystemTimes/topología/PSAPI |
| RAM dinámica | Parcial en 0.1 | Uso/commit/pools implementados; compresión pendiente | `/proc/meminfo`; Windows GetPerformanceInfo |
| DDR, slots, velocidad, form factor | Lector SMBIOS propio | Implementado según firmware | Tabla DMI binaria en sysfs; GetSystemFirmwareTable en Windows |
| Disco físico y volúmenes | Dinámico en 0.2 | Inventario/PDH/volúmenes/medio reportado; validación WMI con permisos pendiente | sysfs + diskstats + statvfs; SetupAPI/Storage IOCTL/PDH/PSAPI/WMI en Windows |
| Ethernet y direcciones | Parcial en 0.1 | IP Helper implementado; ver WINDOWS_NETWORK.md | sysfs/getifaddrs/netlink; GetIfTable2/unicast IP |
| GPU genérica | DRM/hwmon parcial en 0.2 | Pendiente | DRM/hwmon + backends opcionales de driver; DXGI/PDH/vendor APIs |
| Procesos: CPU/RAM | Parcial en 0.1 | Inicial implementado; ver WINDOWS_PROCESSES.md | deltas de ticks/RSS; Toolhelp/Win32 |
| Procesos: disco | Implementado en 0.2 | Decoder ETW/prueba de hilo privado; proveedor general pendiente | `/proc/PID/io`; ETW, no contadores genericos como proxy; ver WINDOWS_DISK_EVENTS.md |
| Procesos: red | F03 fuente opcional implementada: seleccion/admisiones visibles explicitas, standalone/Monitor/Sampler; vista real30/30 strict validada; no total universal | Privilegiado; sin proveedor Windows nuevo | LINUX_NETWORK_ACCEPTANCE.md; payload TCP/UDP instrumentado/ventanas nominales, cobertura y permisos tipados, no wire/async total; H04/J04 separados |
| Productor red compartido | PUBLICO en modo observado: standalone/Monitor/Sampler/owner;65privados reales en dos lotes, hasta1024 actores estructurales; updates retienen origen/readmision fresca | No conectado, fixtures comunes | LINUX_NETWORK_SCALE.md; inicial N+6+8*ceil(N/64)FDs,3 sondas por lote/no constantes tras churn; no captura global |
| Preparacion de seleccion red | Owner readonly PID/start, adquiere/retiene/retira pidfds; max1024 estructural con rechazo explicito, sin enumeracion/captura automatica | Procfs/pidfd, permisos del sistema | LinuxProcessNetworkSelectionOwner; LINUX_NETWORK_SCALE.md;1024/16384 fixtures,65privados nativos/no garantia universal |
| Inventario para seleccion red | /proc visible, barrido completo no atomico; estados y motivos por PID, sin ocultar desconocidos; puente estricto max1024, no captura | Solo lectura procfs/pidfd, vista/permisos actuales | read_linux_network_process_inventory; LINUX_NETWORK_INVENTORY.md y LINUX_NETWORK_SCALE.md; no censo host |
| Admision de red visible | Standalone/Monitor/Sampler, metadata y red pareadas por snapshot/historial, refresh explicito, default estricto; conjunto igual conserva tasas | No disponible; fixtures de ciclo de vida | LINUX_NETWORK_ADMISSION_MONITOR.md; nativo SOLO2privados/enumerador restringido, NO captura global; SystemSnapshot0.x ampliado/recompilar |
| Politica de admision parcial | Opt-in verified_only, exclusiones PID/estado/motivo junto al sample; plan puro; default estricto y rechazo>1024elegibles | Sin proveedor; fixtures de politica/controlador | LINUX_NETWORK_ADMISSION_POLICY.md y LINUX_NETWORK_SCALE.md; procfs verificado NO es permiso BPF; runtime falla cierra TODO |
| TCP sockets/UID | Sampler opt-in SOCK_DIAG implementado, ESTABLISHED/CLOSE_WAIT; no PID ni red total | API ausente motivada | LINUX_TCP.md; sin CLI/eBPF/elevacion, sujeto a soporte/politicas del kernel |
| Servicios | Inventario/estado y control confirmado systemd verificados; SysV/OpenRC por archivos parcial | Inventario SCM y start/stop/restart; validación aislada real pendiente | systemd D-Bus y lectores de configuración; Service Control Manager |
| Inicio automático | XDG implementado | Pendiente | XDG autostart; Registry/Startup/Task Scheduler |
| Usuarios y sesiones | utmpx implementado | Pendiente | utmpx y futuro logind; WTS APIs |
| Historial de apps | CPU observado en memoria; LinuxHistoryStore persiste filas/disponibilidad/notas, red F03->H04 pendiente | Registro propio/OS parcial | LINUX_HISTORY_STORE.md; fusión acumulativa sin doble cuenta, formato separado del legacy/Windows, no escrituras automáticas |
| Acciones | Procesos/XDG/systemd/sesiones | Run new task, End task forzado, EcoQoS y control SCM; resto/UAC pendiente | Implementaciones separadas, autorización y auditoría |

## Fases recomendadas

### 0.2 — telemetría Linux completa

- Deltas de `/proc/diskstats` y `/proc/PID/io`.
- Volúmenes, capacidad formateada, disco del sistema y swap/page file.
- Netlink para metadatos de red y rutas.
- Ampliar el lector SMBIOS propio para inventario adicional, sin ejecutar comandos.
- Enumeración de servicios systemd, sesiones logind y entradas XDG/systemd de inicio.

### 0.3 — GPU por plugins

- Backend DRM común.
- NVIDIA NVML, AMD ROCm/sysfs e Intel Level Zero.
- Enumeración de todos los motores disponibles; el GUI decide cuáles dos `compute` mostrar.
- Vulkan, OpenGL y OpenCL consultados mediante loaders opcionales.

### 0.4 — acciones y seguridad

- `terminate_process`: SIGTERM primero; SIGKILL solo cuando el llamador lo solicita.
- Servicios mediante D-Bus/polkit, nunca construyendo comandos de shell con nombres del usuario.
- Enable/disable de startup con transacciones recuperables.
- Desconexión de sesiones mediante logind.
- Registro de auditoría y errores estructurados.

Auditoría de acciones implementada y verificada mediante `AuditedActions`:
eventos requested/completed, identidad checked conservada y política fail-closed
del sink. Ver [AUDIT.md](AUDIT.md). No sustituye autorización/helper privilegiado.

### 0.5 — backend Windows

- Procesos, servicios, sesiones, startup y performance mediante APIs nativas.
- ETW para disco/red por proceso; PDH para contadores de rendimiento.
- DXGI y contadores GPU; WMI/CIM solo para inventario de baja frecuencia.
- Efficiency Mode mediante EcoQoS/priority APIs cuando la versión de Windows lo permita.

## API que falta diseñar antes de estabilizar 1.0

- Implementaciones y filtros avanzados para `ServiceInfo`, `StartupEntry`, `UserSession` y la vista `AppHistorySample`. `AppHistoryEntry` antiguo es payload legacy, no modelo de mediciones con disponibilidad; ver METRIC_AVAILABILITY.md.
- Observador asíncrono con `std::stop_token`, intervalo configurable y backpressure.
- Selección de campos para evitar que una vista de CPU pague el costo de enumerar todo.
- IDs estables, timestamps monotónicos, unidades SI/IEC explícitas y versionado ABI.
- Errores con `std::error_code` y capacidades consultables por backend.

## Seguridad

La biblioteca de lectura debe funcionar sin privilegios siempre que sea posible. Las acciones deben vivir en un proceso helper mínimo, autenticado por polkit/UAC, con lista cerrada de operaciones. “Run new task” corresponde al GUI o a un módulo de lanzamiento explícito; no debe aceptar una cadena de shell sin validar.

## Política Linux sin aplicaciones externas

El núcleo no invoca procesos ni analiza texto producido por utilidades. El orden de fuentes es:

1. ABI del kernel: procfs, sysfs, netlink, sockets y syscalls.
2. Firmware expuesto por el kernel: SMBIOS/DMI, ACPI y device tree.
3. Interfaces del driver: DRM, hwmon y atributos PCI.
4. Biblioteca del driver ya presente, cargada opcionalmente en runtime, solo para métricas imposibles mediante ABI genérica.
5. Helper privilegiado incluido con PCInsight para lecturas protegidas; nunca exigir al usuario instalar una utilidad de terceros.

No existe una interfaz universal de “servicios” en todas las distribuciones: systemd, OpenRC, runit y s6 tienen modelos distintos. Esa sección necesita adaptadores nativos por init system. De igual modo, firmware no contiene utilización 3D, copy o compute: esos valores vivos únicamente existen si el driver GPU los publica. La API debe declarar la ausencia con precisión en vez de inventar números.
