# Windows: control de eficiencia EcoQoS

Implementado 2026-10-13 con GetProcessInformation y SetProcessInformation,
resueltos desde kernel32 ya cargado. No requiere utilidades externas.

## Lectura

ProcessInfo publica `efficiency_mode_available` cuando la API de política
energética puede consultarse, y `efficiency_mode_requested` para el bit
EXECUTION_SPEED controlado y activado explícitamente. Disponible para lectura
no garantiza permisos de escritura. Ausencia de APIs, clase no soportada,
fallos o estructura desconocida dejan las métricas sin valor. Un error de
acceso denegado se informa como permission_required.

El campo requested representa la solicitud explícita. Una política administrada
por Windows puede aplicar QoS según sus heurísticas aunque el campo sea false.
No se mide el ahorro obtenido, la frecuencia real ni la selección de núcleos.
Las versiones de Windows que no permiten consultar ProcessPowerThrottling
pueden dejar la lectura sin dato aun cuando SetProcessInformation lo acepte.

## Acción

```cpp
// Con autorización explícita del usuario y la muestra seleccionada:
if (selected.windows_creation_time_100ns.value) {
    auto result = actions->set_efficiency_mode_checked(
        selected.pid, *selected.windows_creation_time_100ns.value, true);
}
```

La variante checked verifica PID+fecha de creación en el mismo handle que
se usa para cambiar la política. Identidad distinta devuelve stale_target;
fecha cero se rechaza y nunca hay fallback. La variante antigua
set_efficiency_mode(pid,enabled) conserva el riesgo de PID reutilizado antes
de abrirlo. Linux y otros backends no implementan aún la variante checked.

Se solicita PROCESS_SET_INFORMATION, PROCESS_QUERY_LIMITED_INFORMATION y
SYNCHRONIZE, sin habilitar privilegios. Se rechazan PID 0–4, propio proceso,
procesos ya terminados y procesos críticos. Si falta IsProcessCritical o
falla la consulta se rechaza la modificación. Son comprobaciones secuenciales;
no sustituyen la autorización ni impiden todo cambio concurrente del proceso.

Activar establece ControlMask=EXECUTION_SPEED y StateMask=EXECUTION_SPEED.
Desactivar mantiene ese control explícito y pone StateMask=0: **no devuelve
el mecanismo a gestión automática ni restaura la política anterior**. Solo
se selecciona el mecanismo de velocidad; no se seleccionan políticas de timers
ni se cambia la clase de prioridad CPU.

Éxito significa que Windows aceptó la política, no que se haya verificado su
efecto o permanencia. EcoQoS es el componente energético; el modo combinado
del Administrador de tareas puede incluir prioridad CPU, que no se implementa
en esta acción. La biblioteca no promete equivalencia exacta con ese GUI.
No se gestionan árboles de procesos, thread QoS, reinicios ni persistencia.

## Verificación

Actualización2026-10-19 revision37: EcoQoS checked on/off nativo positivo en
un hijo privado del propio harness, bit solicitado releído después de cada
cambio y token obsoleto rechazado;20/20 repeticiones. El hijo se terminó y
limpió al finalizar; sin políticas del padre/procesos ajenos modificadas.
Ver [WINDOWS_SERVICE_VIEW.md](WINDOWS_SERVICE_VIEW.md). No prueba ahorro
efectivo ni implementa prioridad combinada. Lo siguiente es evidencia inicial2026-10-13.

Fixtures inyectados comprueban máscaras on/off, derechos del handle, identidad,
API ausente, crítico, proceso salido, errores de consulta/modificación y cierre
de handles. La única consulta nativa adicional lee el propio proceso; no se
cambió la política de ningún proceso real. Windows Release 14/14, Linux WSL 6/6.
Como se amplió ProcessInfo y la interfaz virtual, recompilar consumidores.

Pendientes: prioridad CPU combinada con restauración definida, política automática
si se expone otra acción, integración de cambios en VM, más versiones de Windows,
hardware y compiladores. La subtarea completa de procesos sigue en progreso.

Referencias: [SetProcessInformation](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setprocessinformation),
[GetProcessInformation](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocessinformation),
[Quality of Service](https://learn.microsoft.com/en-us/windows/win32/procthread/quality-of-service).
