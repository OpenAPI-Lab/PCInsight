# Windows: End task forzado

Implementado 2026-10-13. `SystemActions::terminate_process_checked(pid, creation, true)` usa
OpenProcess, IsProcessCritical, TerminateProcess y WaitForSingleObject. No
ejecuta taskkill, PowerShell ni otros programas. No habilita privilegios ni UAC.

## Contrato y seguridad

- `force=false` devuelve `not_supported` sin abrir ni terminar el proceso.
  No existe un cierre cooperativo universal: WM_CLOSE solo aplica a ventanas,
  puede abrir confirmaciones y no garantiza terminar el proceso. Nunca hay
  escalamiento automático de un cierre normal a uno forzado.
- Se rechazan PID 0–4 y el propio proceso llamador. Se solicita únicamente
  PROCESS_TERMINATE, PROCESS_QUERY_LIMITED_INFORMATION y SYNCHRONIZE, sin
  herencia del handle. Acceso denegado se comunica sin intentar eludirlo.
- IsProcessCritical se resuelve desde kernel32 ya cargado. Si falta, falla o
  devuelve crítico, no se solicita la terminación. Esta comprobación no es
  atómica con la terminación: no garantiza seguridad frente a un cambio de
  criticidad concurrente. Tampoco identifica toda aplicación importante.
- Se usa un único handle desde la apertura hasta la verificación final y se
  cierra mediante RAII. No se recorren hijos, sesiones ni árboles de procesos.
  Terminar un anfitrión compartido puede afectar todos sus servicios: para
  servicios, usar control_service en vez de terminar su PID.
- Usar la llamada checked con `ProcessInfo::windows_creation_time_100ns` de la
  muestra seleccionada. Tras OpenProcess se consulta GetProcessTimes en el mismo
  handle: si la fecha no coincide, devuelve `stale_target` antes de esperar o
  solicitar terminación. Si falta permiso o falla la consulta, se rechaza la
  acción. El valor cero es inválido y no hay fallback al PID sin verificar.
- `terminate_process(pid, true)` permanece por compatibilidad, **sin protección
  contra reutilización anterior a OpenProcess**. No usar esa variante para
  selecciones antiguas o terminaciones desatendidas.
- El cierre forzado puede perder datos y no da oportunidad de guardar. El GUI
  debe obtener autorización explícita y ejecutar la llamada fuera de su hilo
  de interfaz. Esta biblioteca no reemplaza esa política de autorización.

## Identidad desde el monitor

```cpp
// selected es el ProcessInfo seleccionado; el GUI ya autorizó el cierre forzado.
if (selected.windows_creation_time_100ns.value) {
    const auto result = actions->terminate_process_checked(
        selected.pid, *selected.windows_creation_time_100ns.value, true);
    // Si es stale_target: refrescar la lista y pedir una nueva selección;
    // no reintentar automáticamente sobre el nuevo ocupante del PID.
}
```

La marca es FILETIME nativo (100 ns desde 1601 UTC), no Unix ni milisegundos.
Debe conservarse sin redondear junto al PID de la misma muestra/equipo; no usar
un double ni persistirla como autorización. Si el proceso es inaccesible queda
sin valor y con el soporte correspondiente. Linux y otros backends actualmente
devuelven `not_supported` en la llamada checked, sin delegar en la antigua.
La comprobación PID+fecha no autentica al programa, no es una frontera de
seguridad frente a manipulación privilegiada y no sustituye la autorización.

Se añadió un campo a ProcessInfo y un método virtual con implementación por
defecto segura a SystemActions. Recompilar biblioteca y consumidores juntos:
no hay ABI estable en 0.x. Subclases existentes no tienen que implementar el
nuevo método, pero devolverán no soportado hasta hacerlo explícitamente.

## Resultados

Después de comprobar identidad, un handle ya señalado devuelve éxito sin
llamar a TerminateProcess. Si hay una
carrera de salida durante TerminateProcess, se confirma en el mismo handle.
En el resto de casos, después de solicitar terminación con exit code 1, se
espera hasta 5000 ms. Solo un handle señalado confirma éxito.

Timeout o error de espera se informa como `system_error` con un mensaje de
resultado no confirmado; **no cancela** la petición ya enviada. Permisos y PID
no encontrado se distinguen como `permission_denied` y `not_found`. Si no se
puede abrir un proceso ya desaparecido, no se presenta como éxito idempotente.
La espera no limita cuánto pueden tardar otras llamadas nativas.

## Verificación

Actualización2026-10-19 revision37: terminación checked real del hijo privado
del propio harness, token obsoleto rechazado sin terminarlo, mismo handle
señalado y exit1 confirmados;20/20 repeticiones. Ver
[WINDOWS_SERVICE_VIEW.md](WINDOWS_SERVICE_VIEW.md). No se terminaron procesos
ajenos. La evidencia de abajo describe las pruebas iniciales2026-10-13.

Pruebas exclusivamente inyectadas: orden de operaciones, permisos solicitados,
rechazo de self/reservados/críticos, API de criticidad ausente o fallida,
proceso inexistente, salida concurrente, fallo de terminación, timeout, estados
inesperados de espera y cierre de handles. La prueba de la API pública usa
solo PID 0 o fecha cero, rechazados antes de OpenProcess. Se añaden fixtures de
identidad coincidente, cambio en los 32 bits altos/bajos, tiempo nulo, error de
GetProcessTimes y mismatch incluso si el proceso ya terminó. La lectura real
del propio proceso compara el dato del monitor con GetProcessTimes y verifica
su estabilidad entre muestras. Ningún proceso real fue terminado.

Suite Release Windows 11/11 y Linux WSL 5/5. Pendientes: prueba con proceso
desechable en VM, identidad verificada en Linux, cierre cooperativo específico de GUI,
helper UAC y validación en más compiladores/equipos.

Fuentes: [TerminateProcess](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminateprocess),
[IsProcessCritical](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-isprocesscritical),
[OpenProcess](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-openprocess),
[GetProcessTimes](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getprocesstimes).
