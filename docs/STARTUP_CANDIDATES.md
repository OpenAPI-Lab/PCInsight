# Candidatos de proceso por metadata de lanzamiento

Subtarea implementada 2026-10-14: discover_startup_process_candidates(snapshot),
API en startup_candidates.hpp. Descubre automáticamente candidatos Linux usando
los dos marcadores documentados por [GIO AppInfo.launch](https://docs.gtk.org/gio/vfunc.AppInfo.launch.html),
sin depender de GIO ni ejecutar su launcher/helper. Se leen exclusivamente mediante
procfs: GIO_LAUNCHED_DESKTOP_FILE y GIO_LAUNCHED_DESKTOP_FILE_PID.

```cpp
pcinsight::SampleOptions options;
options.include_processes = options.include_startup_entries = true;
auto snapshot = monitor.sample(options);
auto report = pcinsight::discover_startup_process_candidates(snapshot);
// Revisar confidence/origin/complete antes de aceptar una asociación.
// observer.register_process(candidate.startup_entry_id, candidate.baseline)
// es una decisión EXPLÍCITA del caller, no ocurre automáticamente.
```

Exige PID del marcador igual al proceso: hijos que heredan la variable se ignoran,
sin sumarles E/S otra vez. Exige path absoluto .desktop EXACTO entre IDs efectivos
proporcionados por snapshot. No compara name/comm, no analiza Exec ni admite
basename como sustituto. Aliases/rutas lexicalmente equivalentes no se adivinan.
Revalida precedencia XDG antes/después; snapshot shadowed o inventario cambiado
-> stale_target y sin candidatos. No relabela un source system como personal.

Procfs genuino y namespace compatible con el caller; FD /proc/PID abierto, UID
del directorio igual a geteuid antes/después y PID/start_ticks comprobados contra
ProcessInfo del snapshot antes/después de leer entorno. No lee entornos de otros
UID incluso si el caller es root. Copia solo path de evidencia coincidente y
baseline de contadores identificado por el lector del núcleo de impacto.
Entorno ajeno/no accesible, proceso desaparecido/cambiado o token ausente ->
unavailable y complete=false; otro UID o evidencia no coincidente -> ignored.
No se emiten/restauran/logean otras variables ni se exponen secretos del entorno.

Marcadores duplicados, PID malformado/fuera de rango, archivo relativo/excesivo
o entorno sin NUL terminador -> unavailable, no asociación inventada. Ausencia de
ambos o de uno -> sin evidencia, no error ni prueba de que la app no arrancó.
No busca rutas de otros sistemas de launch cuando no hay marcadores.

association_support=estimated SIEMPRE. automatic_startup_origin permanece
unknown/unsupported: un lanzamiento manual mediante el mismo .desktop también
puede tener esos marcadores, y el proceso puede fabricarlos/modificarlos. Son
metadata declarada, no certificado autenticado de app/cause/startup ni identidad
global/resistente a todas las carreras de tick. No habilita acciones destructivas,
no valida versión/contenido de archivo al momento del lanzamiento y no es un
helper privilegiado. Observador/inventario NO se actualizan silenciosamente;
Monitor.impact sigue not_measured. Disabled actual tampoco prueba causa anterior.

Opt-in: el caller debe haber solicitado procesos y startup en el snapshot;
listas vacías no certifican ausencia global. complete significa que se evaluó
todo lo PROPORCIONADO dentro de límites, no cobertura de toda la sesión/boot.
error none puede coexistir con unavailable>0/complete=false: consultar ambos.
Límites:100000 objetos por lista de entrada,4096 PIDs visitados, IDs32KiB y8MiB
de IDs proporcionados. Entorno64KiB por proceso; presupuesto8MiB de lectura total
de entorno, incluyendo fallos/oversized (más un byte de probe de exceso). Stat
acotado64KiB; inventarios XDG mantienen sus límites propios. No worker ni plazo
universal de procfs/FS; contextos/entornos son del caller y deben coordinarse.

Verificación Release: Linux30/30, Windows41/41, nuevos headers instalados y
consumidores externos ejecutados. Windows prueba parser portable/unsupported,
no un proveedor nativo equivalente. Linux usa tres hijos C++ propios exec del
mismo test con marcadores preparados antes de exec, archivos XDG /tmp privados:
match exacto directo, PID heredado ignorado, path system shadowed ignorado,
snapshot/token stale rechazado y candidato aceptado explícitamente por observer
sin completar falsamente impacto. Hijos se cierran/recolectan, sin ejecutar Exec
del .desktop ni GIO helper ni app real. Parser prueba límites/duplicados/PIDs/
paths/NUL y descarte de variables ajenas. No validación de launch automático real
de todas las distros; GIO markers no son universales.

Pendiente de impacto automático completo: evidencia fiable de causa de startup,
observación temprana/vida corta, otras fuentes (cgroup/session managers), agregación
sin doble cuenta, persistencia e integración conservadora con inventario.

La integración explícita posterior existe en STARTUP_OVERLAY.md; los candidatos
siguen estimados y no se registran/aplican automáticamente ni prueban causa.

STARTUP_STORE.md incorpora persistencia explícita de capturas cerradas como datos
históricos; no implementa recorder temprano ni certifica esos candidatos.
