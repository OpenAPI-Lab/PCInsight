# Inventario de firmware compartido

Linux lee la tabla binaria de sysfs; Windows usa `GetSystemFirmwareTable` con
el proveedor RSMB. Ambos pasan por el mismo parser de memoria, sin herramientas
externas. Límite de tabla: 16 MiB. El inventario se conserva durante la vida
del monitor y no es una fuente de utilización, temperatura o frecuencia viva.

## Reglas de lectura

- Tablas con longitudes inválidas, registros truncados o cadenas sin terminar
  se rechazan sin devolver resultados parciales. El envoltorio Windows también
  valida su longitud. Se tolera padding después del marcador de fin.
- Los dispositivos se filtran por pertenencia a arrays de memoria del sistema
  cuando el firmware publica esa relación. Sin arrays, los datos se marcan
  estimados: podrían incluir memoria distinta de la RAM del sistema.
- Los slots son posiciones descritas por registros de dispositivos del firmware,
  no una inspección física del motherboard; también pueden representar memoria
  soldada. No se reconstruyen posiciones que el firmware omita.
- Un tamaño desconocido no cuenta como un slot vacío ni permite presentar una
  suma parcial como capacidad total. Se admiten tamaños extendidos y unidades
  KiB/MiB. Un tamaño extendido ausente no se interpreta como 32767 MiB.
- Tipo, formato y velocidad solo se resumen si los dispositivos instalados
  tienen valores conocidos iguales. La API actual no expone la lista por DIMM;
  una combinación heterogénea deja el resumen sin valor.
- La velocidad es la configurada, incluidos sus campos extendidos. No se usa
  la velocidad máxima del módulo como sustituto.
- SMBIOS Current Speed de CPU es una estimación del valor al arrancar; no
  garantiza la frecuencia nominal base. Se ignoran sockets no poblados y se
  omite el resumen cuando los procesadores publican valores diferentes.
- Una tabla accesible no implica que cada campo sea accesible: un campo ausente
  devuelve `unsupported`, nunca `available` sin valor. Se conserva
  `permission_required` si no se pudo acceder por permisos.

El firmware puede contener información incorrecta incluso con una tabla
estructuralmente válida. No se promete obtener datos ausentes en toda máquina.
Los casos de RAM persistente/lógica avanzada requieren ampliar el modelo por
dispositivo antes de ofrecer un inventario exhaustivo de esas plataformas.

## Pruebas

Fixtures en memoria y lectura de archivo: DDR4, slots vacíos, tamaños desconocidos
y extendidos, velocidad configurada extendida, mezclas de dispositivos,
exclusión de arrays de vídeo, propiedad faltante, CPU poblado/no poblado,
truncamientos de registros y del envoltorio Windows. Assertions activas en Release.

Referencias: [API de firmware Windows](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemfirmwaretable)
y [especificación SMBIOS de DMTF](https://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.9.0.pdf).
