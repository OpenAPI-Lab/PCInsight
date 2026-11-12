# Capacidad Direct3D 12 por GPU

`GpuInfo.direct3d12_feature_level` expone el mayor nivel validado entre los
candidatos conocidos `12_2`, `12_1`, `12_0`, `11_1` y `11_0`.
No es la versión de DirectX instalada, versión de DLL, versión del driver,
certificación Ultimate ni una lista de todas las capacidades opcionales.
No se infiere por modelo, versión de Windows o existencia de un archivo.

## Proveedor

El inventario DXGI pasa su mismo `IDXGIAdapter1` a `D3D12CreateDevice`, con
`ppDevice=nullptr`: consulta de validación, sin obtener un dispositivo ni crear
colas, ventanas, comandos o workload gráfico. No se usa adaptador por defecto
ni fallback WARP. La función se resuelve en `d3d12.dll` cargada desde System32,
sin herramientas externas, instalación ni dependencia de enlace nueva.
La DLL se libera por RAII y no se retienen punteros al salir. No se activa debug
layer ni se cambia la configuración del consumidor. El runtime puede inicializar
su driver internamente; no se promete coste cero ni deadline para un driver.

Se examinan hasta cinco candidatos en orden descendente:

- `S_FALSE` es el éxito documentado para una consulta con salida nula.
  El nivel queda `available` si todos los superiores fueron rechazados como
  `DXGI_ERROR_UNSUPPORTED`. El alcance termina en 12_2, no futuros niveles.
- `E_INVALIDARG` puede corresponder a un nivel no reconocido por un runtime
  antiguo. Si un nivel inferior se valida, se devuelve como `estimated`, límite
  inferior certificado; no se presenta como máximo absoluto del hardware.
- Denegación de acceso: sin valor, `permission_required`. Device removed,
  otros errores o un inesperado `S_OK`: sin valor, `unsupported`; no se ocultan
  con un valor inferior. DLL/entrypoint ausentes tampoco producen una versión.

Cada muestra vuelve a consultar los adaptadores presentes. Fallos opcionales no
eliminan el resto del inventario. `include_gpus=false` no inicia este proveedor.
En Linux el campo permanece sin valor; no sustituye los proveedores Vulkan/OpenCL.
Los campos públicos 0.x requieren recompilar librería y consumidores juntos.

## Verificación

2026-10-14 local America/La_Paz: prueba Release `windows-gpu-d3d12` con cada
nivel, soporte ausente, permiso denegado, dispositivo retirado, error genérico,
éxito inválido, runtime que rechaza 12_2 y valida 12_1, API/adaptador nulos,
puntero de adaptador exacto y salida nula en cada llamada. Inventario inyectado
prueba dos GPU con resultado distinto y omite el software. Consulta nativa y
Monitor off/on/off probados. RX 7900 XT validó `12_2`.

Regresión Windows 50/50, Linux 35/35. Paquetes instalados y consumidores externos
reconstruidos/ejecutados en ambos. No validación física de runtime antiguo,
Intel/NVIDIA, RDP ni multigpu. Vulkan/OpenCL/OpenGL Windows siguen pendientes;
esta unidad no cierra toda la subtarea GPU.

Fuente: [D3D12CreateDevice: adaptador explícito, consulta sin dispositivo y S_FALSE](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-d3d12createdevice).
