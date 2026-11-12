# K03: auditoría de cierre y paso a L05

Actualización173: [guía de aceptación privada](LINUX_K03_ACCEPTANCE.md),
18/18 pruebas focales en WSL. Sin runtimes en rutas consultadas ni aceptación
real ejecutada. K03 sigue E; la excepción de SysV sigue sin autorización.

Actualización159: [integración pública OpenRC](LINUX_OPENRC_CONTROL_PUBLIC.md)
terminada: selección de archivos/directorios/contexto fijados, consentimiento por
request, selección one-dispatch/revocación, ownership independiente del runner.
Linux188+50OpenRC/MSVC20/SDK3consumidoresrelinkados0; solo modelo privado, no
OpenRC real. Runtimes runsv/s6-supervise/openrc-run/rc-service siguen sin rutas en
PATH WSL consultado (work/openrc-public-runtime-probe.txt), no ausencia global.
K03 E: aceptación aislada real y control SysV pendiente; consulta de excepción
SysV emitida, aún NO concedida. Integración OpenRC ya no es pendiente. L05
despliegue sigue aplazado. Matriz145/direcciones previas son históricas.

Actualización158: se retoma K03 sin ampliar transporte L05. Ejecutor OpenRC
interno opt-in implementado y verificado con modelo privado: rutas/archivos
protegidos, runtime ELF por FD, entorno/FD cerrados, gating/pidfd, ACK/espera,
salida/timeout y TERM/KILL solo del runner explícitamente solicitados. No hay
colección por scripts ni integración pública/aceptación real. Linux187/187,
40 recorridos OpenRC, MSVC10 portable y dos consumidores SDK relinkados exit0.
Ver LINUX_OPENRC_CONTROL.md. K03 E: OpenRC real/integración, aceptación runit/s6
y control SysV sin excepción siguen pendientes. L05 despliegue permanece E
aplazado; la matriz145 siguiente es histórica, no dirección operativa vigente.

Actualización148, 2026-11-02: el usuario aprobó explícitamente scripts/runtime
OpenRC SOLO para start/stop/restart solicitados, opt-in y pruebas privadas, sin
instalación/elevación/servicios reales/datos por scripts. Se retira ese bloqueo de
decisión, no la implementación/aceptación pendiente. SysV no queda autorizado.
Ver [decisión vigente](LINUX_OPENRC_CONTROL_DECISION.md). La matriz145 siguiente
es histórica; no usar su «no autorizado» OpenRC como estado vigente.

Revisión145, 2026-10-30. Criterio original conservado: adaptadores nativos de
OpenRC/runit/s6/SysV, inventario/control y pruebas. K03 **permanece E**, aplazada
en los puntos siguientes, no cerrada ni eliminada del denominador.

| Proveedor | Implementado/verificado | Lo que NO está terminado |
| --- | --- | --- |
| runit | Inventario/status20; FIFO u/d/tcu con pins/consentimiento; observación one-shot; ServiceInfo; selección Monitor | Aceptación con runsv real; observación no demuestra readiness/causa ni identidad de PID de host |
| s6 | Record35/inventario/logger; FIFO u/d/cru con pins/consentimiento; condiciones históricas; ServiceInfo; Monitor | Aceptación con s6-supervise real; archivo/FIFO no certifican vida, versión del peer o completion |
| OpenRC | Catálogo/referencias/stacks directos, marcas separadas, dos vistas ServiceInfo y selección Monitor sin namejoin | Start/stop/restart por scripts/runtime no autorizado; PID/vida no verificables por marcas; aceptación real |
| SysV | Catálogo/referencias, LSB acotado ligado a candidato, ServiceInfo y selección Monitor | Start/stop/restart por scripts no autorizado; PID/vida no derivados de archivos; aceptación real |

No se completó control OpenRC/SysV por implementar lectores. No se reemplaza
control con escritura de marcas, enlaces, pidfiles o señales adivinadas.
Los fixtures privados de FIFO/status son evidencia de protocolo/guardas y
rechazos; no supervisores reales, servicios activos ni una distribución extra.
El éxito de entrega de bytes no equivale a trabajo completado ni readiness.

## Evidencia revisada

Código/API/tests y documentos128..144: LINUX_RUNIT/TRANSITIONS/SERVICES,
LINUX_S6/CONTROL/INVENTORY/OBSERVATION/SERVICES, LINUX_OPENRC/CATALOG/SERVICES,
LINUX_SYSV/LSB/DESCRIBED_CATALOG/SERVICES y LINUX_SERVICE_SELECTION.
Última regresión K03 previa a esta unidad144: Linux170/170+10 selector, MSVC10 puro,
dos consumidores del SDK recompilados exit0. No se reetiqueta como ejecución145.

Comprobación read-only145 en Ubuntu WSL: command -v con nombres literales runsv,
s6-supervise, openrc-run y rc-service no imprimió rutas (exit1). Esto prueba
ausencia en ese PATH en ese momento, NO ausencia universal/paquetes/disco ni
soporte de otras distros. No se instala/ejecuta nada. Primer intento de loop tenía
variables expandidas por PowerShell y no sirve de evidencia; se corrigió a
cuatro command -v literales. No se esconde ese error de comprobación.

## Qué permite retomar cada pendiente

Aceptación real: entorno privado apropiado con supervisor disponible y pruebas
aisladas autorizadas, sin usar servicios personales. No instalar init ni iniciar
servicios reales como supuesto implícito de «prosigue».
OpenRC/SysV control: decisión explícita sobre excepción limitada a acciones de
control con runtime/scripts existentes, nunca recopilación de datos; después
diseño/implementación/validación pendientes. La decisión132 sigue sin concederse.
Un helper polkit no elimina esa restricción ni convierte un script en protocolo.

No hay otra capa de inventario/proyección pendiente que justifique seguir
añadiendo adaptadores duplicados antes de esos puntos. No se cambia el alcance
para cerrar K03 artificialmente. Aceptación real no puede simularse con más
fixtures. Se aplaza ese trabajo y se continúa **L05/Linux** según el plan.

## Handoff acotado

Primera unidad L05: validación interna pura de solicitudes cerradas de control
systemd sistema sobre una allowlist configurada por el futuro mecanismo, no por
el cliente. Sin daemon, socket, setuid, reglas polkit, elevación, instalación,
activación, UAC o operación real. Consentimiento no es autenticación/autorización.
Después identidad IPC/autorización polkit y pruebas de rechazo, aún pendientes.

K03 E; no nueva subtarea/ID/crédito parcial. Inicio L05 no cierra el criterio
compartido con UAC. Porcentaje de completadas no aumenta por este handoff.
