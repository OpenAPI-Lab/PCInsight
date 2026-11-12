# OpenCL Windows: asociación alternativa D3D11

Para GPU sin un LUID OpenCL válido, `Monitor` intenta una asociación mediante
`cl_khr_d3d11_sharing`. El campo sigue siendo `GpuInfo.opencl_version`, pero
su soporte es **estimated**: identifica la versión del dispositivo OpenCL
asociado por interoperabilidad, no prueba identidad física como la vía UUID/LUID.
No se reemplaza ningún valor existente. La vía LUID sigue siendo preferida.

## Por qué estimated

`clGetDeviceIDsFromD3D11KHR` con CL_D3D11_DXGI_ADAPTER_KHR acepta un IDXGIAdapter
y devuelve dispositivos OpenCL que pueden interoperar. CL_ALL_DEVICES_FOR_D3D11_KHR
puede incluir dispositivos no preferidos con peor rendimiento. Por eso no basta
con elegir el primero, ni con tratar interop como prueba del mismo chip físico.
Se acepta únicamente una relación uno-a-uno observada: un dispositivo para ese
adaptador y ese mismo dispositivo no asociado a ningún otro adaptador DXGI.
Aun con ambas comprobaciones se conserva estimated, sin afirmar LUID físico.

## Consulta y controles

Se enumeran todas las plataformas y GPU raíz; se comprueban tipo, propietario
y token exacto cl_khr_d3d11_sharing de cada dispositivo. La función de extensión
se obtiene por plataforma mediante clGetExtensionFunctionAddressForPlatform.
Se consulta CL_ALL_DEVICES_FOR_D3D11_KHR sobre el adaptador explícito, no default,
sin crear dispositivo D3D11, contexto/cola OpenCL, superficies, buffers o kernels.
No nueva DLL, enlace, SDK, instalador ni subprocess: usa OpenCL.dll de System32
ya abierta y las APIs DXGI existentes.

Factory DXGI fresca; adaptadores COM por RAII. Se revalida LUID/flags tras las
consultas y se exige igualdad de topología hardware con el inventario recibido.
Software queda fuera. Inventarios nulos, duplicados, inconsistentes o fallidos
descartan el fallback sin borrar valores previos. La función de extensión ausente
en una plataforma que anuncia soporte también lo invalida: no se puede excluir
una asociación adicional oculta. Un resultado debe ser una GPU raíz de esa
misma plataforma que haya anunciado la extensión. Un dispositivo desconocido,
duplicado o CPU no se acepta. Varias GPU/ICDs para un adaptador quedan ambiguos.

Límites:128 plataformas,256 GPU por plataforma,4096 raíces totales,256 adaptadores
DXGI y256 resultados por consulta interop. Conteos que crecen se reintentan hasta
tres veces; errores y buffers fuera de límite no se publican como listas completas.
CL_DEVICE_NOT_FOUND es una lista vacía, no una versión. Después de validar todo
se consulta CL_DEVICE_VERSION con límites/formato ya compartidos con Linux.
Fallo de versión aislado no borra el de otro adaptador. No caché permanente,
worker, snapshot atómico frente a hotplug ni timeout sobre driver bloqueado.

## Verificación

Windows Release53/53 y Linux39/39; paquetes/consumidores instalados ejecutados.
Pruebas inyectadas con dos plataformas y adaptadores: correspondencia única,
uno-a-muchos/muchos-a-uno, dispositivos desconocidos/repetidos, límites y conteos
crecientes, extensión/token/entrypoint ausentes, error de enumeración tras datos
válidos, LUID cambiado, software/topología contradictoria, factory fallida/nula,
adapter nulo/duplicado, error de versión aislado, preservación del valor LUID y
cierre COM incluso con handles devueltos en errores.

**Camino positivo nativo AMD RX7900XT:** OpenCL 2.0 AMD-APP (3679.0), marcado
estimated por asociación D3D11. El mismo driver no anuncia cl_khr_device_uuid;
por esa vía el dato continúa ausente. La prueba de Monitor acepta únicamente
available con nota LUID o estimated con nota interop, no los equipara.
Hardware multigpu/Intel/NVIDIA/RDP, hotplug real y camino positivo UUID/LUID
siguen pendientes. No implica compatibilidad universal de todos los drivers.

Fuente: [API de asociación, tipo DXGI y alcance del conjunto ALL](https://registry.khronos.org/OpenCL/specs/unified/refpages/man/html/clGetDeviceIDsFromD3D11KHR.html).
