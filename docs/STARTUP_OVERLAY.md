# Impacto observado en el inventario para GUI

Unidad terminada: apply_startup_impact_observations, API en startup_overlay.hpp.
Integra observaciones aceptadas explícitamente con SystemSnapshot.startup_entries,
por ID EXACTO, sin native I/O, threads, auto registro ni side effects del SO.
Funciona sobre datos portables; no implica un lector nativo Windows nuevo.

```cpp
auto view = monitor.sample(options); // startup_entries activado, snapshot fresco
auto rows = observer.observations();
auto result = pcinsight::apply_startup_impact_observations(view, rows, accepted_run_context);
// GUI: usar observed_startup_impact.value/support/note + impact_scope.
// CPU/E/S/intervalo: observed_startup_cpu_time/disk_bytes/interval.
```

expected_native_context viene de la fase aceptada (baseline.native_context), no
de una etiqueta de usuario elegida como autenticación. StartupImpactObservation
ahora incluye native_context. Se rechazan lotes mezclados entre contextos; no se
consulta el boot actual ni se autentica el caller. No persistir timepoints como
identidades universales. Caller elige fase/contexto válidos y sincroniza el snapshot.

Un registro: copia deltas CPU/E/S y duración real, sin sumar veces previas.
Los valores con soporte available/estimated se etiquetan estimated en el GUI;
valores con unsupported/permission_required no se publican como números, aunque
un proveedor externo los haya puesto en value. Baseline solo -> unknown/sin delta.
Low/medium solo con ventana closed/complete, sin gap y evidencia suficiente.
High conocido puede sobrevivir ventana parcial como límite inferior observado.
No se inventa none por disabled/zero ni cambia status/configuración.

Varios procesos de un ID: no suma CPU/disco/windows ni toma máximo de medium/low.
Si alguno tiene high conocido, aplica high estimated como límite inferior; de
otro modo impacto unknown. Totales/intervalo agregados permanecen unknown: una
política de agregación debe resolver contabilidad de hijos/overlap explícitamente.
El campo legacy entry.impact es shorthand del resultado conocido o not_measured;
**no mostrarlo solo** como si fuese impacto completo/certificado del arranque.
impact_scope declara asociación del caller, scope single/multiple y origen
automático NOT certified. Ningún marcador GIO prueba por sí solo esa causa.

IDs de entry no presentes se cuentan y se ignoran, no se relabelan por basename.
Entradas sin observaciones permanecen intactas. Usar snapshot fresco por phase/
refresh; reutilizar una vista vieja sin obs no purga overlays viejos automáticamente.
Name/command/publisher/status/condiciones no se modifican. Mismo lote repetido
reemplaza datos, no incrementa counters ni suma historias. ID exacto no certifica
que el contenido .desktop no cambió después del lanzamiento.

Se validan IDs/counters/identity/contexto/sample_count/interval positivo, coherencia
measurement-observation, flags window y clasificación soportada por contadores.
Una identidad PID/start_ticks no se acepta dos veces ni para dos apps. No se
recalcula la policy original (no viene en la fila) ni se autentican datos fabricados
por el consumidor. Contexto distinto -> stale_target; resto inválido ->
invalid_argument. Validación completa y staging antes de noexcept commit:
errores de validación no dejan un snapshot parcialmente modificado. No hay logs
de datos/argumentos privados ni mensajes de exceptions del proveedor.

Límites:4096 observations,100000 entries, contexto1024 bytes/ID32KiB/nota4096,
8MiB de IDs/notas/contextos de entrada validados (más overhead/staging acotado).
Errores de asignación pueden propagarse antes del commit sin mutar datos. No
sincroniza escrituras de terceros al mismo snapshot; caller coordina acceso.
Structs/API públicas0.x ampliadas: reconstruir biblioteca y consumidores juntos.

Verificado 2026-10-14: Linux31/31 y Windows42/42 Release, instalación y consumidores
externos ejecutados. Tests portable: medium/low parcial/high límite inferior,
multi-process sin sumas, no delta/permiso/unsupported values, idempotencia, entradas
intactas/ignoradas, contexto/identidad/metadata inválidos y rollback de validación.
Pipeline Linux real: XDG /tmp propio + Monitor + baseline del proceso de test +
observer + overlay; métricas GUI disponibles estimated y flags intactos. Monitor
nuevo sin apply conserva not_measured, prueba de que no hay overlay oculto.
No app/Exec real de startup ejecutado ni archivo/servicio real modificado.

Esta unidad cierra el ENLACE explícito de medidas con inventario. Falta la captura
temprana automática con causa confiable, vida corta/descendientes, agregación y
persistencia. No cerrar startup impact completo por este enlace.

Actualización posterior: STARTUP_STORE.md implementa la persistencia explícita
de capturas cerradas. No aplica archivos cargados automáticamente al inventario
ni los convierte en datos actuales del boot o de procesos vivos.
