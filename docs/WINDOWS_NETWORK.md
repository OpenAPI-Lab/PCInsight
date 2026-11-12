# Windows: interfaces, direcciones y tráfico

Implementado 2026-10-13 mediante GetIfTable2 y GetUnicastIpAddressTable,
unidas por InterfaceLuid. `include_networks=true` activa la lectura; false
evita las consultas y borra las bases para calcular tasas. No se generan
paquetes ni se modifica configuración de red.

## Campos

- id: LUID local en texto (`luid:...`). No es identidad permanente portable
  entre equipos/reinicios. Para deltas también se exige GUID y tipo iguales.
- name: alias de interfaz; model: descripción del adaptador dada por Windows.
  No se verifica el modelo de hardware. La consulta adicional
  [query_windows_network_identity](WINDOWS_NETWORK_IDENTITY.md) obtiene
  fabricante PnP/INF por GUID, sin deducirlo de este texto.
- connection_type: Ethernet, Wi-Fi, Loopback, Tunnel, PPP o tipo IANA numérico.
  Un tipo Ethernet también puede representar un adaptador virtual.
- ipv4/ipv6: direcciones unicast configuradas en las tablas del sistema, sin
  duplicados. IPv6 conserva el scope numérico cuando lo proporciona Windows.
  No se filtran por estado DAD ni se garantiza conectividad de esas direcciones.
- total_sent_bytes/total_received_bytes: OutOctets/InOctets de la interfaz;
  no son necesariamente acumulados desde arranque ni tráfico de una aplicación.
- send_bytes_per_second/receive_bytes_per_second: delta de octetos entre
  muestras dividido por segundos monotónicos. Son bytes/s enteros redondeados
  hacia abajo, no bits/s ni velocidad nominal del enlace.

La primera muestra queda sin tasa. Regresión de contadores, identidad/estado
operacional cambiado, intervalo no positivo o overflow deja la tasa sin dato.
Una muestra siguiente válida recupera el cálculo; delta cero válido da 0 B/s.
Interfaces desaparecidas se eliminan de la caché. No se suman entre interfaces:
capas virtuales, túneles y loopback pueden contar el mismo tráfico varias veces.

Las tablas incluyen interfaces lógicas/físicas, activas/inactivas y filtros.
El GUI puede seleccionar cuáles presentar. Son dos consultas separadas, no
un snapshot atómico: direcciones sin interfaz coincidente se omiten y una
interfaz puede cambiar después de leerse. Los intervalos corresponden al
sondeo y no a una medición continua de paquetes.

## Errores y recursos

Fallo global en cualquiera de las tablas lanza system_error y reinicia las
bases de tasas, sin devolver una lista parcial como exitosa. Se validan tablas
no nulas, máximo 100000 registros por tabla, LUID no nulo/único y cadenas Unicode
terminadas y válidas. La memoria asignada por Windows se libera con FreeMibTable
en todas las rutas. Las APIs no devuelven tamaño de asignación: se confía en
que una tabla nativa válida contiene las filas declaradas; el límite numérico
no sustituye esa garantía del proveedor del sistema.

Las dependencias iphlpapi/ws2_32 son bibliotecas del sistema Windows. No hay
instalaciones externas ni subprocess de recolección. No se requiere elevación
para la lectura normal; errores de acceso se propagan.

## Verificación y pendientes

Fixtures cubren asociación LUID, IPv4, IPv6 con scope, deduplicación, Unicode,
deltas, cero, contador regresivo, GUID/estado cambiado, intervalo nulo, reset,
fallo y recuperación, tablas nulas/excesivas, LUID duplicado, cadenas inválidas
y liberación de memoria. Integración de solo lectura: 61 interfaces en el
equipo local y desactivación/reactivación de la opción. Windows Release 15/15,
Linux WSL 6/6. La suite Linux no pretende validar estas APIs Windows.

Actualización2026-10-19 revision38: M03 completa proveedores soportados;
discos/volúmenes/rendimiento/WMI con positivos nativos, fabricante PnP adicional
con disponibilidad. Segunda muestra55interfaces con send/receive conocidos,
sin generar tráfico;20repeticiones red y20identidad. WINDOWS_NETWORK_IDENTITY.md
documenta alcance/límites. No cobertura física/hotplug/toolchains universal;
red por proceso/sesión queda M02/M06, CI N01 y auditoría N03 separados.

Referencias: [GetIfTable2](https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getiftable2),
[MIB_IF_ROW2](https://learn.microsoft.com/en-us/windows/win32/api/netioapi/ns-netioapi-mib_if_row2),
[direcciones unicast](https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getunicastipaddresstable).
