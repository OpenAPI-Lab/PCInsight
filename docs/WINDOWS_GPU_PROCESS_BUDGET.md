# Presupuesto de memoria GPU del proceso llamador

Actualizacion revision54: [NVML Windows](WINDOWS_NVML_MEMORY.md) agrega en
Monitor una fuente INDEPENDIENTE de framebuffer sin asignar para NVIDIA
inequivoca. Las referencias historicas de este documento a free desconocido
siguen siendo ciertas para DXGI/presupuesto; no describen ese nuevo proveedor.
No se usa Budget/CurrentUsage/AvailableForReservation para completar free.

`GpuInfo.local_process_memory` y `non_local_process_memory` contienen
GpuProcessMemoryBudget. Son presupuestos DXGI del proceso que llama a PCInsight
y del nodo físico0, separados de las capacidades/uso globales del adaptador.
**No completan memory_available_bytes ni significan memoria libre global.**

## Campos y contrato

- process_id: PID real del llamador en esa muestra; no PID arbitrario seleccionado
  por GUI, ni suma de todos los procesos.
- node_index: índice solicitado0; no número de nodos ni agregación de linked GPUs.
- budget_bytes: objetivo de presupuesto que Windows asigna al proceso.
- current_usage_bytes: uso de ese proceso/nodo/grupo.
- available_for_reservation_bytes: cantidad que el proceso puede solicitar como
  reserva; no memoria físicamente libre ni garantía de poder asignar ese bloque.
- current_reservation_bytes: reserva actual, una indicación del working set.
- budget_headroom_bytes: max(budget_bytes-current_usage_bytes,0), aritmética
  saturada en enteros64. No allocatable/free: uso puede superar presupuesto.

Todos los tamaños son bytes. LOCAL/NON_LOCAL son grupos de segmentos de memoria
DXGI; no se renombran dedicated/shared porque esa equivalencia depende del
adaptador/UMA. No sumar presupuestos de procesos como capacidad física.
Los valores reported0 son válidos. UINT64 completo se conserva sin double.
El margen es una cantidad definida exactamente por la fórmula, no estimación
de disponibilidad física; su nota mantiene ese alcance.

## Consulta

El mismo IDXGIAdapter1 del inventario se consulta por IDXGIAdapter3, sin nueva
enumeración o asociación por nombre/índice. QueryVideoMemoryInfo consulta nodo0
para cada grupo independientemente. Fallo de LOCAL no borra NON_LOCAL ni viceversa;
interfaz ausente/permisos/fallo dan valor ausente, no cero. PID e índice solicitado
siguen explícitos aunque falle la consulta: son contexto, no datos medidos.
Se reinician los resultados en cada invocación, sin stale tras errores.

Referencia COM adicional por RAII, incluso si un fallo devuelve un puntero;
no dispositivo D3D, workload, cambios de reservas, callbacks, worker, afinidad,
SDK instalado ni herramienta externa. DXGI ya era dependencia del backend.
No se llama SetVideoMemoryReservation. La consulta puede tocar estado interno
del driver; no hay plazo impuesto a API bloqueada. Las dos consultas y otros
proveedores no forman una transacción atómica.

## Alcance pendiente

Solo proceso llamador y nodo0, no monitor de presupuestos de cualquier PID ni
cobertura de todos los nodos. Linux deja estos campos sin valor y conserva sus
proveedores previos. Windows mantiene memory_available_bytes sin valor: no se
restan capacidades DXGI con uso PDH ni se disfraza este presupuesto como libre.
Las estadísticas KMT de segmentos examinadas están documentadas como reservadas
para uso del sistema; no se incorporó ese acceso como fuente estable de free.
La consulta AMD OpenCL examinada no se implementó: su contrato público de layout
y alcance requiere más validación; no se adivinaron tipos o una segunda salida.

Campos públicos añadidos0.x: recompilar biblioteca y consumidores juntos.
La implementación de esta unidad está completa; la memoria libre global sigue
siendo otro pendiente y no se marca completada por este avance.

## Verificación

Windows Release55/55 y Linux39/39, instalación y consumidores externos ejecutados.
Pruebas inyectadas: identidad exacta del adaptador, grupos independientes, campos
y PID/nodo, cero real, uso mayor que budget sin underflow, UINT64máximo, permisos,
interfaz ausente/nula/fallida, error de un grupo, stale reset, tabla incompleta y
cierre COM incluso con excepción. La prueba preserva una métrica global fixture
sin modificarla. Inventario inyectado asigna presupuestos distintos a dos GPU.
Monitor off/on y consulta nativa: RX7900XT, PID22616 en esa ejecución, nodo0,
budget local20607967232bytes y uso0. Son datos de ese proceso y momento, no
capacidad libre ni constantes. Intel/NVIDIA/LDA/hotplug físico no certificados.

Fuentes: [ámbito por proceso y nodo](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/nf-dxgi1_4-idxgiadapter3-queryvideomemoryinfo),
[significado de los cuatro campos](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_4/ns-dxgi1_4-dxgi_query_video_memory_info),
[estadísticas KMT reservadas](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/d3dkmthk/ns-d3dkmthk-d3dkmt_querystatistics_segment_information).
