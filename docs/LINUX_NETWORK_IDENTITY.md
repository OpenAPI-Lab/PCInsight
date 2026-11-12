# Identificación de adaptadores Linux (F02)

## Consulta explícita de disponibilidad de direcciones

```cpp
#include <pcinsight/linux_network_addresses.hpp>
auto report = pcinsight::query_linux_network_addresses();
if (pcinsight::metric_is_known(report)) {
    // Una lista vacía aquí SÍ significa un volcado completado sin direcciones.
    for (const auto& row : *report.value) {
        // row.interface_index, row.ipv4, row.ipv6
    }
} else {
    // Mostrar desconocido y report.note; no inventar una lista vacía conocida.
}
```

Consulta separada, opt-in, del espacio de red del proceso. Solo incluye índices
con direcciones reportadas; no enumera adaptadores sin IP. No usa el respaldo
`getifaddrs`: un dump incompleto permanece desconocido. Los errores de acceso
`EACCES`/`EPERM` observados en syscalls devuelven `permission_required`; la nota
también conserva los códigos válidos recibidos en respuestas de error del
[kernel Netlink](https://docs.kernel.org/userspace-api/netlink/intro.html), con la
misma clasificación de permisos. Un ACK no se acepta como dump completo. La nota
distingue espera agotada, presupuesto o respuesta inválida. No implica permiso
para elevarse. Otras denegaciones no reconocidas pueden quedar unsupported.
No modifica `NetworkAdapter`
ni hace atómica la consulta con Monitor. No unir por índice entre namespaces
ni conservarlo como identidad permanente. Otros sistemas devuelven unsupported.
La adición requiere headers y biblioteca nuevos; los tipos existentes no cambian.

La prueba `linux-network-addresses-denied` provoca `EPERM` para sockets mediante
un filtro seccomp privado al proceso de prueba. Calibra el rechazo y comprueba
que la consulta pública devuelve `permission_required` sin valor. No cambia
permisos del host; si el entorno no admite el filtro, CTest registra skip77.

## Inventario de adaptadores

La lectura principal usa [`RTM_GETADDR`](https://docs.kernel.org/netlink/specs/rt-addr.html#getaddr)
y asocia IPv4/IPv6 por `ifa-index` con el índice sysfs de la fila de contadores.
Para la unión, [`if_nametoindex`](https://man7.org/linux/man-pages/man3/if_nametoindex.3.html)
debe resolver también ese nombre al mismo índice desde el proceso. Un fallo o
discrepancia deja las direcciones vacías sin descartar los contadores. La
coincidencia no es prueba de identidad entre espacios de red distintos que
reutilicen el mismo nombre e índice.
No depende de la etiqueta de la dirección. Si existe `IFA_LOCAL`, se prioriza
sobre `IFA_ADDRESS` para no mostrar un par punto a punto como dirección local.
Solo se usa un volcado terminado sin errores: se descarta una respuesta parcial,
interrumpida, inválida, mayor de 4 MiB o que exceda la espera de 100 ms.

En ese caso queda el respaldo anterior: el registro `AF_PACKET` de la misma
lista de [getifaddrs](https://man7.org/linux/man-pages/man3/getifaddrs.3.html)
debe coincidir en nombre e índice con la fila sysfs. El respaldo aún puede
omitir direcciones cuyo `IFA_LABEL` aparece como un nombre distinto. Ninguna
vía hace atómica la muestra ni detecta toda reutilización de índice. La prueba
sintética cubre etiqueta y punto a punto; la nativa comprueba loopback.

`NetworkAdapter::linux_details` es un Metric de LinuxNetworkDetails. Sustituye
la heurística que presentaba PCI_ID/driver/modalias como modelo y clasificaba
cualquier interfaz no llamada lo/sin wireless como Ethernet. Nombre/id de
interfaz conserva su significado; nunca se interpreta como fabricante o medio.

## Datos y fuentes

| Dato | Fuente / significado |
|---|---|
| interface_index | ifindex, verificado contra la fila de contadores |
| link_layer_type | type sysfs (ARPHRD), no bus ni medio físico |
| connection_type | ARPHRD más marcadores wireless/phy80211 y virtual/net |
| bus / driver | basename de symlinks subsystem / driver del dispositivo |
| hardware_vendor_id / hardware_device_id | IDs16bits PCI o USB según bus |
| vendor / model | etiquetas PCI locales opcionales o cadenas USB |
| virtual_device | padre comparado con devices/virtual/net |
| wireless | directorio wireless/phy80211 válido |

Informe externo ausente: no se estableció identidad coherente. Con informe
presente, cada Metric puede faltar por separado, con razón. `model` y
`connection_type` legacy reflejan valores o cadena vacía, no modalias como modelo.
Windows deja linux_details unsupported; su proveedor de red no cambia.
Interfaces virtuales/loopback no reciben marca/modelo del anfitrión ni driver.
Sin fuente PCI/USB compatible, marca/modelo quedan desconocidos, aunque bus/driver
puedan conocerse. No identidad comercial universal ni datos del BIOS.

## Tipo: sin heurística por nombre

ARPHRD772 produce Loopback incluso renombrado; PPP512, InfiniBand32 e IEEE802.11
(801..803) tienen etiquetas propias. Monitor802.11 no significa conexión asociada.
CAN y tipos de túnel IP/GRE reconocidos por el [kernel](https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/if_arp.h)
tienen etiquetas de capa de enlace; no identifican el transporte físico subyacente.
Otros tipos mantienen número y etiqueta ausente, nunca Ethernet por defecto.
ARPHRD_ETHER1 con marcador inalámbrico produce Wi-Fi; con padre virtual conocido,
Virtual Ethernet link-layer; en otro caso Ethernet link-layer. Estas etiquetas
no certifican cable/fibra/velocidad/estado de enlace/acceso a Internet. Marcador
inaccesible no se inventa como false; virtual_device=false no certifica hardware
físico. No se fabrican clases exactas veth/bridge/VLAN/túnel sin esa evidencia.

## Marca y modelo

PCI: vendor/device 0xHHHH, IDs disponibles aun sin diccionario. Nombres requieren
entrada única en pci.ids local: vendors raíz, dispositivos con un tab. Subsistemas
y clases no reemplazan al dispositivo. Duplicados/NUL/fuente vacía/>16MiB/línea
>4096B invalidan etiquetas dependientes, no IDs. Nombres ASCII imprimibles,
estimated con ruta; no retail autenticado. Reutiliza el lector regular acotado
de LINUX_GPU_NAMES.md, sin libpci/lspci/descargas/instalación. Monitor carga la
primera base disponible al pedir red o GPU y la conserva incluso si falta;
crear otro Monitor para recoger cambios. Mapeo limitado a primeras256 filas de
red por muestra; fuera del límite quedan IDs/metadatos sin etiquetas PCI.

USB: subsystem=usb y DEVTYPE único usb_device. Si device es usb_interface se
sube solo un padre, exigiendo también bus USB y DEVTYPE=usb_device. Lee idVendor/
idProduct (HHHH), manufacturer/product con disponibilidad independiente. Strings
del dispositivo, estimated, no autenticados; vacíos/controles/no ASCII/>4096B
quedan desconocidos. No nombre de interfaz USB, driver o antecesor arbitrario
como sustituto. No usb.ids/lsusb. Strings multilingües no ASCII no admitidos por
este proveedor conservador; no se promete cobertura de todos los textos USB.

## Coherencia y límites

Fija root/interfaz/dispositivo por fd. ifindex debe coincidir con F01 y type ser
decimal16bits válido. Comprueba ifindex/type y mismo directorio de interfaz antes/
después; dispositivo existente debe conservar directorio y bus al final. No es
snapshot atómico de IDs/strings/driver/direcciones/tráfico ni token de generación
hotplug/reset. Cambios ABA pueden pasar inadvertidos. Recrear Monitor al cambiar
network namespace. Fallo de metadatos no borra contadores válidos: fila permanece
con informe ausente y textos vacíos. Baselines/tasas F01 no cambian.

Atributos read-only/CLOEXEC/NONBLOCK/NOFOLLOW, solo archivos regulares,4096B y
sin NUL. FIFO/symlink de atributo rechazados sin bloqueo de apertura; symlinks de
directorio sysfs sí se siguen. Nombre1..15B sin slash/NUL/dot/dotdot y roots
internos acotados. EACCES/EPERM preservan permission_required cuando hay evidencia.
No elevación, comandos, cambios de interfaz ni tráfico externo. Límites no son
deadlines de syscalls ni techo global de memoria. include_networks=false evita
el proveedor. La base local en caché no es metadato de hardware retenido: se
vuelve a consultar la identidad sysfs por muestra, sin datos de una NIC retirada.

## Uso y compatibilidad

```cpp
for (const auto& adapter : snapshot.networks) {
    if (!adapter.linux_details.value) continue;
    const auto& d = *adapter.linux_details.value;
    if (d.vendor.value) { /* presentar con support y note */ }
    if (d.model.value) { /* no confundir con driver/nombre de interfaz */ }
    // Interpretar IDs numéricos junto al bus.
}
```

Nuevo layout público0.x de NetworkAdapter: recompilar biblioteca y TODOS los
consumidores, incluidos Windows, con cabeceras nuevas. SystemSnapshot no añade
campos directos en F02, pero sus elementos NetworkAdapter deben ser compatibles.
No ABI estable.

## Pruebas

tests/linux_network_identity.cpp: etiquetas PCI, duplicados/faltantes/clases/
subsistemas/NUL/bounds; tipo sin dependencia del nombre, virtual/loopback/Wi-Fi/
PPP/desconocido; PCI simulado y USB parent/DEVTYPE/strings/IDs; ifindex/type
inválidos, atributos FIFO/symlink y ausencia. Fixtures sysfs privados, sin crear
adaptadores. Positivos PCI/USB/Wi-Fi son fixtures, no prueba de NIC física.
Integración Monitor prueba interfaces nativas y opt-out; N02 conserva hardware.

Verificacion2026-10-16: Linux final81/81, Windows101/101 Release; paquetes
reinstalados y consumidores externos recompilados/ejecutados exit0. Build final
Linux sin warnings. Nativo WSL: lo ifindex1/ARPHRD772 Loopback; eth0 ifindex2/
ARPHRD1 Ethernet link-layer con bus vmbus y modelo desconocido, no NIC comercial
fabricada. PCI/USB/Wi-Fi positivos siguen siendo fixtures, no hardware fisico.

Una primera regresion Linux fallo en linux-integration por ausencia de CPU
global (assert second.cpu.utilization_percent.value, linea15). La prueba de
identidad y tasas de red paso. Se conservaron logs en work/network-identity-failed-
linux-tests.log y work/network-identity-failed-linux-list.log (work del workspace).
Despues integracion3/3 y suite81/81 pasaron sin cambios de codigo/aserciones.
Causa no determinada, reincidencia del riesgo previo N03; F02 NO lo corrige.

Fuentes primarias: [ABI net sysfs](https://github.com/torvalds/linux/blob/master/Documentation/ABI/testing/sysfs-class-net),
[ARPHRD](https://github.com/torvalds/linux/blob/master/include/uapi/linux/if_arp.h),
[PCI sysfs](https://docs.kernel.org/PCI/sysfs-pci.html),
[USB sysfs](https://github.com/torvalds/linux/blob/master/drivers/usb/core/sysfs.c).
Etiquetas de presentación y límites son decisiones PCInsight, no garantías retail
del kernel.
