// Public SDK example. No launch, autostart mutation, or capture activation.
#include <pcinsight/monitor.hpp>
#include <pcinsight/startup.hpp>
#include "detail/metric_text.hpp"
#include <iostream>
#include <stdexcept>
#ifdef __linux__
#include <unistd.h>
#endif
using namespace pcinsight;
namespace {
using pcinsight_example::field;
void label(std::ostream& out,const std::string& key,const std::string& value){
    Metric<std::string> m{{},Support::unsupported,"No reported label"};
    if(!value.empty())m={value,Support::available,"Reported metadata, not identity proof"};
    field(out,key,"label",m);
}
const char* impact_name(StartupImpact impact){
    switch(impact){case StartupImpact::none:return "none";case StartupImpact::low:return "low";
        case StartupImpact::medium:return "medium";case StartupImpact::high:return "high";
        case StartupImpact::not_measured:return "not measured";}
    return "unknown";
}
void render(std::ostream& out,const StartupEntry& row,const std::string& prefix){
    label(out,prefix+".id",row.id);label(out,prefix+".name",row.name);label(out,prefix+".source",row.source);
    Metric<std::string> config{{},Support::unsupported,"Configuration unknown"};
    if(row.status==StartupStatus::enabled||row.status==StartupStatus::disabled)
        config={row.status==StartupStatus::enabled?"enabled":"disabled",Support::available,"Configuration only, not proof of launch"};
    field(out,prefix+".configured_status","state",config);
    field(out,prefix+".publisher","declared-label",row.publisher_label);
    field(out,prefix+".xdg_standard_eligible","bool",row.xdg_standard_eligible);
    field(out,prefix+".autostart_condition","bool",row.autostart_condition_satisfied);
    const auto& measured=row.observed_startup_impact;
    Metric<std::string> impact{{},measured.support,measured.note};
    if(metric_is_known(measured))impact.value=impact_name(*measured.value);
    field(out,prefix+".observed_impact","category",impact);
    field(out,prefix+".observed_cpu_time","ns",row.observed_startup_cpu_time);
    field(out,prefix+".observed_disk","B",row.observed_startup_disk_bytes);
    const auto& elapsed=row.observed_startup_interval;
    Metric<double> interval{{},elapsed.support,elapsed.note};
    if(metric_is_known(elapsed))interval.value=std::chrono::duration<double>(*elapsed.value).count();
    field(out,prefix+".observed_interval","s",interval);
    label(out,prefix+".impact_scope",row.impact_scope);
    // Neither legacy impact/publisher nor raw Exec/command is used as fallback.
}
void properties(std::ostream& out,const StartupProperties& p){
    render(out,p.entry,"properties");
    field(out,"properties.source_size","B",p.source_file_size_bytes);
    for(std::size_t i=0;i<p.fields.size();++i){const auto key="properties.field["+std::to_string(i)+"]";
        label(out,key+".group",p.fields[i].group);label(out,key+".key",p.fields[i].key);
    } // Preserve order/duplicates; intentionally don't print raw values.
}
void check(bool ok){if(!ok)throw std::runtime_error("Startup example contract failed");}
int self_test(){
    StartupEntry r;r.id="/fixture/example.desktop";r.name="Example";r.source="fixture";
    r.publisher="LEGACY_MUST_NOT_LEAK";r.command="RAW_COMMAND_MUST_NOT_LEAK";r.impact=StartupImpact::high;
    std::ostringstream out;render(out,r,"unknown");
    r.status=StartupStatus::enabled;r.xdg_standard_eligible={false,Support::available,{}};
    r.publisher_label={"Declared",Support::estimated,{}};r.observed_startup_impact={StartupImpact::none,Support::estimated,{}};
    r.observed_startup_cpu_time={std::chrono::nanoseconds(0),Support::estimated,{}};render(out,r,"observed");
    StartupProperties p;p.entry=r;p.file_size_bytes=999;
    p.fields={{"Desktop Entry","Exec","RAW_VALUE_MUST_NOT_LEAK"},{"Desktop Entry","Exec","second"}};
    properties(out,p);const auto lines=out.str();
    for(const auto* s:{"unknown.configured_status\tstate\tunsupported\tunknown","unknown.observed_impact\tcategory\tunsupported\tunknown",
        "observed.configured_status\tstate\tavailable\t\"enabled\"","observed.xdg_standard_eligible\tbool\tavailable\tfalse",
        "observed.observed_impact\tcategory\testimated\t\"none\"","observed.observed_cpu_time\tns\testimated\t0",
        "properties.source_size\tB\tunsupported\tunknown","properties.field[1].key\tlabel\tavailable\t\"Exec\""})check(lines.find(s)!=lines.npos);
    check(lines.find("MUST_NOT_LEAK")==lines.npos);return 0;
}
}
int main(int argc,char** argv){
    if(argc==2&&std::string(argv[1])=="--self-test")return self_test();
#ifdef __linux__
    if(argc==3&&std::string(argv[1])=="--properties"){
        alarm(20);auto result=get_startup_properties(argv[2]);
        if(!result){std::cerr<<pcinsight_example::escaped(result.message)<<'\n';return 1;}
        properties(std::cout,*result.value);return 0;
    }
    if(argc==2&&std::string(argv[1])=="--sample"){
        alarm(20);Monitor monitor;SampleOptions options;
        options.include_startup_entries=true;options.include_processes=false;
        options.include_disks=options.include_networks=options.include_gpus=false;
        const auto s=monitor.sample(options);
        for(std::size_t i=0;i<s.startup_entries.size();++i)render(std::cout,s.startup_entries[i],"startup["+std::to_string(i)+"]");
        std::cout<<"Rendered rows: "<<s.startup_entries.size()<<" (not a completeness guarantee)\n";return 0;
    }
#endif
    std::cout<<"--sample: Linux inventory; --properties ID: Linux read-only metadata; --self-test: fixtures. No actions or capture.\n";return argc==1?0:2;
}
