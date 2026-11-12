#include "linux_startup_override.hpp"
#include "linux_startup.hpp"
#include <atomic>
#include <cstdlib>
#ifdef __linux__
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace fs = std::filesystem;
std::optional<StartupOverridePlan> plan_startup_override(const std::string& entry,
    const std::vector<XdgAutostartDirectory>& directories) {
    if (entry.empty() || entry.size() > 32768 || entry.find('\0') != entry.npos || directories.size() > 64) return {};
    const auto source = fs::path(entry).lexically_normal();
    if (!source.is_absolute() || source.extension() != ".desktop") return {};
    std::optional<fs::path> personal;
    bool recognized = false;
    for (const auto& dir : directories) {
        const auto path = dir.path.lexically_normal();
        if (!path.is_absolute() || path.string().size() > 32768) return {};
        if (path == source.parent_path()) recognized = true;
        if (dir.personal) {
            if (personal && *personal != path) return {};
            personal = path;
        }
    }
    if (!recognized || !personal) return {};
    for (const auto& dir : directories)
        if (!dir.personal && dir.path.lexically_normal() == *personal) return {}; // ambiguous shared destination
    return StartupOverridePlan{source, *personal / source.filename()};
}
#ifdef __linux__
namespace {
constexpr std::size_t limit = 1048576;
struct Fd { int value{-1}; ~Fd() { if (value >= 0) close(value); } };
ActionResult fail(const char* reason, int code=errno) {
    auto kind=ActionError::system_error;
    if (code==EACCES || code==EPERM) kind=ActionError::permission_denied;
    else if (code==ENOENT) kind=ActionError::not_found;
    else if (code==ELOOP || code==ENOTDIR) kind=ActionError::invalid_argument;
    else if (code==ENOTSUP) kind=ActionError::not_supported;
    return {kind, std::string(reason) + ": " + std::strerror(code)};
}
bool same(const struct stat& a, const struct stat& b) {
    return a.st_dev==b.st_dev && a.st_ino==b.st_ino && a.st_size==b.st_size &&
        a.st_mtim.tv_sec==b.st_mtim.tv_sec && a.st_mtim.tv_nsec==b.st_mtim.tv_nsec &&
        a.st_ctim.tv_sec==b.st_ctim.tv_sec && a.st_ctim.tv_nsec==b.st_ctim.tv_nsec;
}
std::atomic<unsigned long long> counter{};
}
#endif
ActionResult apply_startup_override(const StartupOverridePlan& plan, bool enabled) {
    if (!plan.source.is_absolute() || !plan.destination.is_absolute() || plan.source.extension()!=".desktop" ||
        plan.destination.filename()!=plan.source.filename() || plan.source.string().find('\0')!=std::string::npos ||
        plan.destination.string().find('\0')!=std::string::npos)
        return {ActionError::invalid_argument, "plan de override inválido"};
#ifdef __linux__
    const auto directory=plan.destination.parent_path();
    const auto name=plan.destination.filename().string();
    // Existing custom override wins: never refresh its Exec/metadata from system.
    struct stat target{};
    if (lstat(plan.destination.c_str(), &target)==0) {
        if (!S_ISREG(target.st_mode) || target.st_nlink!=1 || target.st_uid!=geteuid())
            return {ActionError::invalid_argument, "override existente debe ser regular, propio y sin enlaces"};
        Fd dir{open(directory.c_str(), O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
        struct stat metadata{};
        if (dir.value<0 || fstat(dir.value, &metadata)!=0) return fail("inspeccionar directorio personal");
        if (metadata.st_uid!=geteuid() || (metadata.st_mode&0022))
            return {ActionError::permission_denied, "directorio personal no es propio/privado frente a escritura externa"};
        const auto result=set_linux_startup_enabled(plan.destination.string(), enabled);
        if (!result) return result;
        return {ActionError::none, "flags personales actualizados: " + plan.destination.string()};
    }
    if (errno!=ENOENT) return fail("inspeccionar override personal");
    Fd source{open(plan.source.c_str(), O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
    if (source.value<0) return fail("leer origen startup");
    struct stat before{}, after{};
    if (fstat(source.value,&before)!=0) return fail("stat origen startup");
    if (!S_ISREG(before.st_mode) || before.st_size<0 || before.st_size>static_cast<off_t>(limit))
        return {ActionError::invalid_argument, "origen debe ser regular de máximo 1 MiB"};
    std::string input; char buffer[4096];
    for (;;) {
        const auto n=read(source.value,buffer,sizeof buffer);
        if (n<0) { if (errno==EINTR) continue; return fail("leer origen startup"); }
        if (!n) break;
        if (static_cast<std::size_t>(n)>limit-input.size()) return {ActionError::invalid_argument,"origen excedió límite"};
        input.append(buffer,static_cast<std::size_t>(n));
    }
    if (!read_xdg_startup(input)) return {ActionError::invalid_argument,"origen startup no legible como Desktop Entry"};
    const auto rewritten=rewrite_xdg_startup(input,enabled);
    if (!rewritten) return {ActionError::invalid_argument,"origen startup ambiguo/malformado"};
    if (fstat(source.value,&after)!=0) return fail("revalidar origen startup");
    if (!same(before,after)) return {ActionError::stale_target,"origen cambió durante lectura"};
    std::error_code error;
    const bool created=fs::create_directories(directory,error);
    if (error) return fail("crear directorio personal", error.value());
    Fd dir{open(directory.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
    if (dir.value<0) return fail("abrir directorio personal");
    if (fstat(dir.value,&after)!=0) return fail("stat directorio personal");
    if (created && after.st_uid==geteuid()) {
        if (fchmod(dir.value,0700)!=0 || fstat(dir.value,&after)!=0) return fail("permisos directorio personal nuevo");
    }
    if (after.st_uid!=geteuid() || (after.st_mode&0022))
        return {ActionError::permission_denied,"directorio personal no es propio/seguro para escritura"};
    if (flock(dir.value,LOCK_EX|LOCK_NB)!=0)
        return errno==EWOULDBLOCK ? ActionResult{ActionError::stale_target,"otro editor mantiene el directorio"} : fail("lock override");
    if (fstatat(dir.value,name.c_str(),&target,AT_SYMLINK_NOFOLLOW)==0)
        return {ActionError::stale_target,"override apareció durante preparación; recargar inventario"};
    if (errno!=ENOENT) return fail("revalidar destino personal");
    Fd temporary;
    std::string temp;
    for (unsigned attempt=0; attempt<128; ++attempt) {
        temp=".pcinsight-override-"+std::to_string(getpid())+"-"+std::to_string(counter.fetch_add(1));
        temporary.value=openat(dir.value,temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
        if (temporary.value>=0) break;
        if (errno!=EEXIST) return fail("crear temporal personal");
    }
    if (temporary.value<0) return {ActionError::system_error,"límite de colisiones de temporales"};
    struct Cleanup { int fd; std::string name; bool active{true}; ~Cleanup(){ if(active)unlinkat(fd,name.c_str(),0); } } cleanup{dir.value,temp};
    for (std::size_t offset=0; offset<rewritten->size();) {
        const auto n=write(temporary.value,rewritten->data()+offset,rewritten->size()-offset);
        if (n<0 && errno==EINTR) continue;
        if (n<=0) return fail("escribir override",n<0?errno:EIO);
        offset+=static_cast<std::size_t>(n);
    }
    if (fchmod(temporary.value,0600)!=0 || fsync(temporary.value)!=0) return fail("sincronizar permisos/datos personales");
    const auto completed=temporary.value; temporary.value=-1;
    if (close(completed)!=0) return fail("cerrar temporal personal");
    if (fstat(source.value,&after)!=0) return fail("revalidar origen antes de commit");
    if (!same(before,after)) return {ActionError::stale_target,"origen cambió antes de publicar copia"};
    if (lstat(plan.source.c_str(),&after)!=0) return fail("revalidar ruta del origen");
    if (!same(before,after)) return {ActionError::stale_target,"ruta del origen cambió antes de publicar copia"};
    // Atomic no-replace publication; a concurrent destination is NEVER overwritten.
    if (linkat(dir.value,temp.c_str(),dir.value,name.c_str(),0)!=0)
        return errno==EEXIST ? ActionResult{ActionError::stale_target,"destino apareció; sin sobrescritura"} : fail("publicar override sin reemplazo");
    if (unlinkat(dir.value,temp.c_str(),0)!=0) return fail("override aplicado; temporal propio no eliminado");
    cleanup.active=false;
    if (fsync(dir.value)!=0) return fail("override aplicado; sincronización de directorio falló");
    return {ActionError::none,"override personal creado: "+plan.destination.string()};
#else
    (void)enabled; return {ActionError::not_supported,"override XDG requiere Linux"};
#endif
}
ActionResult set_linux_startup_for_current_user(const std::string& entry,bool enabled) {
#ifdef __linux__
    try {
        const auto directories=native_xdg_autostart_directories();
        const auto* system_env=std::getenv("XDG_CONFIG_DIRS");
        const auto system_directories=resolve_xdg_autostart("","",system_env?system_env:"");
        for (const auto& personal:directories) if (personal.personal)
            for (const auto& system:system_directories) if (personal.path==system.path)
                return {ActionError::invalid_argument,"directorio personal coincide con configuración del sistema"};
        const auto plan=plan_startup_override(entry,directories);
        if (!plan) return {ActionError::invalid_argument,"entrada no pertenece al contexto XDG o no hay directorio personal"};
        return apply_startup_override(*plan,enabled);
    } catch (const std::invalid_argument&) {
        return {ActionError::invalid_argument,"contexto XDG inválido/excesivo"};
    }
#else
    (void)entry;(void)enabled; return {ActionError::not_supported,"override XDG requiere Linux"};
#endif
}
ActionResult reset_linux_startup_for_current_user(const std::string& entry) {
#ifdef __linux__
    try {
        const auto directories=native_xdg_autostart_directories();
        const auto* env=std::getenv("XDG_CONFIG_DIRS");
        const auto systems=resolve_xdg_autostart("","",env?env:"");
        const auto plan=plan_startup_override(entry,directories);
        if (!plan || plan->source!=plan->destination)
            return {ActionError::invalid_argument,"reset requiere la ruta personal seleccionada"};
        for (const auto& system:systems) if (system.path==plan->destination.parent_path())
            return {ActionError::invalid_argument,"directorio personal coincide con system"};
        const auto name=plan->destination.filename().string();
        Fd dir{open(plan->destination.parent_path().c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW)};
        struct stat metadata{};
        if (dir.value<0 || fstat(dir.value,&metadata)!=0) return fail("abrir directorio personal para reset");
        if (metadata.st_uid!=geteuid() || (metadata.st_mode&0022))
            return {ActionError::permission_denied,"directorio personal no es propio/seguro"};
        if (flock(dir.value,LOCK_EX|LOCK_NB)!=0)
            return errno==EWOULDBLOCK ? ActionResult{ActionError::stale_target,"otro editor mantiene el directorio"} : fail("lock reset");
        Fd personal{openat(dir.value,name.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK)};
        struct stat before{}, after{};
        if (personal.value<0 || fstat(personal.value,&before)!=0) return fail("inspeccionar override para reset");
        if (!S_ISREG(before.st_mode) || before.st_nlink!=1 || before.st_uid!=geteuid())
            return {ActionError::invalid_argument,"override debe ser regular, propio y sin enlaces"};
        if (fsync(personal.value)!=0) return fail("sincronizar datos del respaldo antes de reset");
        // First existing system entry is authoritative, including malformed entries.
        Fd source;
        fs::path system_path;
        struct stat system_before{};
        for (const auto& system:systems) {
            system_path=system.path/name;
            if (lstat(system_path.c_str(),&system_before)!=0) {
                if (errno==ENOENT) continue;
                return fail("buscar contraparte system");
            }
            source.value=open(system_path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK);
            if (source.value<0 || fstat(source.value,&system_before)!=0) return fail("abrir contraparte system");
            break;
        }
        if (source.value<0) return {ActionError::not_found,"no existe contraparte system; no se retira app personal"};
        if (!S_ISREG(system_before.st_mode) || system_before.st_size<0 || system_before.st_size>static_cast<off_t>(limit))
            return {ActionError::invalid_argument,"contraparte system debe ser regular de máximo 1 MiB"};
        std::string content; char buffer[4096];
        for (;;) {
            const auto n=read(source.value,buffer,sizeof buffer);
            if (n<0) { if(errno==EINTR)continue; return fail("leer contraparte system"); }
            if (!n) break;
            if (static_cast<std::size_t>(n)>limit-content.size()) return {ActionError::invalid_argument,"contraparte excedió límite"};
            content.append(buffer,static_cast<std::size_t>(n));
        }
        if (!read_xdg_startup(content) || !rewrite_xdg_startup(content,true))
            return {ActionError::invalid_argument,"contraparte system malformada/ambigua"};
        if (fstat(source.value,&after)!=0) return fail("revalidar contraparte system");
        if (!same(system_before,after) || lstat(system_path.c_str(),&after)!=0 || !same(system_before,after))
            return {ActionError::stale_target,"contraparte system cambió antes de reset"};
        if (fstat(personal.value,&after)!=0 || !same(before,after) ||
            fstatat(dir.value,name.c_str(),&after,AT_SYMLINK_NOFOLLOW)!=0 || !same(before,after))
            return {ActionError::stale_target,"override cambió antes de reset"};
        // One atomic move, no replace. No link/unlink fallback with a crash window.
        for (unsigned attempt=0;attempt<128;++attempt) {
            const auto backup=".pcinsight-reset-"+std::to_string(getpid())+"-"+std::to_string(counter.fetch_add(1))+".backup";
#ifdef SYS_renameat2
            if (syscall(SYS_renameat2,dir.value,name.c_str(),dir.value,backup.c_str(),1U)!=0) { // RENAME_NOREPLACE
                if(errno==EEXIST)continue;
                if(errno==ENOSYS || errno==EINVAL || errno==EOPNOTSUPP)
                    return {ActionError::not_supported,"reset requiere renameat2 NOREPLACE en este kernel/filesystem"};
                return fail("archivar override sin reemplazo");
            }
#else
            return {ActionError::not_supported,"reset requiere renameat2 NOREPLACE"};
#endif
            ActionResult result{ActionError::none,"override retirado; recargar inventario"};
            result.startup_backup_path=(plan->destination.parent_path()/backup).string();
            if (fsync(dir.value)!=0) {
                result.error=ActionError::system_error;
                result.message="reset aplicado; sincronización de directorio falló: "+std::string(std::strerror(errno));
            }
            return result;
        }
        return {ActionError::system_error,"límite de colisiones de respaldos"};
    } catch (const std::invalid_argument&) {
        return {ActionError::invalid_argument,"contexto XDG inválido/excesivo"};
    }
#else
    (void)entry; return {ActionError::not_supported,"reset XDG requiere Linux"};
#endif
}
}
