# Clasificación de motores DRM por driver

classify_linux_drm_engine(driver, engine) devuelve Metric<LinuxDrmEngineClass>.
Es una consulta pura incluida en linux_drm.hpp: no requiere Linux para usar la
tabla, no carga drivers, no hace IO y no descubre hardware. Las lecturas raw y
los agregados incorporan engine_class, también visible en snapshot.linux_drm
cuando se activa la integración de Monitor. El nombre nativo no se cambia.

La categoría describe una clase/capacidad del motor, NO el tipo de trabajo que
está ejecutando ni una medición 3D exclusiva. El porcentaje sigue teniendo la
semántica observada por cliente; la categoría no lo vuelve GPU-global.

## Mapeos exactos

| Driver | Nombre nativo | LinuxDrmEngineClass |
|---|---|---|
| amdgpu | gfx | graphics_compute |
| amdgpu | compute | compute |
| amdgpu | dma | copy |
| amdgpu | dec | video_decode |
| amdgpu | enc, enc_1 | video_encode |
| amdgpu | jpeg | jpeg |
| amdgpu | vpe | video_processing |
| i915 | render | graphics_compute |
| i915 | copy | copy |
| i915 | compute | compute |
| i915 | video | video_codec |
| i915 | video-enhance | video_processing |
| xe | rcs | graphics_compute |
| xe | bcs | copy |
| xe | ccs | compute |
| xe | vcs | video_codec |
| xe | vecs | video_processing |

Coincidencia exacta y sensible a mayúsculas, sin prefijos, sufijos, números
inferidos ni normalización de espacios. Inputs vacíos o mayores de128 bytes,
drivers desconocidos, variantes no documentadas y Xe other quedan sin value,
Support::unsupported. No se inventan aliases como sdma/dma0/compute0 ni se
descartan sus contadores raw si el parser los admite. NVIDIA/nouveau no heredan
categorías de Intel/AMD solo por compartir un nombre.

Un mapeo conocido tiene Support::available y note con procedencia y alcance:
es la taxonomía disponible, no evidencia de que exista ese motor en la máquina.
Los agregados recalculan la clase desde driver/nombre y no confían en categorías
que el consumidor haya colocado manualmente en sus observaciones.

Xe publica normalmente ciclos/total-ciclos. LINUX_DRM_CYCLES.md implementa esa
base sin convertirla a ns; la clasificación permanece independiente de las tasas.

## Verificación y límites

18 pares exactos: clasificación, parser ns sintético, agregación en primera y
segunda muestra, preservación de nombre/tasa y rechazo de clase adulterada.
Negativos: drivers/nombres similares, espacios, mayúsculas, NUL, límites, nombres
desconocidos y registro Xe solo-ciclos. Fixture Monitor confirma graphics_compute
sin tocar GpuInfo.utilization_percent ni sus motores globales.

No instala ni descarga bases de datos/SDK en runtime. La investigación se contrastó
con fuentes del kernel; no se redistribuye código del driver. API0.x ampliada:
recompilar librería y consumidores juntos. Positivo físico Linux AMD/Intel y
NVIDIA, taxonomías nuevas y cobertura hardware global siguen pendientes.
LINUX_DRM_MAXFREQ.md añade la alternativa de ciclos/frecuencia máxima.
Resultado Release: Linux47/47 y Windows60/60; paquetes instalados y consumidores
externos reconstruidos/ejecutados con la API pública de clasificación.

Fuentes:

- [AMDGPU fdinfo, Linux v6.12](https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/gpu/drm/amd/amdgpu/amdgpu_fdinfo.c).
- [Semántica de bloques AMDGPU](https://www.kernel.org/doc/html/v6.10/gpu/amdgpu/driver-core.html).
- [Clases Intel y fdinfo i915](https://www.kernel.org/doc/html/latest/gpu/i915.html).
- [Tabla i915 uabi_class_names](https://kernel.googlesource.com/pub/scm/linux/kernel/git/torvalds/linux/+/69050f8d6d075dc01af7a5f2f550a8067510366f/drivers/gpu/drm/i915/i915_drm_client.c).
- [Nombres de estadísticas Xe](https://www.kernel.org/doc/html/latest/gpu/xe/xe-drm-usage-stats.html).
