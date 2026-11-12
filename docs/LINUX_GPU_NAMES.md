# Nombres GPU Linux mediante IDs PCI locales

El inventario DRM conserva ahora los IDs numéricos en hardware_vendor_id y
hardware_device_id (16 bits, Support::available), incluso sin una base de nombres.
Solo acepta ambos atributos DRM con formato 0xHHHH y ubicación PCI available
válida. No interpreta un nombre de driver como un número ni relaciona por índice.

Cuando existe pci.ids, se añaden pci_vendor_name y pci_device_name como
Metric<string> con Support::estimated y ruta de la fuente en note. vendor/model
usan esos textos para presentación; un modelo NVML posterior tiene prioridad.
Las etiquetas PCI siguen separadas aunque NVML publique un nombre más preciso.
No representan una certificación del modelo comercial, ensamblador o memoria:
una entrada PCI puede describir varias tarjetas y la base puede estar obsoleta.

## Lectura y límites

Se intenta, en orden, /usr/share/hwdata/pci.ids, /usr/share/misc/pci.ids,
/usr/share/pci.ids. Se conserva el primer archivo regular legible, no vacío y
acotado a 16 MiB, con tamaño/mtime estables durante la lectura. Se abre con
O_CLOEXEC/O_NONBLOCK; FIFO/directorios no se leen. No ejecuta pciutils/lspci,
no enlaza libpci, no descarga/instala/busca en red y no requiere root.
No se redistribuye la base ni se obliga al consumidor a instalarla.

Monitor carga la base de forma diferida al solicitar GPU o red (desde F02) y la
conserva durante su vida (también si no existe). Para recoger una actualización
de la base hay que crear otro Monitor. El inventario/IDs de GPU sigue siendo
nuevo por muestra; no se reutilizan etiquetas de una GPU retirada.

Parser propio: vendors de nivel raíz y dispositivos con una tabulación,
IDs hex de cuatro dígitos, nombres ASCII imprimibles, LF/CRLF y EOF sin LF.
Subsistemas con dos tabs no reemplazan al dispositivo. Secciones C, S y futuras
desconocidas reinician el vendor, incluidos sus hijos; comentarios no lo cambian.
Duplicados de vendor invalidan sus etiquetas, duplicados de dispositivo invalidan
ese modelo (incluso con texto idéntico). Línea mayor de 4096 bytes invalida todas
las etiquetas; nunca publica registros truncados. Solo almacena los IDs buscados
para un máximo de 256 filas GPU. Más filas conservan IDs, sin etiquetas.

Sin archivo, entrada única o nombre válido: etiquetas ausentes y cadenas raw de
DRM conservadas; no se inventa un modelo. Los IDs no son atómicos frente a hotplug
y este proveedor no consulta ROM/BIOS, fecha de driver ni GPUs no PCI. Los errores
de acceso a la base opcional no se confunden con permisos del hardware.
En Windows las nuevas etiquetas PCI permanecen unsupported; DXGI no cambia.
Al cambiar GpuInfo en esta API0.x deben recompilarse biblioteca y consumidores.

## Verificación

Prueba Linux dedicada con base sintética: varias GPU/vendors/dominos, IDs raw,
prioridad NVML, propagación de permisos NVML aun con nombre friendly, ausencia,
IDs inválidos, fuente vacía, duplicados, class/subsystem/extensiones desconocidas,
líneas largas, NUL/controles, tamaño global y límite de filas.
Loader nativo probado con archivos temporales propios, fichero regular, rutas
ausentes, directorio, FIFO (sin bloqueo), sparse sobredimensionado y permisos
denegados para callers no root. La base del sistema se consulta sin modificarla.
Estas pruebas no sustituyen una identificación física de tarjeta en Linux.
Resultado Release: Linux42/42 y Windows55/55; paquetes instalados y consumidores
externos reconstruidos/ejecutados. La consulta nativa encontró la base existente
/usr/share/misc/pci.ids (1444058 bytes); WSL no expone una tarjeta DRM física
para comprobar el resultado end-to-end de identificación hardware.

Fuentes: [formato pci.ids](https://github.com/pciutils/pciutils/blob/master/pci.ids.man)
y [atributos PCI sysfs](https://docs.kernel.org/PCI/sysfs-pci.html).
