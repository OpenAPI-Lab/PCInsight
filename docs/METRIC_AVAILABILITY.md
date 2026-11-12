# Disponibilidad, unidades e historial compatible (A01)

Modelo para consumidores nuevos: `Metric<T>` con valor opcional, Support y nota.
`metric_is_known(metric)` exige valor presente Y support available/estimated.
Un0 presente cumple esa regla; un false medido también. No usar value_or(0)
para decidir disponibilidad ni tratar permission_required como cero consumo.

| Estado | Interpretación |
|---|---|
| value presente + available | dato publicado por la fuente indicada |
| value presente + estimated | dato/derivado con alcance o aproximación en note |
| value ausente + permission_required | falta acceso; no afirma autorización para elevarse |
| value ausente + unsupported | fuente desactivada/ausente/no implementada/inválida; leer note |

available NO certifica exactitud física, autenticidad, atomicidad ni frescura.
estimated NO significa cero ni admite ignorar límites de atribución. La ausencia
no distingue automáticamente desactivado de error: esa razón sigue en note.
Metric es agregado público y callers pueden construir estados incoherentes; el
helper evita usarlos como conocidos. No modifica ni sanea todo SystemSnapshot.
La vista de historial siguiente sí normaliza sus campos incoherentes.

## Errores de acciones: un contrato distinto

El resultado de una acción (`pcinsight/actions.hpp`) no es una métrica.
Decidir por `ActionResult::error` (o `if (result)` para `ActionError::none`),
no por el texto, el PID ni la presencia de una copia de seguridad. `message`
y `Metric::note` son diagnósticos para mostrar o registrar, no códigos estables.

| `ActionError` | Qué hacer en el consumidor |
|---|---|
| `none` | Mostrar el éxito específico de la operación; por ejemplo, un job systemd completado no certifica que la aplicación esté lista. |
| `not_supported` | Desactivar esa operación en ese contexto; no sustituirla automáticamente por otra más permisiva. |
| `permission_denied` | Mostrar el rechazo. No elevar permisos automáticamente. |
| `not_found`, `stale_target` | Refrescar el inventario. No repetir contra otro destino, ni reutilizar un PID sin verificar su identidad. |
| `invalid_argument` | Revisar la solicitud antes de volver a enviarla. |
| `system_error` | Consultar el estado y el diagnóstico antes de reintentar. Un error no garantiza que nada haya cambiado. |

En Linux, si el lanzamiento devuelve un PID de hijo directo, el llamador debe
recolectarlo con `waitpid` incluso si falló el canal de error y el estado quedó
incierto. Una copia de respaldo puede existir aunque falle la sincronización
posterior de un cambio de inicio. Ni ese PID ni esa ruta convierten el error en
éxito. La prueba `linux-sdk-result-contract` cubre el contrato de interpretación
contra el SDK instalado y dos rechazos Linux sin despachar ninguna acción.

## Reemplazo del historial ambiguo

Incluir `pcinsight/app_history.hpp`. `AppHistorySample` es la vista de lectura
común, con `cpu_time`, `network_bytes` y `notifications` como Metric. Tiene
id/name, period_start/period_end y origin (linux_observed, windows_observed,
legacy_unverified, unspecified). Una instancia por defecto no mide tres ceros.

```cpp
pcinsight::SampleOptions options;
options.include_app_history = true;
auto snapshot = monitor.sample(options);
auto view = pcinsight::app_history_view(snapshot);
if (pcinsight::metric_is_known(view)) {
    for (const auto& app : *view.value) {
        if (pcinsight::metric_is_known(app.cpu_time)) {
            auto nanoseconds = app.cpu_time.value->count();
            // CPU observado, no porcentaje ni tiempo retroactivo garantizado.
        }
        // Red/notificaciones ausentes: mostrar desconocido, no0.
    }
}
```

La sobrecarga de snapshot selecciona solo el informe nativo del build: Linux
usa linux_app_history y Windows windows_app_history. Nunca combina plataformas
ni recurre a snapshot.app_history legacy. Otros backends devuelven unsupported.
Sobrecargas de los dos informes tipados permiten proyectar datos ya recibidos,
también en otro sistema, sin invocar una API del sistema de origen.

Es proyección, no captura/merge/suma. Preserva etiquetas, IDs, periodos, valores
conocidos, soporte y notas de cada campo, y agrega origin. Mantiene periodos
separados/orden, sin deduplicar por nombre/ruta. Los IDs conservan su ámbito de
fuente y no prueban identidad de paquete. No sumar filas de varias fuentes o
colecciones simultáneas como recursos distintos sin política del consumidor.

Informe con lista vacía conocida es válido, distinto de informe ausente. Campo
con valor+unsupported/permission_required pierde el valor; conserva denegación
cuando corresponde. Campo sin valor+available/estimated pasa a unsupported.
Se anota incoherencia; CPU negativo se rechaza por campo sin borrar otras
mediciones independientes válidas. No se clampa ni suma ningún contador.

Límites por conversión:100000 filas,1MiB por id/name,16MiB de texto acumulado,
sin IDs vacíos/NUL en textos, period_start no anterior a época y end>=start.
Metadatos inválidos rechazan el informe entero, sin filas parciales. No hay
overflow de duraciones al ordenar: se comparan time_points, no se restan.
Copias asignan memoria; no hay presupuesto de bytes de todos los Metric/notas,
deadline ni excepción de asignación convertida en dato. Caller sincroniza fuente.

## Legacy explícito, sin adivinar disponibilidad

`LegacyAppHistoryEntry` es alias del AppHistoryEntry original, mismo tipo real,
layout y símbolos. HistoryStore y snapshot.app_history usan el alias para señalar
la frontera. AppHistoryEntry antiguo permanece por compatibilidad de código;
NO es la vista recomendada para telemetría. Native Linux/Windows dejan ese vector
vacío. No se elimina compatibilidad ni se inventa disponibilidad de un archivo.

HistoryStore sigue leyendo/escribiendo exactamente v1/v2, sin nuevas banderas.
`import_legacy_history(loaded.entries)` produce vista legacy_unverified con
identidades/periodos y TODOS los contadores desconocidos, incluso si el valor
raw es0 o positivo. El formato no guardó soporte/procedencia; no se reconstruye
esa evidencia. Payload raw sigue disponible en loaded.entries. CPU raw negativo
o metadatos inválidos rechazan importación. No modifica el objeto de entrada,
no sobrescribe el archivo ni migra contadores silenciosamente.

AppHistorySample y LegacyAppHistoryEntry no son convertibles implícitamente.
No guardar una vista en HistoryStore rellenando métricas ausentes con0.
Quien aporta contadores nuevos conocidos a la API legacy asume su semántica
explícitamente; no puede recuperar ese conocimiento de un archivo antiguo solo
mirando sus números. El tipo legacy persiste números, no confianza.

WindowsHistoryStore sí tiene formato separado que conserva disponibilidad de su
informe Windows. No se añade LinuxHistoryStore ni persistencia del nuevo tipo
común. H02 mantiene su alcance legacy; H04/H05/M08 siguen siendo captura real de
red/notificaciones pendiente, no se cierran por existir campos o esta vista.

## Unidades y fronteras de compatibilidad

Para reglas de continuidad de filas y conversiones del GUI, ver
[unidades e identidad del SDK Linux](LINUX_SDK_UNITS_IDENTITY.md).

| API | Unidad / contrato |
|---|---|
| AppHistorySample.cpu_time | duración chrono::nanoseconds, CPU observado de su fuente |
| network_bytes | bytes acumulados de la fuente, no bytes/s ni bits/s |
| notifications | cuenta acumulada, no tasa ni historial retroactivo |
| period_start/end | system_clock, periodos de captura; no cronómetro monotónico |
| *_bytes de telemetría | bytes; GUI elige GB/GiB, sin escala implícita |
| *_bytes_per_second | bytes/s según ventana del proveedor |
| *_percent | puntos porcentuales, no fracción0..1; alcance según proveedor |
| *_ghz / speed_mts | GHz / MT/s, no intercambiables entre sí |

No todos los porcentajes tienen el mismo denominador: CPU por proceso normaliza
capacidad host y algunos informes DRM sumados pueden superar100. Conservar notas;
no imponer un clamp global por el nombre de la unidad. Usar timestamps steady
de Sampler para gráficas60s, no wall time para restar intervalos bajo saltos de reloj.

Compatibilidad heredada pendiente de estabilización1.0: MemoryInfo.total_bytes
puede ser placeholder0, preferir reported_total_bytes. CPU vendor/model texto
preferir reported_vendor/reported_model. Detalles Linux de proceso y red tienen
informes por campo; textos shorthand no prueban por sí solos disponibilidad.
Totales de red legacy solo se emiten con contadores válidos en Linux, fila omitida
no significa tráfico0. Startup/servicios/sesiones/GPU tienen sus límites en las
tareas de proveedor abiertas. A01 cierra el contrato de modelo/reemplazo de
historial documentado, NO la auditoría final de cada campo, toda cobertura de
backends, compatibilidad1.0 o la matriz de requisitos N03.

Cambio aditivo0.x: alias/helper/nuevo header/tipo/funciones; sin nuevo layout de
estructuras existentes ni cambio de bytes v1/v2. Recompilar consumidores que
usen símbolos nuevos y enlazar biblioteca actual. Requisitos de recompilación
por cambios previos de layout siguen vigentes. A05 (API/ABI1.0) permanece abierta.

## Pruebas

tests/app_history_api.cpp: defaults,0 conocido frente a ausencia, soporte/nota
por campo y exterior, negativos/incoherencias/bounds/periodos/no parcial,
proyecciones Linux/Windows, importación legacy conservadora y selección nativa.
Static asserts prueban alias idéntico y que las vistas no se convierten a legacy.
Monitor opt-in/opt-out nativo y consumidor instalado prueban API pública nueva.
tests/history_store.cpp conserva formato/checksum/migración/merge/retención
usando alias explícito. No captura red/notificaciones ni escribe automáticamente.

Verificacion2026-10-16: Linux83/83 y Windows103/103 Release; paquetes
reinstalados, consumidores externos recompilados y ejecutados exit0. Native
opt-in/opt-out y seleccion de origen comprobados en ambas plataformas. Build
Linux final sin warnings; avisos iniciales de clock-skew en dependencias no
persistieron al recompilar, no se alteraron relojes/archivos para ocultarlos.
A01 cierra el criterio de reemplazo documentado del historial ambiguo; no se
eliminan tipos legacy compatibles ni se declaran medidas ausentes como resueltas.
El riesgo intermitente CPU registrado antes en N03 no queda diagnosticado porque
esta suite paso; C03/E05 y hardware/CI conservan sus pendientes.
