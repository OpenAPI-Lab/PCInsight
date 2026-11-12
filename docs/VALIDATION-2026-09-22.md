# Validación del 22 de septiembre de 2026

Se retomó el plan original de PCInsight. No se añadió una dependencia de modelos
de IA a la biblioteca.

## Correcciones y entregables

- Corregidas conversiones `Metric<uint64_t>` que impedían compilar el backend
  Linux con GCC 13 (en Windows ese código no se compilaba).
- CPU total excluye guest/guest_nice ya incluidos en user/nice. CPU de un
  proceso excluye el tiempo acumulado de hijos finalizados.
- Las comprobaciones assert de los tests existentes permanecen activas en
  Release. Anteriormente NDEBUG podía eliminar también operaciones de prueba.
- Sampler captura errores del recolector, publica `last_error()`, sincroniza
  start/stop y despierta el worker al destruirlo por asignación de movimiento.
- Instalación CMake completa con Config, Version, Threads y política ABI 0.x.

## Evidencia

- Windows, GCC 16.2, Release: 4/4 pruebas pasan.
- Ubuntu WSL, GCC 13.3, Release: 5/5 pruebas pasan, incluida integración Linux.
- Consumidor externo con find_package: compila y ejecuta en ambos sistemas.
- Instalación Windows copiada a otro prefijo: consumidor compila y ejecuta.
- Prueba Sampler: movimiento con worker durmiendo una hora, stop/start y
  llamadas concurrentes a start/stop. Límite CTest: 20 segundos.

Esto no valida todo el hardware GPU, todos los init systems ni las acciones
destructivas. No se ejecutaron acciones sobre servicios o sesiones del usuario.
La ejecución remota de CI sigue pendiente.

## Seguimiento

ClickUp responde nuevamente. Packaging cerrado: 30/53 subtareas completas,
57% por conteo. HistoryStore se mantiene en progreso tras la revisión:
reemplazo de archivos, merge de snapshots acumulados y entradas corruptas
requieren pruebas y correcciones adicionales. Las pruebas básicas que pasan
no demuestran esas garantías.
