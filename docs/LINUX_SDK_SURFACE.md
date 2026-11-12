# Superficie del SDK Linux 0.2.0

Inventario por encabezado, no una promesa de ABI 1.0 ni una lista de símbolos ya congelados. **Base candidata** significa que el GUI puede empezar aquí. Los campos específicos de plataforma, valores heredados y extensiones que aparezcan en esos encabezados conservan sus propios límites. Incluir un encabezado no concede permisos ni garantiza disponibilidad.

| Categoría | Encabezados bajo `pcinsight/` | Regla de integración |
| --- | --- | --- |
| Base candidata: lectura | `monitor.hpp`, `sampler.hpp`, `types.hpp`, `metric.hpp`, `app_history.hpp` | Usar informes con disponibilidad. No promover valores legacy a mediciones conocidas. |
| Base candidata: acciones | `actions.hpp`, `audit.hpp` | Invocar solo por solicitud explícita. Auditoría no equivale a autorización; no aplicar fallback más permisivo. |
| Integración del SDK | `sdk_headers.hpp` | Generado al compilar. Comprobación de enlace opcional; no certifica una ABI completa. |
| Persistencia y acciones especializadas | `history_store.hpp`, `linux_history_store.hpp`, `startup_store.hpp`, `startup.hpp`, `account_management.hpp`, `linux_session_control.hpp`, `linux_startup_publisher.hpp` | Elegir formato y ámbito explícitamente. No confundir cerrar sesión con desconectar ni administrar cuentas con elevar permisos. |
| Inventario y consultas avanzadas Linux | `linux_drm.hpp`, `linux_filesystem_capacity.hpp`, `linux_network_inventory.hpp`, `linux_network_addresses.hpp`, `linux_tcp.hpp` | Dependientes de proveedor, fuentes y permisos; conservar diagnósticos y alcance. |
| Captura y atribución avanzadas Linux | `linux_network_admission.hpp`, `linux_notifications.hpp`, `linux_process_network_selection.hpp`, `linux_process_network.hpp`, `linux_user_network.hpp` | Opt-in según API. No extrapolar a tráfico global, a apps no observadas ni a historia previa. |
| Supervisores alternativos Linux | `linux_openrc.hpp`, `linux_openrc_services.hpp`, `linux_openrc_control.hpp`, `linux_runit.hpp`, `linux_runit_services.hpp`, `linux_s6.hpp`, `linux_s6_services.hpp`, `linux_services.hpp`, `linux_sysv.hpp`, `linux_sysv_services.hpp` | Proyección y control son distintos. Usar el proveedor seleccionado; consultar [aceptación](LINUX_K03_ACCEPTANCE.md). No habilitar scripts SysV por analogía. |
| Observación avanzada de inicio | `startup_birth_impact.hpp`, `startup_birth_observer.hpp`, `startup_birth_overlay.hpp`, `startup_candidates.hpp`, `startup_executable_matches.hpp`, `startup_impact.hpp`, `startup_observer.hpp`, `startup_overlay.hpp`, `startup_recorder.hpp` | Una asociación o medición no certifica que el SO causó el inicio. No prometer cobertura completa del arranque. |
| Opt-in con riesgo explícito | `linux_efficiency.hpp` | La vía no verificada no es un sustituto automático de la acción genérica rechazada. |
| Windows fuera del alcance Linux | `windows_amd_runtime_memory.hpp`, `windows_history_store.hpp`, `windows_network_identity.hpp`, `windows_services.hpp`, `windows_startup_file_control.hpp` | Su presencia en el SDK no indica un proveedor Linux. La consulta AMD experimental conserva su contrato independiente. |

Las categorías avanzadas no están congeladas por este inventario; tampoco se
eliminan ni cambian. Antes de fijar 1.0, definir el subconjunto de símbolos y
perfiles soportados y verificarlo según la [política de versiones](SDK_VERSION_POLICY.md).

## Consumidor mínimo de lectura verificado

El [consumidor de prueba](../tests/linux_abi/gui_read_consumer.cpp) enlaza el
marcador, el constructor/destructor de `Monitor`, `Monitor::sample`, la
proyección `app_history_view(SystemSnapshot)` y el constructor/destructor de
`Sampler`, `start`, `stop`, `running`, `last_error`, `latest_timed` y
`timed_history`. Verifica una ventana coherente sin requerir que cada campo
tenga datos. No ejecuta acciones ni activa captura privilegiada. Los métodos
no usados y la compatibilidad de objetos previamente compilados quedan fuera
de esta prueba.

Desde la raíz de PCInsight, usando un prefijo del SDK 0.2.0 instalado:

```sh
g++ -std=c++20 -O2 -DNDEBUG -I"$SDK/include" \
  tests/linux_abi/gui_read_consumer.cpp "$SDK/lib/libpcinsight.a" \
  -pthread -o /tmp/pcinsight-gui-read-test
/tmp/pcinsight-gui-read-test
```

Perfil comprobado: Linux x86-64 en Ubuntu WSL con GCC 13.3 y libstdc++, biblioteca
estática Release, consumidor recompilado. No requiere CMake en el consumidor.
Revisión 196: se compilaron también los consumidores de lectura y de resultados
con Clang 18.1.3 y `-stdlib=libstdc++` contra la biblioteca estática GCC 13.3
de revisión 194. Ambos ejecutaron correctamente diez veces en Ubuntu WSL x86-64.
Es evidencia de ese par de herramientas para esas interfaces, no un build de
la biblioteca con Clang, un ensayo con libc++ ni una promesa de ABI 1.0.

Revisión 197: una segunda compilación local produjo la biblioteca estática
Release con Clang 18.1.3 en Ubuntu WSL x86-64; ambos consumidores Clang con
libstdc++ enlazaron y se ejecutaron contra ese SDK. Fue una prueba de biblioteca
y consumidores, no la suite completa ni un ensayo con libc++.

Revisión 198: la suite Linux Release con Clang 18.1.3 pasó 197/197 en el
mismo entorno, con el recorder opcional desactivado. Se retiró una constante
interna sin uso; no cambió el contrato público.
