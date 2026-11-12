# Contribuir a PCInsight

Este repositorio contiene la biblioteca/SDK, sus ejemplos, pruebas y documentación.
No necesitas clonar ni cambiar el GUI o el proyecto principal que usa la API.
Empieza por [el estado de soporte](docs/SUPPORT_STATUS.md) y
[cómo usarla desde C++](docs/GETTING_STARTED_CPP.md).

## Preparar y probar un cambio

Desde la raíz del repositorio, usando un directorio de build nuevo:

```sh
cmake -S . -B build/contributor -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON -DPCINSIGHT_BUILD_TESTS=ON -DPCINSIGHT_BUILD_EXAMPLES=ON
cmake --build build/contributor --parallel 2
ctest --test-dir build/contributor --output-on-failure --no-tests=error --timeout 60
```

Para Clang, añade `-DCMAKE_CXX_COMPILER=clang++` a la configuración inicial
en otra carpeta. Las pruebas deben conservar las comprobaciones en Release.
Los tests usan fixtures y algunos crean archivos, sockets o hijos privados.
No ejecutes sondas manuales privilegiadas, scripts de servicio, cambios de
startup real ni cierres de sesión como si fueran parte obligatoria de contribuir.
Un test omitido por permisos o capacidad no es una validación positiva.

Para probar también la instalación y el enlace externo:

```sh
bash scripts/verify-linux-sdk.sh build/contributor work/verification-new
```

La carpeta de evidencia debe ser nueva y su padre existir (`mkdir -p work`
si hace falta). No requiere `sudo`; conserva logs privados antes de compartirlos.
El verificador prueba el SDK instalado y no obliga a tu GUI a utilizar CMake.

## Reglas del proyecto

- Linux primero. Recopilar mediante interfaces del kernel, firmware o drivers; no añadir comandos externos para obtener datos. Las acciones explícitas son otra superficie y necesitan un diseño de permisos aparte.
- No confundir desconocido con cero, `false` o lista vacía. Mantener `Metric`, unidades, soporte y alcance; no convertir proxies en mediciones exactas.
- Una captura no es atómica por ser una estructura C++. Cuidar reutilización de PID/índice, pérdida de continuidad, permisos, overflow y desaparición de recursos.
- Añadir pruebas de éxito, ausencia y errores relevantes. Preferir fixtures privados para casos que no se puedan reproducir con hardware disponible. No afirmar cobertura física por un fixture.
- Documentar cambios de tipos públicos y semántica. Agregar un miembro también puede romper ABI. Recompilar biblioteca y consumidores; revisar [política de versiones](docs/SDK_VERSION_POLICY.md).
- No cambiar una línea base solo para que un test pase: medir el perfil, revisar la diferencia y documentarla. No declarar compatibilidad universal por una prueba local.

## Qué incluir en un issue o pull request

Describe el problema, entorno, comportamiento observado/esperado, API afectada,
prueba añadida y límites que siguen abiertos. Incluye versión/commit, compilador,
arquitectura y si ejecutaste en Linux nativo, VM o WSL. Para hardware usa la
[guía específica](docs/LINUX_HARDWARE_EVIDENCE.md); un solo dispositivo o campo
validado es útil. Separa medición observada de una conclusión sobre todo el equipo.

No publiques números de serie, credenciales, nombres de usuario/rutas privadas,
IPs o MACs no necesarias, comandos de aplicaciones, dumps de firmware ni logs
completos sin revisarlos. Para un posible fallo de seguridad, consulta
[SECURITY.md](SECURITY.md) antes de abrir un reporte público.

PCInsight usa [MIT](LICENSE). Conserva los avisos de copyright y licencia y
declara cualquier material de terceros que propongas incorporar. La licencia
de PCInsight no establece la licencia de otros proyectos que lo consuman.
