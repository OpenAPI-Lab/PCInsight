# N03: Usuarios y sesiones Linux para el GUI

Revisión 164, 2026-11-06. Traza R16 y acciones de Users a J01/J02/J04/J05/J06
y L01. **Una fila de sesión no es una fila de consumo de esa sesión**: CPU/RAM,
disco y red tienen agrupaciones diferentes. El GUI debe rotular su alcance,
no sumar filas de sesión ni unir dominios solo porque coincidan nombre o UID.

| Campo | API / unidad | Alcance y evidencia |
| --- | --- | --- |
| User | `UserSession.id/username/display_name`, texto | Identidad y etiquetas reportadas por login1/utmp; dos sesiones del mismo nombre siguen siendo dos filas. Ni una etiqueta ni el ID son credenciales. [Inventario](LINUX_USERS.md). |
| Status | `linux_details.state/active/remote`, estado/bool | Consultar padre y cada Metric. login1 publica estado nativo, actividad de primer plano y origen remoto independientes. `active=false` no es desconexión; `remote=true` no prueba conexión viva. utmp aporta `reported_login`, no estado activo/remoto ni UID inferido. [Contrato](LINUX_SESSION_STATE.md), [pruebas](../tests/linux_session_state.cpp). |
| Fuente de inventario | `snapshot.linux_session_inventory_source`, texto | Consultar separado de `user_sessions`; lista vacía no acredita ausencia global de sesiones. login1 opcional con fallback utmp según disponibilidad; `prefer_logind_sessions=false` solicita utmp. Fallo de una enumeración encontrada no es éxito vacío. [Contrato](LINUX_SESSION_STATE.md). |
| CPU | `UserSession.cpu_percent`, % de capacidad visible | Agregado estimado de procesos visibles por **username**; el primer intervalo puede faltar. Repetido en sesiones del mismo nombre, no CPU de cada sesión ni toda la máquina. Ver `resource_scope`. [Contrato](LINUX_USERS.md), [CPU por proceso](LINUX_PROCESS_CPU.md). |
| Memoria | `UserSession.memory_bytes`, B RSS | Suma estimada visible por **username**; RSS compartido puede contarse varias veces. Ni RAM física única ni memoria atribuida a una sesión. Mismo `resource_scope` que CPU. [Contrato](LINUX_USERS.md). |
| Disco | `linux_io_details.disk_bytes_per_second`, B/s | Agregado parcial por **UID real reportado**, no username ni por sesión. Ver `scope`, `real_uid`, `matching_processes` y `disk_coverage`. UID validado alrededor de lecturas y entre extremos del intervalo de proceso. No throughput físico. [Contrato](LINUX_USER_IO.md), [pruebas](../tests/linux_user_io.cpp). |
| Red | `snapshot.linux_user_network`, `partial_*_bytes_per_second`, B/s de payload | Un UID por **dominio del observador** de F03, independiente del inventario; no por sesión ni total en cable. No usar los campos legacy de red de `UserSession` o `linux_io_details` como sustituto. [Contrato](LINUX_USER_NETWORK.md), [pruebas](../tests/linux_user_network.cpp). |

## Muestreo y cobertura

`include_user_sessions=true` solicita inventario. Ocultar `include_processes`
no evita la recopilación interna para CPU/RAM y disco. Conservar el mismo
Monitor para deltas. Comprobar soporte y notas; un cero conocido se conserva,
pero una ausencia o permiso denegado no se vuelve cero. Los scopes visibles
son parciales por permisos, namespace, NSS y continuidad de muestras. Una
sesión utmp sin UID no recibe disco por coincidencia de username. Para disco,
`known` y `missing` describen las coincidencias; `permission_denied` e
`identity_unverified` son subconjuntos de `missing` que pueden solaparse.
`overflow` señala que no hay suma representable, no un valor saturado.

Red requiere `include_linux_user_network` e `include_linux_process_network`
y una captura F03 observada configurada con autorización previa; las flags
no la inician. No necesita `include_user_sessions`. Mostrar boot token,
dominio de usuario del observador y cobertura; el informe también conserva
productor, época y límites temporales del barrido. No unir por UID aislado
entre boots, namespaces, observadores o con una sesión reportada por un bus
de otro entorno. `initial_real_uids` puede contener varias identidades del
kernel mapeadas a un UID observado. Un UID sin login puede tener fila.

Conservar `selected_processes`, `idle_processes`, `observed_buckets` y
`known/missing/permission_denied/overflow` por TCP envío/recepción y UDP
envío/recepción. Los agregados son tasas parciales, no contadores acumulados.
Sin buckets observados no significa tráfico cero; no inferir cobertura total
de procesos a partir de selección manual. `linux_network_admission`, si existe,
describe la admisión correspondiente. [API pública](../include/pcinsight/linux_user_network.hpp).

## Acciones explícitas

| Acción | API y límite |
| --- | --- |
| Disconnect | `disconnect_user` no está soportado por los adaptadores Linux incluidos. `query_linux_disconnect_capability` y `disconnect_available` permiten mostrar el motivo (false conocido para login1/utmp reconocidos). No usar logout, señal, lock ni otra operación como fallback. [Contrato](LINUX_DISCONNECT.md). |
| Cerrar sesión | `terminate_user_session(logout_token)` es separado y destructivo; pedir confirmación de pérdida de trabajo. Token del inventario vigente, no ID, PID ni TTY; `logout_available` no prueba permisos. El backend revalida identidad y no eleva. Tras aceptación, un error no prueba ausencia de efectos. Salida del objeto de sesión no certifica salida de todos sus procesos. [Contrato](LINUX_LOGOUT.md), [prueba privada](../tests/linux_logout_native.cpp). |
| Manage user accounts | `AccountManagementIntegration` es integración opcional del host, no editor nativo. Configurar consulta read-only y callback de navegación de confianza; reconsulta en cada petición, solo `available + true` invoca. No ejecutar comandos de escritorio inferidos ni elevar. `host_invoked` puede ser true aun con error: no reintentar a ciegas. [Contrato](ACCOUNT_MANAGEMENT.md), [pruebas](../tests/account_management.cpp). |
| Run new task | `launch_process(LaunchRequest)` es la misma acción explícita de los demás paneles; no iniciar como usuario de una fila ni asociar a una sesión por el nombre. [Contrato](LINUX_LAUNCH.md). |

## Ejemplo SDK

[linux_users.cpp](../examples/linux_users.cpp) muestra inventario, métricas
y red independiente, sin hacer joins o totales de sesiones. No imprime
`logout_token`, invoca acciones ni activa captura. `--self-test` verifica
sesiones repetidas, UID/cero válidos, permiso denegado, ausencia de fallback
legacy y separación de red. `--sample` hace dos observaciones Linux de solo
lectura; una sesión vacía o una métrica ausente sigue siendo un resultado
posible. El [consumidor externo](../tests/users_consumer/CMakeLists.txt) usa
el SDK instalado sin imponer CMake al GUI. Sin cambio de ABI ni biblioteca.

Verificación: Linux Release 198/198, dos pruebas del ejemplo repetidas diez
veces, diez recorridos MSVC portables, 20 enlaces locales comprobados y
consumidor del SDK instalado con self-test y muestra exit0. Logs
`work/users-contract-*`. No es nueva validación de cierre de sesiones reales.
