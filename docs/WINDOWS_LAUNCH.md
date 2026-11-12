# Windows: Run new task

Implementado 2026-10-13 mediante `SystemActions::launch_process` y
`CreateProcessW`. Es una acción explícita, separada del muestreo. No requiere
programas externos ni abre una shell implícitamente.

```cpp
auto actions = pcinsight::make_system_actions();
pcinsight::LaunchRequest request;
request.executable = R"(C:\Program Files\Example\app.exe)";
request.arguments = {"--name", "texto con espacios", ""};
request.working_directory = R"(C:\Work)";
auto result = actions->launch_process(request);
// Consultar result.error y result.message; no asumir que la app ya está lista.
```

## Contrato

- Entradas UTF-8 estrictas, sin NUL embebido. Ruta ejecutable absoluta de unidad
  o UNC convencional. No se busca en PATH ni en App Paths; no se expanden variables.
  Rutas relativas, entrecomilladas por el consumidor o namespaces de dispositivo
  (`\\?\`, `\\.\`) no se admiten en esta versión.
- `working_directory` vacío hereda el directorio del llamador. Si se especifica,
  debe ser absoluto. Entorno y contexto de seguridad provienen del proceso llamador.
  No se cambia usuario/sesión y no se reduce una elevación que el padre ya tenga.
- Los argumentos se codifican individualmente con reglas del runtime C de Microsoft,
  incluidos vacíos, comillas y barras finales. Windows entrega una línea de texto,
  no un vector argv; programas con parsers propios pueden interpretarla distinto.
  Máximo 32766 unidades UTF-16 más NUL en la línea completa.
- `lpApplicationName` siempre está separado y explícito; no se delega su selección
  al primer fragmento de la línea. No se habilita herencia general de handles ni se
  redirigen stdin/stdout/stderr. La consola/interfaz siguen el comportamiento normal
  del ejecutable y de Windows.
- No hay ShellExecute, asociaciones de archivos ni intérprete para scripts/batch.
  El consumidor puede elegir explícitamente un intérprete, pero entonces debe tratar
  sus argumentos como código potencial; esta API **no es una sandbox**.
- `elevated=true` devuelve `permission_denied` sin lanzar nada: falta el helper UAC.
  También se distinguen acceso denegado/elevación requerida, archivo/ruta inexistente,
  parámetros/formato inválidos y otros errores Win32.
- Éxito significa que Windows creó el proceso, no que terminó de inicializarse ni
  que permanecerá ejecutándose. `message` incluye PID informativo, no una identidad
  durable ni un handle para acciones posteriores. Los dos handles nativos se cierran
  mediante RAII; no se espera al hijo y cerrar handles no termina el proceso.

El consumidor debe autorizar la acción y confiar en el ejecutable seleccionado.
La validación de ruta no verifica firma/editor ni impide reemplazos concurrentes
del archivo. No se añaden privilegios, búsqueda de ejecutables ni políticas de
autorización a partir del texto de los argumentos.

## Verificación y pendientes

Actualización2026-10-19 revision37: prueba nativa positiva de lanzamiento del
propio ejecutable con hijo privado, confirmación de argumentos por evento y
limpieza checked;20/20 repeticiones. Ver [WINDOWS_SERVICE_VIEW.md](WINDOWS_SERVICE_VIEW.md).
No valida parsers arbitrarios, UAC ni todos los toolchains. Las pruebas de abajo
son la evidencia inicial2026-10-13, no el estado actual de integración.

Pruebas con CreateProcessW y CloseHandle inyectados: parámetros de seguridad,
buffer modificable, cierre de handles, permisos/errores, Unicode, entradas inválidas,
límite exacto de longitud y rechazo de elevación. Casos de escape explícitos y
300 grupos deterministas de argumentos se contrastan con CommandLineToArgvW
(solo parser, no ejecución). No se lanzó una aplicación real en estas pruebas.

Windows Release 10/10 y Linux WSL 5/5. Pendientes: integración con ejecutable
desechable en entorno aislado, validación MSVC/otras arquitecturas, helper UAC,
efficiency mode. Terminación forzada implementada en WINDOWS_TERMINATE.md;
el cierre cooperativo sigue pendiente. Este bloque no cierra la subtarea de procesos.

Referencias: [CreateProcessW](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw)
y [reglas de argumentos C](https://learn.microsoft.com/en-us/cpp/c-language/parsing-c-command-line-arguments).
