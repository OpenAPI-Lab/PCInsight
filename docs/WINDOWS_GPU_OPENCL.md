# OpenCL Windows por LUID DXGI

Proveedor de CL_DEVICE_VERSION por GPU mediante cl_khr_device_uuid. Reutiliza
el núcleo de inventario/validación de LINUX_GPU_OPENCL.md, pero la identidad
Windows es LUID, no PCI. `Monitor` lo ejecuta solo con include_gpus=true.

Se carga OpenCL.dll de System32, se resuelven tres entrypoints y se enumeran
todas las plataformas y GPU raíz. No se crea contexto, cola, subdevice ni kernel.
Sin SDK/header/enlace OpenCL obligatorio ni herramientas externas. La tabla ABI
declara CL_API_CALL (__stdcall en Windows), enteros fijos y handles opacos
prestados. Se ha probado x64; no se certifica ejecución física de Windows32.
DLL por RAII, sin conservar punteros al descargarla. No se cambia configuración.

## Identidad

Solo si el dispositivo anuncia el token exacto cl_khr_device_uuid se consulta
CL_DEVICE_LUID_VALID_KHR (cl_bool de32 bits). Debe ser CL_TRUE; si es CL_FALSE,
LUID y máscara son indefinidos y no se consultan. Otros valores son inválidos.
Se leen exactamente ocho bytes LUID y cuatro de CL_DEVICE_NODE_MASK_KHR. La
máscara debe tener un bit. El LUID se convierte al mismo ID dxgi-luid:high:low
del inventario DXGI. Mitad alta con bit de signo y ceros se conservan.

No match por modelo/vendor/orden ni PCI incompleto. Un LUID repetido entre
dispositivos, ICDs o entradas destino es ambiguo y no recibe versión, aunque
las versiones coincidan. GPU ajenas no se asignan. El valor es la cadena
CL_DEVICE_VERSION del dispositivo, no plataforma, OpenCL C ni certificación
de todas las funciones opcionales de OpenCL3.0.

## Fallos y límites

El núcleo conserva límites Linux:128 plataformas,256 GPU por plataforma,4096
totales, tres intentos de conteo creciente, extensiones64KiB y versión1KiB.
Propietario/tipo, tamaños exactos, terminación NUL, tokens y formato de versión
comprobados. Inventario/identidad inválidos descartan toda la muestra OpenCL;
un fallo de versión afecta solo a la GPU ya identificada. Otros proveedores
GPU y el inventario DXGI siguen disponibles. Datos ausentes no significan que
el hardware no soporte OpenCL. Denegación Win32 al cargar DLL ->permission_required;
no se inventa esa equivalencia para todos los errores OpenCL.

Sin extensión o LUID válido esta vía deja valor ausente. Actualización:
WINDOWS_OPENCL_INTEROP.md añade asociación D3D11 uno-a-uno como estimated,
verificada en AMD real. No equivale a identidad LUID ni reemplaza un valor
existente; no se instala software ni se copia versión global como fallback.
No snapshot atómico entre APIs,
deadline impuesto a driver bloqueado ni promesa de coste cero al cargarlo.

## Verificación

Windows Release52/52 y Linux39/39; paquetes/consumidores instalados ejecutados.
API inyectada: dos plataformas con versiones distintas, LUID signed high,
duplicados origen/destino, API ausente, permisos/errores de consultas tratados
sin valor, conteos, crecimiento, handles/owner/tipo, texto/NUL, tokens parecidos,
LUID falso/inválido, tamaños, máscara cero/multibit y error de versión aislado.
Monitor off/on/off y consulta nativa. El núcleo compartido conserva las pruebas
Linux por PCI y loader fixture.

En RX7900XT Windows, el dispositivo OpenCL nativo **no anunció
cl_khr_device_uuid**; por ello no se obtuvo versión por esta vía. Diagnóstico
nativo de solo lectura registrado por la prueba, no inferido por modelo. No se
certifica el camino positivo con hardware real, otros vendors, multigpu ni RDP.
Las fixtures prueban el camino positivo; no sustituyen esa validación.

Fuente: [CL_DEVICE_VERSION, LUID válido y correspondencia con DXGI](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetDeviceInfo.html).
