# Carga NVML Windows: System32 y ubicacion legacy

Revision55, bloque M04 terminado. La [fuente de memoria NVML](WINDOWS_NVML_MEMORY.md)
ya no exige que nvml.dll este exclusivamente en System32. NVIDIA documenta
System32 para DCH y una ubicacion NVSMI bajo Program Files para instalaciones
estandar ([referencia NVIDIA](https://docs.nvidia.com/deploy/nvml-api/nvml-api-reference.html)).
Esto amplia las ubicaciones admitidas, no la cobertura fisica certificada ni
los simbolos/drivers compatibles.

## Politica de busqueda

1. Intenta nvml.dll con LOAD_LIBRARY_SEARCH_SYSTEM32. Un exito gana siempre:
   no resuelve Known Folders ni abre otra copia.
2. Solo MOD_NOT_FOUND, DLL_NOT_FOUND, FILE_NOT_FOUND y PATH_NOT_FOUND habilitan
   el segundo intento. ACCESS_DENIED, BAD_EXE_FORMAT y cualquier otro error
   detienen la busqueda; no se ocultan con otra copia instalada.
3. Windows resuelve FOLDERID_ProgramFilesX64 mediante SHGetKnownFolderPath con
   DONT_VERIFY, sin crear carpetas. No se usa ProgramFiles/ProgramW6432 del
   entorno, ni se supone C:\\Program Files. La cadena recibida se libera con
   CoTaskMemFree tambien ante excepcion.
4. Se exige una ruta de unidad absoluta ordinaria, acotada a32767 unidades
   wchar incluyendo sufijo/NUL. No se aceptan UNC/device namespaces, rutas
   relativas, raiz de unidad, traversal, componentes vacios, ADS, controles/NUL,
   slash, comodines, expansion porcentual o componentes con punto/espacio final.
   Un separador final ordinario se normaliza. Rutas Unicode y otra unidad son
   posibles; configuraciones de Program Files fuera de este contrato conservador
   quedan desconocidas, no se adivinan.
5. Se agrega unicamente NVIDIA Corporation\\NVSMI\\nvml.dll. Esa ruta absoluta
   se carga con DLL_LOAD_DIR | SYSTEM32, sin cambiar PATH/cwd ni directorios DLL
   del proceso. No se escanean discos, DriverStore, aplicaciones, toolkits ni
   carpetas de usuario; no hay instalacion o descarga.

La [API de carga Windows](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw)
puede fallar por una dependencia ausente, no solo por el archivo principal:
MOD_NOT_FOUND habilita el intento legacy pero no diagnostica cual archivo falta.
La politica controla las rutas solicitadas; no certifica firma/ACL/reparse
points ni neutraliza modulos que el host ya hubiera cargado. Se confia en la
configuracion de Known Folders y el loader del sistema. No se prometen rutas
de origen autenticadas por verificar solamente estos flags.

## Integracion y errores

Solo nvml.dll tiene este fallback. nvcuda.dll y gdi32.dll siguen con busqueda
System32. Sin CUDA disponible no se intenta NVML desde el recolector; el test
del loader puede probar sus ubicaciones separadamente, sin inicializar NVML.
Si encuentra una DLL sin simbolos requeridos no busca otra: el proveedor queda
desconocido. El resto del contrato de identidad CUDA/NVML/KMT y memoria v1 no
cambia. La alternativa NO hace compatibles drivers anteriores sin UUIDv2.

El resultado interno devuelve handle propio, error Win32 y motivo/origen de
busqueda. El caller libera la referencia con FreeLibrary mediante RAII. Se
preserva inmediatamente el error de cada LoadLibraryEx antes de otra API;
un fallo incoherente con error0 se normaliza a GEN_FAILURE. Resolver Known
Folder denegado y carga denegada propagan permission_required en la metrica.
La nota del recolector incluye el diagnostico Win32 del loader. La ayuda de
errores ya no agrega un status0 ficticio cuando no recibe un codigo.
No hay cambios de API/layout/vtable/version ni formatos de persistencia.

## Evidencia y pendientes

Windows MinGW130/130 y MSVC130/130, Linux111/111 Release. Veinte repeticiones
de loader y memoria por toolchain Windows (40 ejecuciones de tests por cada
una). Tres consumidores externos instalados reconstruidos/relinkados/exit0,
sin nuevos enlaces ni cabeceras publicas. `work/nvml-legacy-final-*`,
`nvml-legacy-repeat-*`, `nvml-legacy-install-*` y `nvml-legacy-consumer-*`.
El primer build MinGW detecto tipos de constantes DWORD/initializer_list y un
narrowing; corregidos con tipos explicitos, registro inicial conservado.
MSVC conserva D9025 esperado de /UNDEBUG tests; no se desactivan advertencias.

Prueba inyectada: prioridad System32, legacy positivo/ruta/flags, ausencia de
ambos, errores/denegaciones sin bypass, Known Folder fallido, path malformado/
traversal/UNC/ADS/controles/NUL/limite, Unicode/unidad distinta/separador final,
API faltante y error0. Prueba de memoria previa conserva sus comprobaciones.
Probe nativo en host AMD: System32 no encontro NVML y el intento legacy tambien
fallo con Win32=126. No se escriben DLLs en System32/Program Files ni se invoca
nvmlInit/CUDA en el probe de ubicacion. Un exito inyectado no prueba un driver
NVIDIA real ni sus dependencias.

Sigue faltando positivo fisico NVIDIA, matriz drivers/topologias y free AMD/Intel.
M04 permanece abierta; M05 sigue aplazada. Linux no cambia de proveedor ni de
semantica. La [identidad Known Folder](https://learn.microsoft.com/en-us/windows/win32/shell/knownfolderid)
no es una certificacion de permisos/seguridad del contenido de una carpeta.
