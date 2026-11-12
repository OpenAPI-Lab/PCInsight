# OpenGL Windows: contexto actual atribuido al adaptador DXGI

M04 parcial, revision52, 2026-10-21. Nuevo proveedor WGL integrado en Monitor
mediante la opcion existente include_current_opengl_context=false. No se crea
una ventana/contexto en la biblioteca, no hay draw/workload ni cambio de contexto,
perfil, pixel format o configuracion. No ejecuta programas ni instala SDK/drivers.

```cpp
pcinsight::SampleOptions options;
options.include_gpus = true;
options.include_current_opengl_context = true;
// Invocar en el hilo que YA tiene el contexto desktop WGL actual del host.
auto snapshot = monitor.sample(options);
for (const auto& gpu : snapshot.gpus) {
    if (pcinsight::metric_is_known(gpu.opengl_version)) {
        // Version del CONTEXTO asociada a este adaptador, no un maximo hardware.
    }
}
```

Sin contexto actual, contrato de identidad o correspondencia unica, el campo
queda sin valor/unsupported, con motivo; no0 ni una version global del loader.
La opcion desactivada no consulta WGL; include_gpus=false tampoco. El Sampler
ejecuta en su propio hilo: no hereda el contexto actual del hilo GUI. No cambiar
esa propiedad creando un contexto automaticamente dentro del colector.

## Atribucion y ambito

Se exige contexto+DC actuales estables y GL_VERSION desktop valido/acotado.
Las extensiones se enumeran por glGetStringi/GL_NUM_EXTENSIONS para GL>=3, sin
usar GL_EXTENSIONS legacy en un perfil core. Contextos antiguos usan la lista
legacy. Se exige un par exacto base+Win32 de EXT_memory_object o EXT_semaphore;
prefijos/sufijos/duplicados/metadatos malformados no dan capacidad.

GetUnsignedBytevEXT(DEVICE_LUID_EXT) devuelve los8bytes que la especificacion
asocia a IDXGIAdapter1. Se exige LUID coincidente con EXACTAMENTE una
fila DXGI, no nombre/vendor/device ID/orden. Se repiten LUID, version y checks
contexto/DC antes de publicar; cambios detectados descartan el resultado.
Las dos salidas LUID parten de bytes distintos para detectar ausencia/escritura
parcial; un LUID cero reportado consistentemente puede coincidir con una fila,
no se trata el cero como fallo ni se inventa un indicador de validez.
[Contrato de identidad EXT Win32](https://raw.githubusercontent.com/KhronosGroup/OpenGL-Registry/main/extensions/EXT/EXT_external_objects_win32.txt),
[contrato base de funciones EXT](https://registry.khronos.org/OpenGL/extensions/EXT/EXT_external_objects.txt).

El ambito de GpuInfo Windows es un ADAPTADOR DXGI. Se publica GL_VERSION del
contexto asociado a ese adaptador: no version maxima/core/compat soportada, nivel
DirectX, capacidad de una GPU fisica interna ni prueba de aceleracion hardware.
La mascara de nodos no se consulta ni se deduce. No se reparte este contexto a
otros adaptadores, y LUID duplicado en el inventario deja todas esas filas sin
valor. El informe conserva el texto completo reportado, no una version comercial.

## Carga, limites y efectos

opengl32.dll se carga SOLO desde System32 con referencia RAII, y se resuelven
los entrypoints basicos. Funciones EXT se resuelven para el contexto actual,
sin cache entre contextos; se rechazan NULL y sentinels1/2/3/-1. Falta de modulo/
funciones deja unknown, denegacion Win32 observada al cargar conserva
permission_required. Fallos GL sin codificacion de permisos no se reclasifican.
[WGL y el hilo llamador](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-wglgetcurrentcontext),
[resolucion por contexto](https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-wglgetprocaddress).

Limites:1024B para GL_VERSION (ASCII imprimible),65536B lista legacy,4096
extensiones,256B por nombre indexado y128KiB de nombres indexados retenidos.
Version numerica de2/3componentes, major>0, componentes<=65535, sufijo imprimible
permitido; OpenGL ES no se atribuye como desktop GL. Datos fuera de limite o
sin terminador se rechazan. Punteros del driver deben cumplir el contrato de
la API: no hay aislamiento frente a un driver defectuoso/punteros invalidos.

La biblioteca NO llama glGetError ni consume errores previos. Como cualquier
consulta GL, un driver que incumple su contrato puede generar errores nuevos;
no se promete restaurar el estado de errores, ni se intenta hacerlo consumiendo
los del host. Las consultas finales se verificaron aqui sin errores nuevos.
Tampoco se impone timeout a un driver bloqueado. Las comprobaciones repetidas
no son una transaccion/defensa frente a ABA o hotplug despues del retorno.

No cambia layout de SampleOptions/GpuInfo, firmas/vtables, formatos de historial
ni version0.2. La opcion ANTES solo se aplicaba a Linux; cambio de comportamiento
Windows opt-in: ahora realiza estas consultas en el hilo llamador. Recompilar y
relinkar biblioteca/consumidores juntos. Linux conserva EGL/GLX sin cambios.
El paquete no requiere enlazar opengl32 en el consumidor: carga dinamica. Solo
el ejecutable de prueba enlaza OpenGL/GDI/User para crear su contexto privado.

## Validacion y diagnostico nativo

La prueba crea SOLO su ventana oculta/contexto privados, restaura el par previo
y elimina contexto/DC/ventana/clase. No usa SwapBuffers ni dibuja. CTest limita
el proceso a40s. Comprueba fuente sin contexto, pares memory/semaphore, legacy/
indexado, versiones invalidas/ES, limites, nombres duplicados, funciones ausentes/
sentinels, LUID cero conocido/salida ausente o parcial/cambiante/ajena, DC/contexto/version cambiantes y filas DXGI
duplicadas. No quedan versiones viejas ni cambian metricas ajenas. Monitor
on/off y include_gpus=false verificados; consumidor instalado sin contexto no
recibe version inventada ni abre un contexto.

Diagnostico acotado en RX7900XT: el contexto privado reporto
`4.6.0 Compatibility Profile Context 26.8.1.260810` y su LUID coincidiente con
DXGI. La consulta adicional de nodo SOLO EN EL TEST devolvio GL_INVALID_ENUM
(1280), sin escribir la salida. El primer prototipo, que exigia un nodo, quedaba
unknown. El contrato requerido se reviso: para la version del contexto a nivel
ADAPTADOR basta LUID, no hace falta atribucion fisica por nodo. Se retiro esa
consulta innecesaria del proveedor, sin convertir -1/0 a un nodo valido ni
afirmar una capacidad fisica. El diagnostico previo queda preservado en
work/wgl-identity-diagnostic.log y wgl-final-native-mingw.log.

Proveedor final nativo positivo en MinGW y MSVC: mismo LUID/version solo en la
fila correcta, llamada directa e integracion Monitor, contexto/DC intactos.
Se prueba con error previo sembrado (no consumido) y tambien sin error previo
(sin error nuevo que pudiera quedar oculto por aquel). No se toma el renderer
por nombre como prueba de identidad. Es una observacion del driver actual,
no matriz multivendor/multiGPU/RDP/headless ni maxima version por adaptador.

Suites Release finales: Windows128/128 MinGW y128/128 MSVC, Linux111/111.
Repeticion20 del test WGL por toolchain Windows, consumidores instalados
independientes de las3toolchains recompilados/relinkados/ejecutados exit0.
Logs finales work/wgl-adapter-{build,native,suite}-{mingw,msvc}.log,
wgl-final-{build,suite}-linux.log, wgl-install-* y wgl-consumer-*.
Evidencia de entrega definitiva (incluye LUID cero conocido, HighPart negativo,
salidas ausentes/parciales y diagnostico protegido frente a sentinels) en
wgl-release-{build,suite,repeat,install,consumer}-{mingw,msvc}.log y
wgl-release-{build,suite,install,consumer}-linux.log.20repeticiones finales por
Windows toolchain; las repeticiones de prototipos anteriores se preservan, pero
no se suman como si hubieran comprobado todos los ultimos casos.
MSVC conserva D9025 intencional de /UNDEBUG en tests; no se desactivan warnings.
Los logs iniciales/intermedios no sustituyen la evidencia del ambito final.
M04 sigue abierta por el resto de su criterio y cobertura, sin nuevo cierre ni
credito fraccionario; M05 permanece aplazada por decision del usuario.
