# Condiciones estándar XDG de startup

Unidad funcional terminada: `StartupEntry::xdg_standard_eligible`, Metric<bool>
independiente de `status` (flags enabled/disabled). Se evalúa en el inventario
Linux con el contexto del proceso llamador. No ejecuta TryExec ni Exec.

Una entrada enabled puede devolver elegibilidad false porque OnlyShowIn excluye
al escritorio actual. Activar flags no elimina esas condiciones. Elegibilidad
ausente no equivale a false: el GUI debe mostrar desconocido y la nota del Metric.

## Criterios evaluados

- Hidden=true: false. Los tombstones Hidden=true siguen enmascarando
  archivos inferiores aunque no tengan metadata de aplicación completa.
- Type distinto de Application: false. Type/Name requeridos ausentes y falta de
  Exec sin DBusActivatable=true producen unknown; no se valida toda la gramática Exec.
- OnlyShowIn/NotShowIn en el grupo principal, nunca en Desktop Action. Ambos a
  la vez o duplicados: unknown. Comparación case-sensitive, con listas separadas
  por `;`, escapes string/list y XDG_CURRENT_DESKTOP separado por `:`.
- Con condiciones de escritorio, un contexto ausente/vacío no se presume GNOME
  ni KDE: unknown. Una condición conocida que excluye al caller produce false.
- TryExec no vacío: path absoluto o nombre simple buscado en PATH. No se analiza
  como comando ni se divide en argumentos. Se decodifican escapes de string;
  una ruta relativa con `/` es inválida/unknown. Un punto y coma es parte del
  nombre de archivo, no código de shell. No se expande `$VAR` ni `~`.

La lectura nativa comprueba archivo regular y faccessat(X_OK, AT_EACCESS) con
credenciales efectivas. Symlink a un archivo ejecutable puede cumplir TryExec;
directorio o falta de acceso/permiso no lo cumplen. Es una consulta de acceso,
no validación del formato ELF/script ni garantía de exec exitoso. Los archivos
pueden cambiar después de la consulta.

PATH se captura por evaluación de inventario; componentes vacíos/relativos
se resuelven con cwd capturado. Si PATH está ausente, no se inventa una lista
de directorios por defecto. Fallos no concluyentes producen unknown; encontrar
un candidato accesible permite true aunque otro directorio no se haya evaluado.
Una condición conocida false puede resolver el resultado aunque falte el contexto
del escritorio. Un TryExec desconocido conserva su estado de soporte/nota.

Límites: archivo 1 MiB (lector existente), cadenas decodificadas 32 KiB,
4096 elementos por lista/PATH y contexto desktop 32 KiB. Casos excesivos o
malformados no se presentan como elegibles. Las listas usan búsqueda por conjunto
para evitar comparar todas las combinaciones de identificadores.

## Alcance exacto

True significa que los **criterios comprobados** pasan en este contexto, no que
la app realmente arrancará o está running. No evalúa condiciones GNOME/KDE
especiales, políticas del escritorio, servicios D-Bus disponibles, NoDisplay
como filtro de menú, validez completa de Desktop Entry/Exec ni startup impact.
NoDisplay no se transforma por sí solo en disabled de autostart.
X-GNOME-Autostart-enabled=false sigue reflejado en el status de configuración
existente, pero no se convierte en una exclusión XDG universal: otros escritorios
pueden ignorar esa extensión. El campo nuevo solo certifica criterios estándar.

La API posterior autostart_condition_satisfied evalúa por separado las condiciones
vendor de existencia de archivo; ver XDG_FILE_CONDITION.md. No cambia el contrato
de xdg_standard_eligible.

El entorno/credenciales del proceso pueden diferir de la sesión gráfica. WSL
sin XDG_CURRENT_DESKTOP no certifica condiciones GNOME por el nombre de la distro.
No se eleva ni se cambia el entorno del consumidor al recopilar información.
Para otras plataformas el nuevo campo permanece unsupported si el proveedor
no lo calcula. Es un campo nuevo en una struct pública 0.x: reconstruir biblioteca
y consumidores con headers compatibles.

## Verificación de cierre

2026-10-14: Linux Release 15/15, Windows 27/27. Windows verifica lógica portable
con contexto/consulta inyectados, no permisos ni desktop nativo Linux.
Instalación de ambos paquetes Release y reconstrucción/ejecución de consumidores
externos verificadas tras la corrección final de la extensión GNOME.

Tests: inclusión/exclusión y múltiples desktops, case, campo vacío, escapes,
lista inválida, duplicados/both fields, condiciones de otro grupo ignoradas,
missing context, disabled, query desconocida y parámetros TryExec preservados.
En Linux se crean archivos privados con/sin X_OK y se comprueban path absoluto,
PATH, inexistente, directorio y ruta relativa inválida sin ejecutar el candidato.

Monitor completo, en entorno privado del proceso de prueba y con autostart
temporal, mantiene status=enabled y xdg_standard_eligible=false cuando una
entrada OnlyShowIn=KDE se consulta con XDG_CURRENT_DESKTOP=GNOME.
Los entornos se restauran al terminar. No se cambia startup real ni se inicia
una aplicación para probar esta unidad.

Referencias primarias: [condiciones Autostart XDG](https://specifications.freedesktop.org/autostart/latest/),
[claves Desktop Entry](https://specifications.freedesktop.org/desktop-entry/latest/recognized-keys.html),
[tipos/escapes de valores](https://specifications.freedesktop.org/desktop-entry/latest/value-types.html).
