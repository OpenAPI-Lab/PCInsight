# Temperatura GPU Windows por KMT

`Monitor.sample(include_gpus=true)` consulta el sensor principal del adaptador
mediante D3DKMTQueryAdapterInfo y ADAPTERPERFDATA. `temperature_celsius` contiene
la lectura reportada en Celsius (entero nativo de décimas /10.0), no temperatura
hotspot, VRAM, CPU ni un valor inferido de carga/ventilador. Cada muestra vuelve
a consultar; no se conserva una temperatura antigua ante errores.

## Identidad y ABI

Abre el LUID exacto del inventario DXGI con D3DKMTOpenAdapterFromLuid. No usa
nombre, vendor ni orden. Comprueba que se mantenga el LUID de entrada y exige
un único adaptador físico con PHYSICALADAPTERCOUNT antes y después de la lectura.
Si hay varios adaptadores físicos enlazados no elige arbitrariamente el índice0
como temperatura general; el campo queda ausente. Direcciones DXGI inválidas o
duplicadas, incluidos IDs equivalentes en distinta capitalización, no se asignan.

Subconjunto ABI documentado con assertions de tamaño/offset/alineación: handle
KMT de32 bits, open-LUID de12bytes, perfdata de64bytes y query de24bytes en x64.
Los tipos numéricos30 (physical count) y62 (perfdata) se verificaron en el header
publicado por Microsoft: no se deducen contando líneas de la página resumida.
No SDK/DDK ni librerías nuevas requeridos. Gdi32 se carga desde System32 y sus
tres funciones se resuelven dinámicamente. Este proveedor nativo se limita por
ahora a callers de64 bits; 32 bits informa ausencia en vez de arriesgar otro ABI.

Se consulta solo índice físico0 después de comprobar count1. Tamaños, punteros
y campos de entrada no deben cambiar. La temperatura se inicializa con un
sentinel fuera de rango para detectar éxito sin salida. Se valida0..255°C;
0 devuelto correctamente sigue siendo un valor, no ausencia inferida del cero.
La métrica describe lo reportado por el driver; no certifica precisión/calibración
del sensor ni sustituye una comprobación física externa.

## Fallos y recursos

Faltan API/soporte, query fallida, rango/formato inválido, count0 o varios,
cambio de count: valor ausente con motivo. STATUS_ACCESS_DENIED distingue
permission_required; no se eleva ni lanza un helper. El fallo de una GPU no
elimina otras ni el inventario/memoria/motores. Máximo256 entradas por llamada.
Handle KMT y DLL se liberan por RAII incluso en errores o excepciones. No se
retiene la DLL ni hay worker, caché permanente o carga gráfica. Hotplug no es
una transacción atómica y no se impone un plazo a una llamada de driver bloqueada.
No se usan APIs térmicas vendor, aplicaciones externas ni lectura del BIOS.

## Verificación

Windows Release54/54 y Linux39/39, paquetes y consumidores instalados ejecutados.
API inyectada: GPU distintas y LUID signed high, conversión48.1/51.2°C, cero y
límite255°C, open/query denegados, handle nulo/identidad alterada, count0/múltiple,
tamaños/punteros/campos cambiados, éxito sin salida, rango inválido, revalidación
count fallida, actualización dinámica, error sin stale, IDs inválidos/duplicados,
API ausente, límite de inventario y cierres de todos los handles devueltos.
Monitor off/on/off y llamada nativa en AMD RX7900XT: **43°C** en una muestra.
Es dato dinámico, no constante. No se validó hardware Intel/NVIDIA, LDA real,
hotplug físico, callers32 bits ni drivers antiguos/sin soporte de perfdata.
Linux conserva sus proveedores hwmon/NVML anteriores.

Fuentes: [unidades y alcance de ADAPTER_PERFDATA](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ns-d3dkmthk-_d3dkmt_adapter_perfdata),
[apertura por LUID](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ns-d3dkmthk-_d3dkmt_openadapterfromluid),
[query y buffer](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ns-d3dkmthk-_d3dkmt_queryadapterinfo),
[header Microsoft con valores/layouts](https://github.com/microsoft/win32metadata/blob/main/generation/WinSDK/RecompiledIdlHeaders/shared/d3dkmthk.h).
