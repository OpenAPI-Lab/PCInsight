# Auditoría explícita de acciones

Ampliacion revision46: [accion explicita de archivos Startup Windows](WINDOWS_STARTUP_FILE_AUDIT.md)
integrada en SystemActions/AuditedActions con operacion propia. Requiere recompilar
consumidores/backends por el nuevo virtual0.x; no cambia la aprobacion de Windows.

Unidad funcional terminada: `AuditedActions`, decorador opt-in de SystemActions,
registra solicitud y resultado de las operaciones actuales, incluyendo el cierre
de sesión explícito y las acciones de override/reset personal añadidas después. No ejecuta
comandos, no eleva permisos y no crea archivos ni threads automáticamente.

Ampliación 2026-10-15: operación explícita Windows Run propia, distinta del
startup genérico, integrada en SystemActions/AuditedActions. No copia comando,
bytes nativos ni diagnósticos del backend al evento; sí ID/flag/resultado según
el contrato de privacidad del caller. Ver [auditoría Run](WINDOWS_RUN_AUDIT.md).
Windows76/76 y Linux56/56, consumidores instalados exit0. Esto no valida una
retirada/restauración nativa exitosa: permanece pendiente6801 del proveedor.

```cpp
#include <pcinsight/audit.hpp>
#include <mutex>
#include <vector>

std::mutex audit_mutex;
std::vector<pcinsight::AuditEvent> events; // ejemplo; producción debe limitar/guardar
pcinsight::AuditedActions actions(pcinsight::make_system_actions(),
    [&](const pcinsight::AuditEvent& event) {
        std::lock_guard lock(audit_mutex);
        events.push_back(event);
    }, "GUI local");
// Usar actions en lugar del backend sin auditoría. La creación no muta el SO.
```

El consumidor proporciona un `AuditSink` para cada evento; puede elegir memoria,
archivo, base de datos o una API de logging. Si retiene un evento debe copiarlo
durante el callback. Para un sink persistente, retornar sin excepción debe
significar que cumplió las garantías de almacenamiento que el consumidor necesita.
PCInsight no comprueba ese almacenamiento ni ofrece un registro inviolable.

## Contrato verificado

- Dos etapas, requested/completed, con el mismo ID único dentro del decorador.
  IDs de instancias distintas no son globalmente únicos ni sobreviven reinicios.
- Operación, fecha, duración de ejecución monotónica, target/PID, identidad Windows
  esperada (o expected_linux_start_ticks para terminación checked Linux) y flags
  force/enabled/elevated/service_action. actor_label es una etiqueta
  que aporta el llamador, **no una identidad OS autenticada**.
- El evento no contiene argumentos de lanzamiento, cwd ni texto de diagnóstico
  del backend/excepción. El target (ejecutable, servicio, startup o sesión) y la
  etiqueta sí son datos del llamador y pueden ser sensibles; el consumidor decide
  acceso, retención y sanitización. Cadenas son bytes, no se valida UTF-8 ni se
  escapan saltos de línea/NUL: usar serialización estructurada, no concatenar logs.
- Target/etiqueta limitados a 4096 bytes, con metadata_truncated. Puede cortarse
  una secuencia UTF-8; un serializador debe gestionarlo. **Solo se trunca el evento**:
  los parámetros completos originales llegan al backend sin alterarse.
- Default require_request_record: si falla el registro solicitado, no se invoca
  el backend y se devuelve system_error con mensaje explícito. Se intenta también
  registrar ese rechazo como completed/backend_invoked=false.
- Best_effort se elige explícitamente: fallo de sink no impide ejecutar.
- Un fallo al registrar completed no convierte una operación ya realizada en
  fracaso ni promete rollback. `diagnostics()` informa contadores de fallos y
  último tipo de fallo. El consumidor debe consultarlo y gestionar esa pérdida.
- Excepciones del backend se vuelven a lanzar después de intentar registrar
  completed/system_error/backend_threw=true. Una excepción no demuestra ausencia
  de efectos: no reintentar automáticamente una acción destructiva.
- Se invoca exactamente el método solicitado. Checked terminate/efficiency
  conserva el token de creación y nunca cae en un método PID-only.

## Concurrencia y límites

El acceso al backend está serializado por instancia. Callbacks de sink se invocan
sin locks internos y pueden consultar diagnostics, pero **deben ser thread-safe**.
No se garantiza orden de entrega entre IDs concurrentes: correlacionar por ID
y stage, no por posición de llegada. Evitar callbacks recursivos que vuelvan a
emitir acciones indefinidamente y backends que reentren el mismo decorador.
No destruir la instancia mientras hay operaciones en curso.

Los callbacks son síncronos, sin timeout/cancelación: un sink lento retrasa la
operación. No existe cola/event history sin límites dentro del decorador.
IDs no se reutilizan al alcanzar el límite uint64; la asignación falla antes
de ejecutar. Los contadores de diagnóstico saturan. Las excepciones de sink no
propagan; incluso si el diagnóstico no pudiera actualizarse por un fallo interno,
no se cambia el resultado de una operación ya ejecutada.

El decorador es auditoría de las llamadas que lo atraviesan, no autorización,
un helper polkit/UAC ni observación de acciones realizadas fuera de PCInsight.
No cubre caída del proceso/energía con una garantía de completed: requested puede
quedar sin finalización y debe tratarse como resultado desconocido.

## Criterio de cierre y evidencia

2026-10-14: ocho acciones, parámetros/identidad, dos etapas, resultados y
excepciones conservados, redacción de campos sensibles, truncación sin modificar
acción, política fail-closed/best-effort, fallo posterior y 200 operaciones
concurrentes (400 eventos, sin IDs duplicados), diagnóstico desde el callback
y constructor inválido verificados con backend simulado.

Release Linux 11/11 y Windows 23/23. Ningún proceso/servicio/sesión real fue
modificado por esas pruebas. El consumidor externo del paquete instalado usa
el decorador solo con identidad PID=0/token=0, rechazada sin mutación.
Instalación, recompilación y ejecución de ese consumidor verificadas tanto en
Linux como en Windows con el nuevo header audit.hpp.

Este cierre corresponde a **API de auditoría de acciones y sus pruebas**;
almacenamiento persistente inviolable, autenticación de actor y helpers
privilegiados no forman parte del bloque ni se dan por implementados.
