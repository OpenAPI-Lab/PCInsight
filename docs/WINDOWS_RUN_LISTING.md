# Listado de registros Run y respaldos PCInsight

Revision48: [listado HKLM32/64 separado](WINDOWS_MACHINE_RUN_CONTROL.md) mediante
list_windows_machine_run_registrations(view). El listado descrito aqui sigue
siendo SOLO HKCU/shared, sin ampliar Monitor silenciosamente.

Unidad terminada: `list_windows_run_registrations()`, en pcinsight/startup.hpp.
Permite reconstruir el listado desde el registro, incluidos valores retirados
del Run ordinario y conservados en el almacén PCInsight. No necesita IDs guardados
por el proceso anterior ni activa acciones de escritura.

```cpp
auto listing = pcinsight::list_windows_run_registrations();
if (listing.value) {
    for (const auto& row : *listing.value) {
        // entry_id, state; command/native_type/native_data si no hay conflicto.
        // Mostrar procedencia y listing.support (estimated).
    }
}
// Sin valor: mostrar error/ausencia del proveedor, no una lista vacía ficticia.
```

## Semántica y alcance

Solo HKCU Run del contexto actual y HKCU Software/PCInsight/StartupDisabled/v1/Run.
RegOpenCurrentUser(KEY_READ), RegOpenKeyExW(KEY_QUERY_VALUE), RegEnumValueW y cierre
RAII. Sin KTM, creación de claves, set/delete, ejecución, elevación, utilidades
externas ni consultas de aprobación Windows. Linux devuelve unsupported.

Una fila por nombre de valor incluido en alguna ubicación:

- registered: solo Run; no significa enabled por Windows ni ejecución garantizada.
- disabled_by_pcinsight: solo almacén PCInsight; no prueba autenticidad/autoría.
- conflict: ambas ubicaciones; no sobrescribe ni elige un comando para restaurar.

No emite filas absent: un ID que desaparezca se puede reconsultar con el lector
individual. La comparación ordinal Unicode ignora mayúsculas con CompareStringOrdinal;
por ejemplo Same y sAME se agrupan como conflicto, no dos aplicaciones.
Usa para el ID el nombre reportado por Run cuando existe, o el del respaldo.
Codificación de ID compatible winrun:hkcu:shared:HEX_UTF8_MAYÚSCULAS. Orden ordinal
case-insensitive, no orden de ejecución, nombre comercial ni orden localizado GUI.

En filas registered/disabled conserva comando UTF-8 sin expandir, tipo nativo y
bytes exactos de origen (incluido terminador si existía). En conflict deja esos
campos vacíos/no seleccionados; no oculta la existencia del conflicto. En el Run
ordinario ignora valores sin nombre, comandos vacíos y tipos no textuales, como
el inventario actual. En respaldos esos casos son errores, no descartes silenciosos.
Texto malformado/tipo de respaldo no soportado rechaza el listado completo.

## Consistencia y límites

Enumera Run/respaldo dos veces (cuatro recorridos), compara mapas por nombre
exacto/tipo/bytes y solo entonces devuelve Support::estimated. La comparación
detecta incluso cambios de case entre pasadas. No snapshot atómico, protección
ABA ni garantía de ausencia de cambios posteriores/intermedios. La comparación
no certifica estabilidad de valores ignorados ni completitud de todos los autoruns.
No anuncia que la acción transaccional esté disponible solo porque el listado funciona.

Máximo10000 valores visitados por clave/pasada, incluidos ignorados; nombre hasta
16383 wchar_t, datos hasta65536B. Acumulado raw16MiB para los cuatro recorridos
(nombres UTF-16 y datos, también ignorados) y16MiB de valores de salida (IDs,
comandos y bytes nativos). Unión como máximo20000 filas. UTF-16/UTF-8 estricto,
sin NUL intermedio. Duplicados/case-equivalentes dentro de una pasada, cambios,
ERROR_MORE_DATA, permisos o exceso de límites dejan el Metric sin valor, no
resultado parcial. Se preserva permission_required y diagnóstico nativo cuando
aplica. Clave inexistente es vacía; fallo de consulta no lo es. No hay deadline
duro sobre APIs del registro.

Los datos pueden contener rutas/argumentos sensibles. El almacén PCInsight es
configuración editable por el usuario, no firma ni frontera de seguridad. El GUI
debe volver a leer antes de actuar y no presentar el texto como un comando autorizado.
Listado a demanda, también integrado como informe Monitor/Sampler con opt-in
defaultfalse (WINDOWS_RUN_MONITOR.md), evitando escaneos en cada tick por defecto.
Auditoría de la acción explícita ahora disponible por decorador opt-in
(WINDOWS_RUN_AUDIT.md), sin cambiar disponibilidad de escritura. API0.x ampliada:
recompilar consumidores contra la instalación actualizada.

## Verificación

2026-10-15: Windows74/74 y Linux54/54 Release; instalación y consumidores externos
reconstruidos/ejecutados exit0 en ambos. Consumidor Windows verifica listado estimado
y scopes HKCU sin mutación; Linux verifica unsupported.

Prueba nativa con claves privadas fuera de Startup: valores Run, respaldo Unicode
sin Run y conflicto Same/sAME;3 filas correctas, dos invocaciones independientes,
sin IDs previos ni estado en memoria del proveedor. No se reinició Windows ni se
ejecutó otro proceso para afirmar una prueba de reboot. El listado real HKCU
reportó16 filas de solo lectura. Se eliminaron únicamente los valores/claves de
la fixture propia, por nombres exactos, sin borrado recursivo ni tocar Startup real.

Proveedor inyectado sin callbacks KTM/escritura: estados, duplicados por case,
cambio de case entre pasadas, errores tardíos/permisos/MORE_DATA, campos inválidos,
tipos ignorados vs respaldo inválido, lista vacía válida, límites10000 y16MiB,
API incompleta y cierre de handles. Esto no resuelve la validación nativa positiva
de retirada/restauración (6801), ni implementa otras fuentes de Startup.

Fuentes oficiales:
[RegEnumValueW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regenumvaluew),
[CompareStringOrdinal](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-comparestringordinal).
