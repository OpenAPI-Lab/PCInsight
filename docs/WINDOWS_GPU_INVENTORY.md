# Inventario nativo de GPU Windows

Actualizacion revision54: [NVML memoria Windows](WINDOWS_NVML_MEMORY.md) agrega
en Monitor una fuente independiente de bytes framebuffer sin asignar por GPU
NVIDIA inequívoca. No cambia el inventario estatico ni deduce free de DXGI/PDH.
Tiene soporte condicionado a DLLs del driver e identidades/KMT; no positivo
fisico NVIDIA aun, no cierre M04 ni soporte global AMD/Intel.

Actualizacion revision52: [WGL opt-in](WINDOWS_CURRENT_OPENGL.md) completa
opengl_version del contexto desktop actual del llamador, solo por LUID unico
de adaptador DXGI. No crea contexto ni prueba version maxima/por nodo fisico.
Los pendientes de OpenGL de la unidad historica siguiente no describen esta
capacidad nueva y limitada; M04 no se cierra por ella.

Monitor.sample con include_gpus=true enumera adaptadores DXGI1.1 no-software,
sin WMI, comandos externos, componentes instalados adicionales, creación de
dispositivo D3D ni workload gráfico. include_gpus=false no abre una factory.
Se enlaza dxgi, biblioteca del propio Windows (exportada también al consumidor
del paquete CMake). No cambia COM apartment, afinidad, driver ni configuración.

## Datos de esta unidad

- ID dxgi-luid:high:low, ambas mitades hex de32 bits; identidad del adaptador
  de esta sesión/host, NO ID PCI físico o identificador persistente entre boots.
- Model: Description UTF-16 convertido a UTF-8 con límites/Unicode válido.
- Vendor y hardware_vendor_id/hardware_device_id: IDs PCI/ACPI de GetDesc1.
  Nombres AMD/Intel/NVIDIA para IDs conocidos; otro ID se expone como ID, no
  marca inventada. VendorId0 deja nombre vacío; IDs0 se conservan como reportados,
  pueden significar no especificados en niveles antiguos de hardware.
- dedicated_memory_bytes: capacidad de video dedicada reportada por DXGI.
- shared_memory_bytes: máximo de memoria de sistema compartida utilizable por
  ese adaptador, NO cantidad actualmente usada, reservada o libre.
- dedicated_system_memory_bytes: memoria de sistema dedicada reservada al boot,
  aparte de la memoria de video dedicada; no se suman esas cantidades.
- driver_version: CheckInterfaceSupport con IDXGIDevice, versión UMD de64 bits
  presentada en cuatro componentes de16 bits; NO versión comercial AMD/NVIDIA,
  fecha, versión de DirectX ni feature level. Consulta denegada -> permission_required;
  fallida/unsupported -> valor ausente. No devuelve versión cero ante errores.

Cada capacidad es valor disponible reportado por el driver, también si es0 en
un adaptador UMA. No implica capacidad nominal anunciada/BIOS ni VRAM enteramente
libre: el driver puede reportar una cantidad menor por sus propias reservas.
No sumar shared_memory_bytes de varias GPUs como si fuera RAM física separada.
La fecha del driver y ubicación física quedan desconocidas, no se infieren del
LUID/device ID. No se convierten capacidades en uso dinámico ni porcentaje.

Esta unidad deja utilization, engines, temperature, memory_used y memory_available
sin valor, con razones explícitas. Vulkan/OpenCL/OpenGL tampoco se consultan aquí.
Sigue pendiente proveedor de motores/contabilidad global de memoria, ubicación,
driver date, temperatura por vendor y capabilities de runtimes gráficos.

Actualización: WINDOWS_GPU_MEMORY.md añade uso dinámico dedicated/shared por
adaptador en Monitor, sin convertir capacidades ni usar consumo del caller como
global. La función interna read_windows_gpus conserva su alcance de inventario.

## Enumeración, errores y recursos

Actualización de capacidades: WINDOWS_GPU_D3D12.md añade el nivel Direct3D 12
validado mediante el mismo adaptador, sin crear dispositivo ni workload. No es
la versión de DirectX instalada. WINDOWS_GPU_DEVICES.md completa fecha/PCI en
Monitor; WINDOWS_GPU_ENGINES.md añade motores. Los pendientes anteriores
describen el alcance histórico de la primera unidad, no el estado combinado.

Factory nueva en cada muestra: el inventario estático de una factory no es una
vista válida después de hotplug. No se cachean adapters ni se usa índice de
enumeración como ID. Flags SOFTWARE excluyen WARP/Basic Render de la lista;
no se afirma que todos los restantes sean tarjetas PCI físicas: puede haber
adaptadores virtuales/remotos. Un inventario vacío correcto significa que DXGI
no enumeró adaptadores no-software, no que la máquina carezca de hardware gráfico.

Se exige S_OK y handle válido. Solo DXGI_ERROR_NOT_FOUND termina la lista.
Factory/EnumAdapters/GetDesc1 fallidos, LUID duplicado, Unicode inválido o más
de256 adaptadores lanzan system_error: no devuelven inventario vacío/parcial como
si fuera completo. Monitor.sample propaga ese error; el consumidor debe manejarlo.
Sampler conserva su contrato de reportar errores, no es necesario inventar cero.
El fallo opcional de driver_version no elimina un adaptador ya identificado.

Factory/adapters se liberan mediante RAII en éxito, fin de enumeración, errores
y excepciones de allocations. La tabla inyectable interna prueba cada camino
sin mocks enormes de interfaces COM. No hay callbacks públicos ni posesión de
referencias COM transferida al consumidor. No garantía de consistencia global
contra hotplug que ocurra DURANTE la muestra ni deadline de una API/driver lento.
Campos añadidos a GpuInfo0.x: reconstruir librería y consumidores, no mezclar ABI.
Linux permanece con sus proveedores previos; nuevos IDs/dedicated-system metric
de esta unidad quedan unsupported allí hasta implementar su fuente.

## Verificación

2026-10-14: Linux35/35 y Windows46/46 Release; paquetes y consumidores externos
reconstruidos/ejecutados en ambos sistemas, con dependencia dxgi y campos nuevos.
Pruebas inyectadas: discrete/UMA/capacidades/no used/free falsos, filtrado software,
vendor PCI/ACPI desconocido, LUID signed high, driver conocido/denegado/unsupported,
query exit correcto sin adaptadores, factory/null/enumeration/GetDesc1 fallidos,
duplicados, descripciones truncadas/Unicode inválido, límite y liberación de
todos los recursos. include_gpus off/on/off no conserva datos ocultos.

Equipo real x64: un adaptador AMD Radeon RX 7900 XT; DXGI reportó21413273600 bytes de
video dedicado,16972089344 de capacidad shared y UMD32.0.31041.1004. Se validó
enumeración directa y a través de Monitor sin crear dispositivo gráfico/carga.
Esto NO prueba otros drivers, hotplug físico, RDP/headless ni máquinas multigpu.
No cerrar GPU Windows completo por esta unidad de inventario.

Fuentes: [EnumAdapters1 y lifetime/hotplug](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory1-enumadapters1),
[capacidades y LUID de GetDesc1](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_adapter_desc1),
[versión UMD mediante IDXGIDevice](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiadapter-checkinterfacesupport).
