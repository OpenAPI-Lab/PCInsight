# Manage user accounts: integración del GUI (J06)

`pcinsight/account_management.hpp` proporciona AccountManagementIntegration.
Es el punto de integración para que el GUI consumidor navegue a su gestor de
cuentas; **no incluye un gestor de cuentas nativo ni una pantalla de ajustes**.
No crea/borra usuarios, cambia credenciales, desconecta sesiones ni eleva
privilegios. No busca/instala aplicaciones ni adivina comandos de GNOME/KDE/
Windows. Una aplicación puede abrir su propia vista, sin herramienta externa.

## Contrato

Se configura explícitamente una instancia con dos callbacks de confianza:
consulta read-only Metric<bool> y apertura ActionResult. No hay registro global,
autodetección ni ejecución al construir la instancia o muestrear el Monitor.
Sin ambos callbacks, la capacidad y la acción quedan indisponibles.

`capability()` normaliza ausencia/estimated/soporte inválido, notas con NUL o
excesivas y excepciones. Permission_required conserva el motivo y elimina un
valor contradictorio. Available+false es indisponibilidad conocida. Solo
available+true permite request_open; cada llamada vuelve a consultar antes de
invocar el opener, aunque el GUI hubiera consultado previamente.

`AccountManagementOpenResult.host_invoked` es false cuando no se llamó al
opener; true incluso si devuelve error o lanza excepción. En ese último caso
la UI puede haberse abierto antes del fallo: no reintentar automáticamente.
El error concreto del host se conserva. Éxito significa aceptación del host,
no prueba de ventana visible ni de modificaciones posteriores en cuentas.

Los callbacks se ejecutan en el thread llamador. El host gestiona el event
loop, marshalling al thread GUI, sincronización y referencias capturadas.
No hay background, timeout duro ni cancelación de callbacks. Son código de
confianza, no un sandbox: el host debe cumplir el contrato de solo navegación,
sin acciones CRUD/elevación implícitas. Si el host usa launch_process, hereda
sus validaciones y responsabilidad de reap de hijos Linux; process_id no es
identidad duradera. No se añaden targets/rutas/comandos aportados por usuarios
a esta API y no se modifica el vtable de SystemActions/AuditedActions.

Ejemplo de conexión a una vista que ya pertenece al GUI consumidor:

```cpp
pcinsight::AccountManagementIntegration accounts(
    [&gui] { return pcinsight::Metric<bool>{gui.has_account_view(),
        pcinsight::Support::available, "account view supplied by host"}; },
    [&gui] {
        gui.request_account_view(); // El host despacha a su event loop.
        return pcinsight::ActionResult{pcinsight::ActionError::none,
            "navigation request accepted"};
    });
// Solo cuando el usuario selecciona explícitamente Manage user accounts:
const auto receipt = accounts.request_open();
```

`gui` es pseudocódigo del consumidor, no una clase incluida en PCInsight.
La instancia no debe sobrevivir a las referencias que captura.

## Verificación y alcance del cierre

tests/account_management.cpp prueba aceptación, ausencia/false/estimated,
denegación, configuración incompleta, cambios de capacidad, notas inválidas,
excepciones de ambas fases y errores del opener sin reintentos. También prueba
el recorrido público con SystemActions::launch_process en Linux/Windows: un
hijo privado escribe un marcador en un directorio nuevo; el padre comprueba
aceptación y Linux hace waitpid. **Ese hijo no es un gestor de cuentas real**.
No se abren ajustes del escritorio ni se cambian usuarios reales.

El consumidor CMake externo incluye/enlaza la nueva cabecera instalada y
verifica que el estado por defecto no ejecuta nada. Nueva API adicional; sin
layout público existente/ABI virtual/historial alterados.

J06 exige definir una acción/integración segura para acceder a gestión de
cuentas y probar capacidades/errores, no construir CRUD. Esta unidad cumple
ese alcance como integración explícita del host; no se cierra por devolver
unsupported. El soporte efectivo depende de que el GUI aporte su gestor.
No se afirma un proveedor automático/universal de paneles de cada escritorio.

Verificación2026-10-19: Release Linux102/102 y Windows116/116; builds sin warnings.
Ambos paquetes reinstalados y consumidores CMake externos recompilados/relinkados/
ejecutados exit0. Logs work/account-management-suite-{linux,windows}.log,
account-management-native-* y build/install/consumer con el mismo prefijo.
N03 previo sigue abierto: la regresión verde no demuestra corregir sus fallos
intermitentes históricos. No hubo pruebas sobre cuentas/gestores reales.
