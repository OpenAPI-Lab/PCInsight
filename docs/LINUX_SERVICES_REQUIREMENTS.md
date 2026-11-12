# N03: Servicios Linux para el GUI

Revisión 165, 2026-11-06. Traza R17 y acciones de servicio a K01–K04 y L05.
**Inventario, vida del servicio y capacidad de control son cosas distintas.**
La fuente seleccionada para leer no habilita ni selecciona la API de control.

## Lectura y presentación

`Monitor::sample` con `include_services=true` devuelve `services` y
`linux_service_inventory_source`. Revisar valor, soporte y nota antes de
mostrar filas: una lista vacía conocida no equivale a fuente indisponible.
Un censo parcial no certifica cobertura. `legacy_default` conserva su fallback
histórico por archivos; los modos explícitos no cambian de proveedor al fallar.
No hay detección universal del init. [Selección](LINUX_SERVICE_SELECTION.md),
[pruebas](../tests/linux_service_selection.cpp).

| Campo | API | Cómo interpretarlo |
| --- | --- | --- |
| Name | `ServiceInfo.id`, `linux_details.name` | Etiqueta y clave del ámbito observado, no autorización. Para systemd, usar el nombre canónico `.service` y el ámbito explícito. No pasar un filename, PID o ID de otro proveedor. |
| PID | `linux_details.main_pid` | MainPID reportado por systemd: `0` conocido significa que no reporta PID principal. Ausente es desconocido. Ni siquiera un positivo es un pidfd o token para terminar un proceso. No promover PID crudo de supervisor a PID verificado. |
| Description | `linux_details.description` | Descripción reportada o metadata según proveedor; cadena vacía conocida es válida. No rellenar ausencias con el nombre ni certificar procedencia comercial. |
| Status | `linux_details.status`, `load_state`, `active_state`, `sub_state`, `unit_file_state` | Estado nativo/representación del proveedor. Estado de instalación no equivale a ejecución. Un candidato, record o marca puede existir sin proceso vivo; no normalizar todo a running/stopped. |
| Group | `linux_details.group_kind` y `slice` | En systemd es agrupación Slice, no grupo de orden de carga Windows ni grupo Unix de credenciales. Un grupo vacío conocido difiere de uno desconocido. El campo `provider` va separado. |

Primero comprobar el Metric exterior `linux_details`, luego cada campo;
evitar inferencias desde los strings/PID legacy. [Metadata systemd](LINUX_SERVICE_METADATA.md),
[pruebas](../tests/linux_systemd.cpp).

| Proveedor explícito | Qué informa y qué no |
| --- | --- |
| `systemd_system` / `systemd_current_user` | Managers separados, el segundo es del UID efectivo del caller. No unir por nombre de unidad ni usar el owner como autorización duradera. [API](../include/pcinsight/linux_services.hpp), [ámbitos](LINUX_USER_SERVICES.md). |
| `sysv` | Catálogo y, con presupuesto, comentarios LSB como metadata. No se ejecutan scripts ni se obtiene vida por ver un archivo o enlace. [Proyección](LINUX_SYSV_SERVICES.md), [pruebas](../tests/linux_sysv_services.cpp). |
| `runit` / `s6` | Reportes/records del supervisor con sus scopes, no prueba de vida actual ni readiness. [runit](LINUX_RUNIT_SERVICES.md), [s6](LINUX_S6_SERVICES.md), [pruebas runit](../tests/linux_runit_services.cpp), [pruebas s6](../tests/linux_s6_services.cpp). |
| `openrc_configured` / `openrc_marked` | Dos vistas independientes: candidatos/referencias de configuración frente a marcas runtime. No unir por basename; `marked-started` no prueba vida, PID o permiso de control. [Proyección](LINUX_OPENRC_SERVICES.md), [pruebas](../tests/linux_openrc_services.cpp). |

Las raíces filesystem son elegidas explícitamente por el host, no rutas
universales. Usar las consultas tipadas de cada proveedor cuando se necesiten
sus records, relaciones o diagnósticos crudos; no forzarlos al modelo systemd.

## Acciones separadas

| Acción | Contrato |
| --- | --- |
| Open service | `open_linux_service(LinuxServiceTarget)` consulta detalles de un nombre observado, en su propio scope systemd. No abre aplicaciones, no carga unidades por nombre ni adivina aliases. El GUI presenta la vista. [Contrato](LINUX_USER_SERVICES.md), [pruebas](../tests/linux_services.cpp). |
| Start / Stop / Restart systemd | `control_linux_service(target, action)` en el manager explícito; `SystemActions::control_service` conserva scope sistema. Solicitud expresa, permisos nativos, sin elevación ni fallback. Éxito es el job de esa solicitud con resultado `done`, no readiness ni estado permanente. Timeout no cancela. [Contrato](LINUX_SERVICE_ACTIONS.md). |
| Control runit / s6 | Adaptador seleccionado explícitamente (`LinuxRunitService` o `LinuxS6Control`) y `request(action, autorización)`. Entrega al canal no certifica éxito de servicio; la observación posterior conserva sus límites y no prueba causalidad. [API runit](../include/pcinsight/linux_runit.hpp), [API s6](../include/pcinsight/linux_s6.hpp). |
| Control OpenRC | `LinuxOpenrcService::open(config, name)` fija selección, y `request` exige consentimiento independiente. Opt-in, experimental. Conservar el runner hasta recolectarlo incluso ante error incierto; resultado del runner no es estado del servicio. `close` revoca futuras peticiones, no cancela un runner entregado. [Contrato](LINUX_OPENRC_CONTROL_PUBLIC.md), [pruebas](../tests/linux_openrc_control_public.cpp). |
| Control SysV | No implementado ni autorizado por la excepción OpenRC. No ejecutar un script obtenido del catálogo como fallback. |
| Run new task | Donde el GUI ofrezca esa acción, `launch_process(LaunchRequest)` es un lanzamiento independiente. No enviar comandos derivados de metadata ni presentarlo como control de servicios. [Contrato](LINUX_LAUNCH.md). |

No extrapolar autorización de una lectura previa. Una respuesta incierta no
permite reintentar a ciegas, cancelar ni asumir que no se actuó. La aceptación
con runtimes alternativos reales sigue abierta en K03, y el despliegue del
helper privilegiado en L05 sigue aplazado. [Auditoría K03](LINUX_K03_AUDIT.md).

## Ejemplo SDK

[linux_services.cpp](../examples/linux_services.cpp) consulta systemd sistema
sin fallback con `--sample`; `--details UNIT.service` consulta metadatos en
ese mismo scope. Ninguno llama APIs de control. `--self-test` verifica fuente
indisponible con filas no mostradas, censo vacío conocido, MainPID 0, cadenas
vacías conocidas, estado de instalación separado y marcas no promovidas a PID.
El [consumidor externo](../tests/services_consumer/CMakeLists.txt) valida el SDK
instalado; no impone el build del GUI. Sin cambios de ABI ni biblioteca.

Verificación: Linux Release 200/200, dos pruebas del ejemplo repetidas diez
veces, diez recorridos portables MSVC y 25 enlaces locales comprobados. El
consumidor del SDK instalado pasó self-test y muestra; observó 199 filas
de systemd sistema en WSL sin fallback. Es una observación, no una garantía
de cobertura universal ni validación de control real. Logs `work/services-contract-*`.
