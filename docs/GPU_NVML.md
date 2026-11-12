# Proveedor NVIDIA NVML en Linux

Revision21: auditoría/correcciones G06 en [LINUX_GPU_TELEMETRY.md](LINUX_GPU_TELEMETRY.md).
La fusión exige ubicación PCI available y tuple de memoria NVML completo y
coherente; no mezcla capacidad NVML con uso/libre DRM. Dedicated-used es alias
explícito del used reportado; shared-used sigue ausente, no GTT renombrado.
El diagnóstico público de inventario distingue cobertura parcial/vacía/ausente.
Nuevo layout GpuInfo/SystemSnapshot0.x: recompilar TODOS. Verificación actual:
Linux89/89,Windows107/107 y consumidores instalados reconstruidos exit0.
La evidencia numérica anterior abajo es histórica, no cobertura física nueva.

## Alcance de esta unidad

Proveedor opcional integrado en `Monitor::sample` cuando `include_gpus=true`:
inventario NVIDIA (UUID/modelo/PCI), versión del driver, utilización general,
memoria framebuffer total/usada/libre y temperatura. No ejecuta `nvidia-smi`,
no instala paquetes, no cambia clocks/políticas ni eleva permisos.

La biblioteca usa `dlopen("libnvidia-ml.so.1", RTLD_NOW|RTLD_LOCAL)` y símbolos
NVML versionados explícitamente. El driver debe proporcionar esa biblioteca:
no hay dependencia de headers CUDA/NVML para compilar PCInsight. Se intentará
la carga al primer muestreo GPU de cada Monitor; crear otro Monitor permite
reintentar después de instalar/cambiar el driver. Sin NVML, el lector DRM sigue
funcionando. Un Monitor conserva la biblioteca cargada hasta su destrucción.

Cada lectura tiene init/shutdown equilibrados, incluso al fallar inventario o
lanzarse una excepción. Las consultas no tienen timeout propio: un driver
bloqueado puede demorar el muestreo. NVML puede compartir referencias de
inicialización con otras bibliotecas del mismo proceso.

## Semántica y límites

- Identidad por UUID; fallback PCI validado, nunca por índice efímero.
- Asociación a DRM solo con PCI equivalente y único en ambos inventarios.
  Se normalizan dominios de cuatro/ocho dígitos y mayúsculas/minúsculas.
  Las asociaciones ambiguas permanecen separadas, sin sobrescribir otra GPU.
- Memoria NVML v1: `used` incluye reserva del driver; no se confunde con consumo
  de una app. Se exige total=free+used sin overflow ni valores sentinel.
  El total es el framebuffer reportado, no una lectura independiente del BIOS.
- Utilización es tiempo de kernels activos en la ventana del driver, no un
  motor 3D/copy ni porcentaje de bytes asignados. No se inventan motores.
- Errores por campo dejan el valor ausente; NO_PERMISSION produce
  `permission_required`. Símbolos opcionales ausentes no destruyen inventario.
  Un fallo de NVML no descarta un dato válido del lector DRM.
- No ofrece reserva shared estilo Windows, fecha del driver ni contextos
  OpenGL. Tampoco implementa enumeración MIG o telemetría de procesos GPU.
  Productos/particiones con métricas no soportadas devuelven ausencia.
- Inventario limitado a 4096 dispositivos; cadenas bounded con NUL obligatorio.
  Errores/identidades ausentes permiten un inventario parcial diagnosticado
  internamente. El API público actual no tiene un diagnóstico global de GPU
  cuando no puede enumerarse ningún dispositivo.

## Verificación y criterio de cierre

2026-10-14: implementación y pruebas automáticas terminadas para el alcance
anterior. Linux Release 9/9; Windows Release 21/21 (en Windows se prueba la lógica
portable, no se carga un driver NVML Linux).

Las pruebas cubren cero válido, límites, overflow/sentinel de memoria,
permisos, GPU perdida, símbolos opcionales ausentes, dos GPU, identidades
duplicadas/ambiguas, normalización PCI y balance init/shutdown. Además CTest
carga un `.so` sintético con los nombres ABI reales y prueba `Monitor` completo
con dos GPU. `LD_LIBRARY_PATH` se altera solo para ese proceso de prueba;
la biblioteca sintética no se instala ni se incluye en el producto entregable.

**Validación física pendiente, no sección GPU completa:** el equipo disponible
es AMD Radeon RX 7900 XT. Ubuntu WSL tiene `/dev/dxg`, pero no hay
`libnvidia-ml.so.1` en las rutas del driver inspeccionadas ni NVML cargable.
La ejecución nativa verificó ausencia de NVML y fallback, no métricas NVIDIA.
Para verificar hardware se necesita Linux con NVIDIA y driver NVML disponible:
ejecutar `pcinsight-nvml-tests` sin el entorno sintético de CTest y comprobar
inventario y métricas reales; después comparar dos muestras de `Monitor`.
No se cerrará por esta evidencia una subtarea que exija esa validación física.

Referencias primarias: [NVML](https://docs.nvidia.com/deploy/nvml-api/latest/index.html),
[consultas y semántica](https://docs.nvidia.com/deploy/nvml-api/latest/api/group__nvmlDeviceQueries.html),
[ABI publicada por NVIDIA](https://github.com/NVIDIA/go-nvml/blob/main/pkg/nvml/nvml.h).
