# Auditoría del control Run explícito

Revision48: el metodo tambien reconoce [HKLM32/64 por ID](WINDOWS_MACHINE_RUN_CONTROL.md).
No se agrega virtual/operacion; target conserva hive/vista. Ahora se copia la
observacion antes del sink para que el callback no cambie el request pendiente.
Esto no permite mutacion concurrente durante la copia ni acredita autorizacion.
KTM6801 sigue sin resolver; lecturas nativas positivas no equivalen a mutacion.

`SystemActions::set_windows_run_registration_enabled(expected, enabled)` es la
acción explícita para el [mecanismo propio Run](WINDOWS_RUN_CONTROL.md), separada
de set_startup_enabled genérico y de StartupApproved. Windows delega al mismo
proveedor transaccional de la función libre de startup.hpp; Linux y backends
personalizados sin override devuelven not_supported. Añadir el método no autoriza
ni cambia ninguna entrada al construir el backend.

`AuditedActions` ahora envuelve ese método. El consumidor puede pasar por el
decorador para registrar intento/resultado con su propio sink; la función libre
sigue disponible y no genera auditoría automáticamente. No es observación global
del sistema ni una obligación impuesta a aplicaciones que omitan el decorador.

## Contrato y privacidad

- AuditOperation::windows_run_registration es distinta de startup; se añade al
  final del enum para conservar los ordinales anteriores.
- requested/completed correlacionan el mismo ID local al decorador; enabled=false
  solicita retirar a respaldo, true restaurar, no significa aprobación Windows.
- target copia solo entry_id, con el límite existente4096B y metadata_truncated.
  command, native_data y native_type no se copian al evento. Tampoco diagnósticos
  ni excepciones del backend. ID/actor siguen siendo datos del caller y pueden
  ser sensibles: el ID incluye nombre de valor codificado en hex, no anonimizado.
  Si el caller introduce secretos en ID/actor, la biblioteca no puede detectarlos.
- La solicitud completa, sin truncar ID/tipo/bytes/comando/estado, se entrega al
  método explícito del backend; nunca cae al enable/disable genérico ni a una
  llamada sin la observación esperada. No reinterpreta ni valida por su cuenta
  si la observación autoriza la acción. Eso corresponde al proveedor checked.

Se conserva la política de [auditoría general](AUDIT.md): por defecto, si no se
puede registrar requested, no se invoca el backend. Best_effort debe elegirse
explícitamente. Si falla completed, se mantiene el resultado ya devuelto por el
backend y aumenta el diagnóstico, sin afirmar rollback. Excepciones del backend
se registran como backend_threw/system_error y se propagan; no prueban ausencia
de efectos. backend_invoked indica llamada al proveedor, no escritura realizada,
éxito de commit ni ejecución al próximo login.

El sink debe copiar eventos si los retiene, ser thread-safe y no modificar los
argumentos de la acción desde callbacks. El caller debe mantenerlos válidos y
sin mutación durante la llamada. El backend se serializa por instancia, no entre
otros procesos/decoradores. No existe persistencia de auditoría automática,
autenticación de actor, reintentos destructivos, timeout de sink o helper elevado.
El host sigue siendo responsable del consentimiento y de gestionar fallos del log.

API0.x ampliada (nuevo virtual y enum): recompilar biblioteca y consumidores;
backends personalizados existentes conservan compatibilidad de fuente gracias
al método default not_supported, no compatibilidad binaria de vtables anteriores.

## Evidencia

2026-10-15: Release Windows76/76 y Linux56/56, paquetes y consumidores externos
reconstruidos/ejecutados exit0 en ambos. Reconfirmado6801 del proveedor nativo
en fixture privada; la auditoría no cambia esa disponibilidad.

Pruebas portables: forwarding exacto de todos los campos, separación del método
genérico, resultados none/not_supported/permission_denied/stale_target/system_error,
dos etapas e ID, flags, privacidad, truncación solo del evento, excepción original,
fallos de sink requested/completed y best_effort. La prueba con backend real usa
únicamente un objeto con ID vacío: Windows invalid_argument y Linux not_supported,
sin solicitar una modificación válida del registro. Consumidor instalado usa
también solo esa acción inválida y verifica ambos eventos.

Esto verifica el decorador y el dispatch, **no** una retirada/restauración nativa
exitosa. El pendiente6801 del proveedor permanece; una operación auditada no lo
sortea ni se presenta como habilitada por existir un informe de Monitor.
