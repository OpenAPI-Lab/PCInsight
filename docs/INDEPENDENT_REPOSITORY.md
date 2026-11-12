# Publicar PCInsight como repositorio independiente

La raíz del repositorio debe ser esta carpeta, la que contiene `CMakeLists.txt`,
`include/pcinsight`, `src`, `README.md` y `LICENSE`. No uses como raíz el workspace
ni el proyecto grande del GUI. No hay dependencia de ese proyecto consumidor.

## Antes del primer commit

1. Revisa [soporte y faltantes](SUPPORT_STATUS.md), [uso desde C++](GETTING_STARTED_CPP.md), `CONTRIBUTING.md` y `LICENSE`.
2. Comprueba que no haya builds, SDK binarios, logs privados, credenciales ni datos de hardware sin revisar. El `.gitignore` ayuda, pero no elimina archivos ya rastreados.
3. Revisa la lista de archivos exacta con `git status --short` y, tras preparar el índice, `git diff --cached --stat` y `git diff --cached`. No subas el workspace entero.
4. La documentación histórica incluye referencias a pruebas locales y registros internos no distribuidos; no se anuncian como resultados de GitHub Actions.

Desde esta raíz, cuando quieras crear el primer commit:

```sh
git rev-parse --show-toplevel
git status --short
git add .
git diff --cached --stat
git diff --cached
git commit -m "Initial PCInsight SDK: Linux implementation, tests and documented limitations"
```

Si todavía no es un repositorio, ejecuta `git init` antes. Si Git solicita nombre
y correo para el commit, configura los que quieras usar para este repositorio;
no hace falta cambiar la identidad global de otros proyectos.

El commit es local. Para publicar, crea o elige un repositorio remoto separado,
confirma su visibilidad y conecta su URL. No se proporciona una URL inventada
ni se añade un remoto automáticamente. Revisa los resultados de CI cuando lo
subas; la verificación local no predice todos los resultados del runner remoto.

La licencia MIT se limita al material de PCInsight: no subas el código del GUI
ni otros proyectos para conceder esa licencia sobre ellos. Si más adelante
consumes PCInsight como submódulo o dependencia desde el proyecto grande,
conserva ambos repositorios y su configuración separados.

