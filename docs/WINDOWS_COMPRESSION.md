# Memoria comprimida Windows: proveedor experimental

Implementado y probado el 2026-10-12. No se encontró en las interfaces públicas
consultadas un equivalente documentado que garantice la cifra global mostrada
por el Administrador de tareas. El proveedor utiliza una consulta interna de
Windows, por lo que **está desactivado por defecto**.

## Activación explícita al compilar

```sh
cmake -S outputs/pcinsight -B work/build-compression -DCMAKE_BUILD_TYPE=Release -DPCINSIGHT_ENABLE_EXPERIMENTAL_WINDOWS_COMPRESSION=ON
cmake --build work/build-compression
ctest --test-dir work/build-compression --output-on-failure
```

Solo está habilitado para compilaciones Windows x64. En otras arquitecturas o
sin la opción, `memory.compressed_bytes` permanece sin valor y con una nota.
No cambia la API pública ni requiere software adicional.

## Semántica y límites

Se consulta `NtQuerySystemInformation`, clase `SystemStoreInformation` (109),
operación de lectura `MemCompressionInfoRequest` (22), solicitud versión 3.
El campo obtenido es `TotalCompressedSize`: bytes de contenido comprimido.
Se publica como `Support::estimated`, con nota que distingue este contenido de
la huella residente total del almacén y del campo del Administrador de tareas.
No se suma a `in_use_bytes`: no es una reserva adicional independiente.

Esta interfaz interna no tiene garantía de estabilidad entre versiones de
Windows. La implementación valida tamaños/alineación, versión y envoltorio de
respuesta, pero esas comprobaciones no garantizan compatibilidad futura.
Un error de la consulta o estructura inesperada deja el valor ausente; acceso
denegado o privilegio faltante produce `permission_required`. Cero solo se
publica tras una consulta exitosa con respuesta compatible.

Se resuelve el símbolo desde `ntdll.dll` ya cargada por Windows. No se carga una
DLL externa, no se instala un driver, no se habilitan privilegios y no se llaman
operaciones que vacíen, reconfiguren o desactiven la compresión.

## Verificación

Windows Release: 7/7 pruebas con la opción OFF y 7/7 con ON. Linux WSL: 5/5.
La lectura experimental real devolvió 297533440 bytes en la PC de desarrollo;
es una muestra dinámica, no un valor esperado para otras ejecuciones.

Pruebas inyectadas: dato válido, cero real, función ausente, acceso denegado,
privilegio faltante, clase/longitud rechazada, estado no exitoso, cambios en
versión/operación/puntero/longitud y recuperación sin reutilizar datos anteriores.
No se ha validado equivalencia exacta con el Administrador de tareas ni cobertura
de distintas versiones de Windows. No se considera soporte estable completo.

Referencias de interoperabilidad, consultadas el 2026-10-12:

- [Definiciones de la interfaz interna en System Informer/phnt](https://github.com/winsiderss/systeminformer/blob/master/phnt/include/ntexapi.h)
- [Uso de la consulta en System Informer](https://github.com/winsiderss/systeminformer/blob/master/phlib/native.c)
- [Información de memoria documentada por Microsoft](https://learn.microsoft.com/en-us/windows/win32/memory/memory-performance-information)
