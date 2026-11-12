# Alcance real de PCInsight

**Idioma:** Español · [English](SUPPORT_STATUS.en.md)

PCInsight 0.2.0 es un SDK en desarrollo. Está disponible para probar e integrar,
pero no completa todas las funciones de un Administrador de tareas ni asegura
que cualquier dato exista en toda distribución. Este documento separa
**implementación**, **validación** y **funciones ausentes**. Los porcentajes de
un proyecto consumidor no son un indicador del estado de esta API.

## Qué puede usarse en Linux

| Área | Implementación disponible | Límites que debe mostrar el consumidor |
| --- | --- | --- |
| CPU | Identidad, utilización entre muestras, frecuencias/topología/cachés/conteos/uptime cuando hay fuente | Base y frecuencia actual son distintas. KVM no demuestra el estado del BIOS. Topología o frecuencia pueden ser estimadas o desconocidas. |
| RAM | Totales visibles, uso, disponible, commit, cachés, detalles del kernel y lectura SMBIOS | DDR/slots/velocidad dependen de firmware y acceso. zram/zswap no equivalen a compresión global Windows. Una resta de RAM instalada y visible no es reserva hardware exacta. |
| Disco | Inventario, tasas, actividad y latencia del proveedor, capacidad, montajes y asociación con raíz/swap | Capacidad montada no prueba toda capacidad formateada. No inferir SSD por nombre ni repartir arbitrariamente capas RAID/LVM. |
| Red | Adaptadores, tasas, direcciones por índice y consulta explícita de IP con disponibilidad | No identifica cualquier marca/modelo comercial. Consulta IP y snapshot no son atómicos; un índice puede reutilizarse. Una lista legacy vacía no demuestra protocolo deshabilitado. |
| GPU | Inventario y telemetría que publican DRM/sysfs/hwmon y proveedores API disponibles | No hay telemetría global completa garantizada en AMD/Intel/NVIDIA. Motores, temperatura, memoria o versiones pueden faltar. GTT no es automáticamente la memoria compartida de Windows. |
| Procesos y detalles | PID, identidad, campos accesibles de usuario/estado/arquitectura/ejecutable, CPU/RAM y E/S según proveedor | Snapshot no atómico; procesos pueden desaparecer o ser inaccesibles. E/S y RSS no equivalen a espacio de disco y RAM privada. Red por proceso requiere captura explícita y permisos. |
| Historial | Observación y almacenamiento según la fuente de CPU, red o notificaciones | No reconstruye historial previo. Las fuentes tienen periodos y coberturas distintos; no combinarlas como si fueran un registro universal. Instalar el SDK no activa observadores. |
| Startup | Inventario XDG, elegibilidad, propiedades, cambios personales y medición/overlays de impacto observado | No cubre todo mecanismo de inicio de todo escritorio ni demuestra causa de inicio por nombre/PID. Publisher es metadata declarada, no firma autenticada. |
| Usuarios | Sesiones disponibles y recursos visibles agregados con ámbito declarado | No atribución universal por sesión ni suma de memoria física única. Disconnect y cerrar sesión no son la misma acción. |
| Servicios | systemd y adaptadores separados para runit/s6/OpenRC/SysV | Inventario de archivos/marcas no prueba estado vivo. No hay detección universal de init; seleccionar proveedor no autoriza control. |

Para campos, unidades, funciones y pruebas concretas, consulta los contratos de
[rendimiento](LINUX_PERFORMANCE_REQUIREMENTS.md), [procesos](LINUX_PROCESS_REQUIREMENTS.md),
[historial](LINUX_APP_HISTORY_REQUIREMENTS.md), [startup](LINUX_STARTUP_REQUIREMENTS.md),
[usuarios](LINUX_USERS_REQUIREMENTS.md) y [servicios](LINUX_SERVICES_REQUIREMENTS.md).

## Faltantes funcionales: no son solo pruebas de hardware

| Función | Qué falta o no se ofrece |
| --- | --- |
| Virtualización configurada en BIOS/UEFI | No hay proveedor OEM validado e implementado para el ajuste correcto. No se reemplaza por flags CPU, SMBIOS o acceso KVM. |
| Start/stop/restart SysV | El catálogo de solo lectura existe; el control por scripts no está implementado. No hay fallback que ejecute el script. |
| Atribución automática completa de startup | No se demuestra que el SO inició todas las aplicaciones observadas ni se captura universalmente todo el login. Se mantiene el papel de observador, no un iniciador que sustituya al escritorio. |
| Helper privilegiado operativo | Hay componentes internos de planificación/autorización y pruebas privadas; falta un despliegue integrado operativo. La biblioteca no instala un servicio ni eleva permisos automáticamente. |
| Efficiency mode genérico seguro en Linux | La acción genérica está deshabilitada. La vía avanzada no verificada requiere aceptación explícita de riesgos y no debe ser un fallback automático. |

Los pools paged/nonpaged y DirectX de Windows no se presentan como datos Linux.
Tampoco se inventa una fecha de driver o telemetría GPU que la fuente no publique.
No prometer una equivalencia simplemente porque el tipo C++ tenga ese campo.

## Acciones y permisos

Las lecturas no ejecutan acciones. Terminar procesos, modificar autostart,
cerrar sesión o controlar servicios requieren una petición separada y los
permisos correspondientes. Usa identidades verificadas donde se ofrezcan;
un PID o un nombre visible no es autorización. No hagas fallback automático
a otra operación cuando haya un rechazo.

El control OpenRC es opt-in y usa su runtime/scripts solo para operaciones
solicitadas explícitamente, no para recopilar datos. No está habilitado por
defecto. No extrapolar ese mecanismo a SysV. Las acciones con los permisos
actuales del llamador no equivalen a disponer de un helper de elevación.

## Qué se ha probado y qué sigue sin probar

- Evidencia local de referencia: Ubuntu WSL x86-64, GCC 13.3 y Clang 18.1.3 con libstdc++. La revisión 230 pasó 201 pruebas GCC y 198 Clang, con recorder opcional activado solo en GCC; 13 pruebas de entrega por perfil. Las revisiones posteriores añadieron pruebas y cambios focales; no se les atribuye automáticamente aquella suite completa.
- Las últimas mejoras de consulta IP y permisos tienen pruebas sintéticas, nativas de loopback, consumidores de SDK y comprobaciones de paquete. Un caso simulado no prueba el hardware correspondiente.
- Falta una matriz física suficiente de CPU/RAM/firmware, discos/topologías, adaptadores y GPU AMD/Intel/NVIDIA, así como aceptación de supervisores alternativos reales en sus entornos. [Cómo ayudar](LINUX_HARDWARE_EVIDENCE.md).
- Hay workflows GCC/Clang preparados, pero la comprobación local no demuestra que la CI de este repositorio haya pasado en GitHub. Revisar los resultados de Actions después de publicarlo.
- La ABI 1.0 y la auditoría final no están cerradas. C++20 y la versión exacta del paquete no garantizan compatibilidad entre cualquier compilador/runtime/revisión. Reconstruye consumidores al cambiar de SDK.
- Hay fallos intermitentes históricos de CPU/historial que no se reprodujeron en las repeticiones recientes. No se afirma que su causa haya quedado resuelta solo porque los tests actuales pasen.

La falta de equipo físico no impide publicar el alcance disponible. Debe
permanecer visible qué fue probado, simulado o todavía no validado. Windows
contiene desarrollo parcial en el fuente, pero está fuera del foco actual;
no se anuncia paridad con Linux ni con el Administrador de tareas de Windows.
