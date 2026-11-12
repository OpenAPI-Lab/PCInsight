#include "linux_startup_eligibility.hpp"
#include "linux_startup.hpp"
#include "linux_startup_inventory.hpp"
#include "linux_startup_gsettings.hpp"
#include "linux_startup_session.hpp"
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#ifdef __linux__
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace pcinsight::detail {
namespace {
Metric<bool> unknown(const char* reason) { return {std::nullopt, Support::unsupported, reason}; }
Metric<bool> known(bool value, const char* reason) { return {value, Support::available, reason}; }
std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.remove_suffix(1);
    return s;
}
std::optional<std::vector<std::string>> decode(std::string_view raw, bool list) {
    std::vector<std::string> parts; std::string part;
    for (std::size_t i = 0; i < raw.size(); ++i) {
        char c = raw[i];
        if (c == '\\') {
            if (++i == raw.size()) return {};
            switch (raw[i]) {
                case 's': part += ' '; break;
                case 'n': part += '\n'; break;
                case 't': part += '\t'; break;
                case 'r': part += '\r'; break;
                case '\\': part += '\\'; break;
                case ';': if (!list) return {}; part += ';'; break;
                default: return {};
            }
        } else if (list && c == ';') {
            if (part.empty()) return {};
            parts.push_back(std::move(part)); part.clear();
        } else part += c;
        if (part.size() > 32768 || parts.size() > 4096) return {};
    }
    if (!part.empty() || !list) parts.push_back(std::move(part));
    if (parts.size() > 4096) return {};
    return parts;
}
}
Metric<bool> xdg_standard_eligibility(std::string_view text, const XdgEligibilityContext& context) {
    const auto entry = read_xdg_startup(text);
    if (!entry) return unknown("XDG entry malformed or unreadable");
    std::map<std::string, std::string> keys;
    bool main = false;
    while (!text.empty()) {
        const auto end = text.find('\n'); auto line = text.substr(0, end);
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        line = trim(line);
        if (!line.empty() && line.front() == '[') main = line == "[Desktop Entry]";
        if (main && !line.empty() && line.front() != '#' && line.front() != '[') {
            const auto equals = line.find('=');
            if (equals != line.npos) {
                const std::string key(trim(line.substr(0, equals)));
                if (key == "Hidden" || key == "OnlyShowIn" || key == "NotShowIn" || key == "TryExec" || key == "Type" || key == "DBusActivatable")
                    if (!keys.emplace(key, trim(line.substr(equals + 1))).second) return unknown("duplicate XDG condition");
            }
        }
        if (end == text.npos) break;
        text.remove_prefix(end + 1);
    }
    if (keys.count("Hidden") && keys.at("Hidden") == "true") return known(false, "XDG Hidden disables this entry");
    if (keys.count("OnlyShowIn") && keys.count("NotShowIn")) return unknown("OnlyShowIn and NotShowIn cannot coexist");
    if (!keys.count("Type")) return unknown("missing required desktop entry Type");
    if (keys.at("Type") != "Application") return known(false, "not an Application desktop entry");
    if (entry->name.empty()) return unknown("missing required Name");
    if (keys.count("DBusActivatable") && keys.at("DBusActivatable") != "true" && keys.at("DBusActivatable") != "false")
        return unknown("invalid DBusActivatable boolean");
    if (entry->command.empty() && (!keys.count("DBusActivatable") || keys.at("DBusActivatable") != "true"))
        return unknown("missing Exec without DBus activation");
    bool unresolved = false;
    Support executable_support = Support::available;
    for (const auto* key : {"OnlyShowIn", "NotShowIn"}) if (keys.count(key)) {
        auto list = decode(keys.at(key), true);
        if (!list) return unknown("invalid desktop environment list");
        const std::set<std::string> allowed(list->begin(), list->end());
        if (!context.desktop || context.desktop->empty()) { unresolved = true; continue; }
        if (context.desktop->size() > 32768 || context.desktop->find('\0') != std::string::npos) return unknown("invalid desktop context");
        bool matches = false;
        std::string_view desktops(*context.desktop);
        while (!desktops.empty()) {
            const auto end = desktops.find(':'); const auto candidate = desktops.substr(0, end);
            if (allowed.count(std::string(candidate))) matches = true;
            if (end == desktops.npos) break;
            desktops.remove_prefix(end + 1);
        }
        if ((std::string_view(key) == "OnlyShowIn" && !matches) || (std::string_view(key) == "NotShowIn" && matches))
            return known(false, "XDG desktop environment condition excludes caller");
    }
    if (keys.count("TryExec") && !keys.at("TryExec").empty()) {
        const auto value = decode(keys.at("TryExec"), false);
        if (!value || value->size() != 1) return unknown("invalid TryExec string escape");
        if (!context.executable) return unknown("TryExec cannot be checked in this context");
        const auto check = context.executable(value->front());
        if (check.value && !*check.value) return check;
        if (!check.value) return check;
        executable_support = check.support;
    }
    if (unresolved) return unknown("XDG_CURRENT_DESKTOP not known for desktop condition");
    auto result = known(true, "standard XDG flags/desktop/TryExec conditions pass; not a launch guarantee");
    result.support = executable_support;
    return result;
}
Metric<bool> autostart_condition_satisfied(std::string_view text, const XdgEligibilityContext& context) {
    if (!read_xdg_startup(text)) return unknown("AutostartCondition entry malformed/unreadable");
    std::optional<std::string> raw;
    bool main=false;
    while (!text.empty()) {
        const auto end=text.find('\n'); auto line=text.substr(0,end);
        if (!line.empty() && line.back()=='\r') line.remove_suffix(1);
        line=trim(line);
        if (!line.empty() && line.front()=='[') main=line=="[Desktop Entry]";
        if (main && !line.empty() && line.front()!='#' && line.front()!='[') {
            const auto equals=line.find('=');
            if (equals!=line.npos && trim(line.substr(0,equals))=="AutostartCondition") {
                if (raw) return unknown("duplicate AutostartCondition");
                raw=std::string(trim(line.substr(equals+1)));
            }
        }
        if (end==text.npos) break;
        text.remove_prefix(end+1);
    }
    if (!raw) return known(true,"no AutostartCondition constraint; not a launch guarantee");
    const auto decoded=decode(*raw,false);
    if (!decoded || decoded->size()!=1) return unknown("invalid AutostartCondition string escape");
    const auto& condition=decoded->front();
    const auto separator=condition.find_first_of(" \t");
    if (separator==condition.npos) return unknown("AutostartCondition requires a kind and operand");
    std::string kind=condition.substr(0,separator);
    for(auto& c:kind) if(c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
    if(kind=="gnome3") {
        auto rest=trim(std::string_view(condition).substr(separator));
        const auto split=rest.find_first_of(" \t");
        if(split==rest.npos) return unknown("GNOME3 condition requires predicate and session name");
        std::string predicate(rest.substr(0,split));
        for(auto& c:predicate) if(c>='A'&&c<='Z')c=static_cast<char>(c-'A'+'a');
        const auto target=trim(rest.substr(split));
        if((predicate!="if-session" && predicate!="unless-session") || target.empty() || target.size()>1024 ||
            target.find('\0')!=target.npos || target.find_first_of("\n\r\t")!=target.npos)
            return unknown("invalid GNOME3 session condition");
        if(!context.session_name) return unknown("GNOME session name provider unavailable");
        auto session=context.session_name();
        if(!session.value) return {std::nullopt,session.support,session.note};
        if(session.value->empty() || session.value->size()>1024 || session.value->find('\0')!=std::string::npos)
            return unknown("invalid GNOME session name context");
        for(const auto c:*session.value) if(static_cast<unsigned char>(c)<32 || c==127)
            return unknown("invalid GNOME session name controls");
        const bool matches=*session.value==target; // case-sensitive NAME, not XDG desktop
        return {predicate=="if-session"?matches:!matches,session.support,
            "GNOME3 session name predicate sampled; not launch guarantee"};
    }
    if (kind=="gsettings") {
        auto parameters=trim(std::string_view(condition).substr(separator));
        const auto split=parameters.find_first_of(" \t");
        if(split==parameters.npos) return unknown("GSettings condition requires schema and key");
        const std::string schema(parameters.substr(0,split));
        const std::string key(trim(parameters.substr(split)));
        auto valid=[](const std::string& s) {
            if(s.empty() || s.size()>1024) return false;
            for(const auto c:s) if(!((c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='.' || c=='-' || c=='_')) return false;
            return true;
        };
        if(!valid(schema) || !valid(key)) return unknown("invalid GSettings condition identifiers");
        if(!context.setting_boolean) return unknown("GSettings condition provider unavailable");
        return context.setting_boolean(schema,key);
    }
    if (kind!="if-exists" && kind!="unless-exists") return unknown("AutostartCondition kind has no provider yet");
    auto operand=std::string_view(condition).substr(separator);
    while (!operand.empty() && (operand.front()==' ' || operand.front()=='\t')) operand.remove_prefix(1);
    if (operand.empty() || operand.size()>32768 || operand.find('\0')!=operand.npos ||
        operand.find_first_of("\n\r\t")!=operand.npos)
        return unknown("invalid AutostartCondition file operand");
    // Literal file path: no shell, expansion, quoting, or tokenization of spaces.
    auto path=std::filesystem::path(operand);
    if (operand.front()!='/') {
        if (!context.config_home || context.config_home->empty() || context.config_home->front()!='/' ||
            context.config_home->size()>32768 || context.config_home->find('\0')!=std::string::npos)
            return unknown("AutostartCondition relative file needs caller config directory");
        path=std::filesystem::path(*context.config_home)/path;
    }
    const auto resolved=path.generic_string();
    if (resolved.size()>32768) return unknown("AutostartCondition resolved file exceeds limit");
    if (!context.file_exists) return unknown("AutostartCondition file existence provider absent");
    auto result=context.file_exists(resolved);
    if (result.value) {
        if (kind=="unless-exists") result.value=!*result.value;
        result.note="AutostartCondition file predicate sampled; not a launch guarantee";
    }
    return result;
}
XdgEligibilityContext native_xdg_eligibility_context() {
    XdgEligibilityContext context;
#ifdef __linux__
    context.setting_boolean=read_native_gsettings_boolean;
    struct SessionCache { std::mutex mutex; std::optional<Metric<std::string>> value; };
    auto session=std::make_shared<SessionCache>();
    context.session_name=[session] {
        std::lock_guard lock(session->mutex);
        if(!session->value) session->value=read_native_gnome_session_name();
        return *session->value; // one lazy query per inventory context, including failures
    };
    for (const auto& dir:native_xdg_autostart_directories()) if(dir.personal) {
        context.config_home=dir.path.parent_path().string(); break;
    }
    context.file_exists=[](const std::string& path) -> Metric<bool> {
        struct stat info{};
        if (stat(path.c_str(),&info)==0) return known(true,"condition file exists for effective caller");
        if (errno==ENOENT || errno==ENOTDIR) return known(false,"condition file does not exist for caller");
        if (errno==EACCES || errno==EPERM)
            return {std::nullopt,Support::permission_required,"condition file existence cannot be inspected"};
        return unknown("condition file stat failed; existence unresolved");
    };
    if (auto* value = std::getenv("XDG_CURRENT_DESKTOP")) context.desktop = value;
    std::optional<std::string> search;
    if (auto* value = std::getenv("PATH")) search = value;
    std::error_code ec; const auto cwd = std::filesystem::current_path(ec);
    context.executable = [search, cwd, cwd_known = !ec](const std::string& executable) -> Metric<bool> {
        if (executable.empty() || executable.size() > 32768 || executable.find('\0') != executable.npos)
            return unknown("invalid TryExec path");
        auto check = [](const std::filesystem::path& path) -> Metric<bool> {
            struct stat info{};
            if (stat(path.c_str(), &info) != 0) {
                if (errno == ENOENT || errno == ENOTDIR || errno == EACCES) return known(false, "TryExec not accessible to caller");
                return unknown("TryExec stat failed");
            }
            if (!S_ISREG(info.st_mode)) return known(false, "TryExec is not a regular executable file");
            if (faccessat(AT_FDCWD, path.c_str(), X_OK, AT_EACCESS) == 0) return known(true, "TryExec executable accessible to effective caller");
            if (errno == EACCES || errno == EPERM) return known(false, "TryExec execution permission denied");
            return unknown("TryExec permission check failed");
        };
        if (executable.front() == '/') return check(executable);
        if (executable.find('/') != executable.npos) return unknown("relative TryExec with path components is invalid");
        if (!search || search->size() > 1048576) return unknown("PATH is absent or excessive");
        bool unresolved = false;
        std::string_view paths(*search); unsigned count = 0;
        for (;;) {
            if (++count > 4096) return unknown("PATH search exceeds limit");
            const auto end = paths.find(':'); const auto part = paths.substr(0, end);
            auto directory = std::filesystem::path(part);
            if (!directory.is_absolute()) {
                if (!cwd_known) { unresolved = true; }
                else directory = cwd / directory;
            }
            if (directory.is_absolute()) {
                const auto result = check(directory / executable);
                if (result.value && *result.value) return result;
                if (!result.value) unresolved = true;
            }
            if (end == paths.npos) break;
            paths.remove_prefix(end + 1);
        }
        return unresolved ? unknown("TryExec PATH search incomplete") : known(false, "TryExec not found/executable on caller PATH");
    };
#endif
    return context;
}
}
