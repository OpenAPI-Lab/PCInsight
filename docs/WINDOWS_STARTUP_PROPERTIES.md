# Properties de registros Run Windows

`get_startup_properties(entry.id)` permite construir el panel Properties del GUI
para entradas `winrun:...`. No abre diálogos del sistema, ejecuta comandos,
expande variables, consulta binarios ni modifica el registro. Funciona con
Advapi32 del sistema, sin utilidades externas. Las propiedades de archivos de
carpetas Startup (`winfolder:`) tienen ahora una unidad separada:
[Properties del archivo fuente](WINDOWS_STARTUP_FILE_PROPERTIES.md). No confundir
campos de registro con campos de archivo ni tamaño del acceso directo con destino.

```cpp
#include <pcinsight/startup.hpp>
auto result = pcinsight::get_startup_properties(entry.id);
if (result) {
    // entry: nombre, comando literal, fuente; status unknown e impacto not_measured
    // fields: hive, view, key, value_name, value_type, command_unexpanded
    // source_file_size_bytes: sin valor, porque el registro no es un archivo
}
// stale_target: recargar el inventario y Properties, no ejecutar una alternativa.
```

## Selección y datos

Acepta únicamente los scopes emitidos por el inventario: hkcu:shared, hklm:64 y
hklm:32. El sufijo es hex canónico en mayúsculas del nombre UTF-8, no una ruta
de registro arbitraria. Decodificación estricta UTF-8, sin NUL, nombre hasta16383
wchar_t. Identificador hasta131128 bytes para admitir nombres Unicode codificados
en hex; Linux conserva su máximo32KiB. No acepta RunOnce, otra clave, otra cuenta
ni un ID de carpeta. HKCU se consulta en el contexto Windows del caller; el ID
no incorpora SID ni sirve de token persistente entre cuentas/impersonaciones.

Abre solo `Software\Microsoft\Windows\CurrentVersion\Run` con KEY_QUERY_VALUE
y vista WOW64 explícita en HKLM AMD64. En x86 nativo admite la vista32 sin flags;
vista64 no existe. ARM64 queda not_supported, coherente con el inventario actual,
sin afirmar cobertura de sus vistas adicionales.

Devuelve un StartupEntry recién consultado, no una copia persistida del inventario.
Los campos del grupo `Windows Run` son UTF-8: hive, view, key, value_name,
value_type y command_unexpanded. REG_SZ/REG_EXPAND_SZ conservan el comando literal;
no hay interpretación shell, PATH ni sustitución de porcentajes. Registro no
equivale a aprobación de inicio: estado unknown, impacto not_measured y editor/
firma/metadatos de accesos directos ausentes. Esos opt-ins de Monitor son separados.

`source_file_size_bytes` es Metric sin valor para registro, no tamaño0 conocido.
En Linux se llena con el tamaño del .desktop; `file_size_bytes` se conserva como
campo legacy Linux-only. Consumidores nuevos deben usar el Metric. API0.x
ampliada: recompilar contra la biblioteca y headers instalados.

## Errores y consistencia

Dos lecturas del mismo valor mediante el mismo handle abierto, buffer fijo64KiB
por lectura. Comprueba tipo, longitud y bytes iguales; no hace reintentos sin
límite. REG_SZ sin terminador se acepta dentro del tamaño declarado; si existe
un terminador final se elimina solo uno. Bytes impares/excesivos, NUL intermedio,
UTF-16 inválido o ID malformado -> invalid_argument. Otros tipos -> not_supported;
comando vacío -> not_found (no registro de aplicación inventariado).

ACCESS_DENIED -> permission_denied; clave/valor ausente inicialmente -> not_found;
crecimiento/ERROR_MORE_DATA, desaparición en segunda lectura o cambio de bytes/
tipo/tamaño -> stale_target. Otros errores nativos -> system_error con código
Win32. No hay resultado parcial. Handle liberado por RAII también en errores.

Dos observaciones coincidentes no son una transacción: no detectan cambios ABA,
renombrados equivalentes por case, recreación de clave bajo el mismo nombre ni
mutaciones posteriores. El ID identifica nombre/vista, no la instancia histórica
del valor ni del ejecutable. No autoriza habilitar/deshabilitar/ejecutar. La consulta
puede mostrar un comando nuevo si cambió antes de ambas lecturas. Sin deadline
duro de API; no implica inventario completo de todas las fuentes de inicio.

## Verificación

2026-10-15: Release Windows71/71 y Linux54/54, paquetes instalados y consumidores
externos reconstruidos/ejecutados en ambos, exit0. Windows nativo observó3
registros Run reales mediante consulta pública, sin cambios al registro.
Pruebas inyectadas de vistas HKCU/HKLM32/64/x86/ARM64, Unicode, datos sin terminador,
variables literales, NUL/UTF-16/tamaños inválidos, límite64KiB, cambios y desaparición,
errores/permisos y cierre. Linux verifica el nuevo Metric de tamaño y conserva las
regresiones de propiedades XDG. MSVC/otros hosts requieren validación adicional.

Fuentes oficiales:
[RegQueryValueExW y strings sin terminador garantizado](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regqueryvalueexw),
[RegOpenKeyExW](https://learn.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeyexw),
[vistas de registro WOW64](https://learn.microsoft.com/en-us/windows/win32/winprog64/registry-redirector).
