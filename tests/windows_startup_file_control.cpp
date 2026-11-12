#include "../src/windows_startup_file_control.hpp"
#include "../src/windows_startup_file_monitor.hpp"
#include "pcinsight/audit.hpp"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <cstddef>
using namespace pcinsight;
namespace {
std::string utf8(const std::wstring& text){
    const auto n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);assert(n>0);
    std::string result(n,'\0');assert(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),result.data(),n,nullptr,nullptr)==n);return result;
}
std::string id(const std::wstring& path,bool common=false){
    std::string result=common?"winfolder:common:":"winfolder:user:";constexpr char hex[]="0123456789ABCDEF";
    for(unsigned char c:utf8(path)){result+=hex[c>>4];result+=hex[c&15];}return result;
}
void put(const std::wstring& path,const char* content){
    const auto h=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);assert(h!=INVALID_HANDLE_VALUE);
    DWORD written{};assert(WriteFile(h,content,static_cast<DWORD>(std::strlen(content)),&written,nullptr) && written==std::strlen(content));assert(CloseHandle(h));
}
std::string get(const std::wstring& path){
    const auto h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(h!=INVALID_HANDLE_VALUE);
    char bytes[100]{};DWORD read{};assert(ReadFile(h,bytes,sizeof(bytes),&read,nullptr));assert(CloseHandle(h));return {bytes,read};
}
bool exists(const std::wstring& path){return GetFileAttributesW(path.c_str())!=INVALID_FILE_ATTRIBUTES;}
int query_mode=0,query_calls=0,restarts=0;std::wstring changed_backup;
BOOL WINAPI directory_query(HANDLE handle,FILE_INFO_BY_HANDLE_CLASS kind,LPVOID buffer,DWORD size){
    ++query_calls;if(kind==FileIdBothDirectoryRestartInfo)++restarts;
    if(query_mode==1 || (query_mode==6 && query_calls>1)){SetLastError(ERROR_ACCESS_DENIED);return FALSE;}
    if(query_mode==5)return TRUE; // Empty successful record must not become a known empty list.
    if(query_mode==7 && kind==FileIdBothDirectoryRestartInfo && restarts==2){assert(DeleteFileW(changed_backup.c_str()));put(changed_backup,"changed during listing");}
    if(query_mode==8){
        const auto stride=(offsetof(FILE_ID_BOTH_DIR_INFO,FileName)+sizeof(wchar_t)+7)&~std::size_t(7);
        for(std::size_t i=0;i<100;++i){FILE_ID_BOTH_DIR_INFO row{};row.FileAttributes=FILE_ATTRIBUTE_DIRECTORY;row.FileNameLength=2;row.FileName[0]=L'.';row.NextEntryOffset=i==99?0:static_cast<DWORD>(stride);assert((i+1)*stride<=size);std::memcpy(static_cast<std::byte*>(buffer)+i*stride,&row,stride);}
        return TRUE;
    }
    const auto result=GetFileInformationByHandleEx(handle,kind,buffer,size);
    if(result){auto* row=static_cast<FILE_ID_BOTH_DIR_INFO*>(buffer);
        if(query_mode==2)row->FileNameLength=1;
        if(query_mode==3)row->NextEntryOffset=size;
        if(query_mode==4)row->FileName[0]=static_cast<wchar_t>(0xd800);
    }
    return result;
}
struct Fixture {
    std::wstring root,live,saved,backup;std::string entry;
    Fixture(){
        wchar_t temp[32768]{};const auto size=GetTempPathW(32768,temp);assert(size && size<32768);
        static unsigned fixture_sequence=0;
        root=std::wstring(temp)+L"PCInsightStartupControl-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(++fixture_sequence);
        assert(CreateDirectoryW(root.c_str(),nullptr));root+=L'\\';
        live=root+L"Private \u00e9 never execute.cmd";backup=root+L"PCInsight.Disabled.v1";saved=backup+L"\\Private \u00e9 never execute.cmd";entry=id(live);
    }
    ~Fixture(){
        // Exact owned files/directories only, never a recursive delete.
        if(exists(live))assert(DeleteFileW(live.c_str()));
        if(exists(saved))assert(DeleteFileW(saved.c_str()));
        if(exists(backup))assert(RemoveDirectoryW(backup.c_str()));
        assert(RemoveDirectoryW(root.c_str()));
    }
};
struct PrivateFileActions final:SystemActions {
    detail::WindowsStartupRoot root;unsigned calls{};detail::WindowsStartupFileScope scope;
    explicit PrivateFileActions(detail::WindowsStartupRoot r,detail::WindowsStartupFileScope s=detail::WindowsStartupFileScope::current_user):root(std::move(r)),scope(s){}
    ActionResult launch_process(const LaunchRequest&)override{return {ActionError::not_supported,""};}
    ActionResult terminate_process(std::uint32_t,bool)override{return {ActionError::not_supported,""};}
    ActionResult set_efficiency_mode(std::uint32_t,bool)override{return {ActionError::not_supported,""};}
    ActionResult control_service(const std::string&,ServiceAction)override{return {ActionError::not_supported,""};}
    ActionResult set_startup_enabled(const std::string&,bool)override{return {ActionError::not_supported,"generic action must not be called"};}
    ActionResult disconnect_user(const std::string&)override{return {ActionError::not_supported,""};}
    ActionResult set_windows_startup_file_registered(const WindowsStartupFileRegistration& expected,bool registered)override{
        ++calls;return detail::set_windows_startup_file_registered(expected,registered,root,scope);
    }
};
}
int main(){
    Fixture f;detail::WindowsStartupRoot root=[&]{return f.root;};
    auto read=[&]{return detail::read_windows_startup_file_registration(f.entry,root);};
    auto change=[&](const auto& observed,bool registered){return detail::set_windows_startup_file_registered(observed,registered,root);};
    auto list=[&]{return detail::list_windows_startup_file_backups(root);};
    auto listing=list();assert(listing.value && listing.value->empty() && listing.support==Support::estimated && !exists(f.backup));
    auto state=read();assert(state.value && state.support==Support::estimated && state.value->state==WindowsStartupFileState::absent && !state.value->file_identity && !exists(f.backup));
    assert(change(*state.value,false).error==ActionError::stale_target && !exists(f.backup));
    put(f.live,"private fixture, not executable code\r\n");
    state=read();assert(state.value && state.value->state==WindowsStartupFileState::registered && state.value->file_identity && !exists(f.backup));
    const auto original=*state.value;
    auto forged=original;forged.file_identity->file_index^=1;assert(change(forged,false).error==ActionError::stale_target && exists(f.live) && !exists(f.backup));
    forged=original;forged.directory_identity.file_index^=1;assert(change(forged,false).error==ActionError::stale_target && !exists(f.backup));
    auto private_backend=std::make_unique<PrivateFileActions>(root);auto* private_calls=private_backend.get();
    bool deny_request=true;std::vector<AuditEvent> events;
    AuditedActions audited(std::move(private_backend),[&](const AuditEvent& e){if(deny_request && e.stage==AuditStage::requested)throw 1;events.push_back(e);});
    assert(!audited.set_windows_startup_file_registered(original,false) && private_calls->calls==0 && exists(f.live) && !exists(f.backup));
    deny_request=false;events.clear();
    auto result=audited.set_windows_startup_file_registered(original,false);
    assert(events.size()==2 && events[0].id==events[1].id && events.back().operation==AuditOperation::windows_startup_file_registration && events.back().result==ActionError::none && events.back().backend_invoked && !events.back().enabled);
    if(!result)std::cerr<<result.message<<'\n';
    assert(result && !exists(f.live) && exists(f.saved) && get(f.saved)=="private fixture, not executable code\r\n");
    assert(change(original,false).error==ActionError::stale_target);
    state=read();assert(state.value && state.value->state==WindowsStartupFileState::retained_by_pcinsight && state.value->file_identity==original.file_identity);
    const auto retained=*state.value;
    listing=list();assert(listing.value && listing.value->size()==1 && listing.value->front().registration.entry_id==f.entry && listing.value->front().registration.state==WindowsStartupFileState::retained_by_pcinsight && listing.value->front().registration.file_identity==original.file_identity && listing.value->front().name=="Private \xc3\xa9 never execute.cmd");
    SystemSnapshot report_snapshot;SampleOptions report_options;report_options.include_windows_startup_file_backups=true;
    detail::fill_windows_startup_file_backups(report_snapshot,report_options,list);
    assert(report_snapshot.windows_startup_file_backups.value && report_snapshot.windows_startup_file_backups.value->size()==1 && report_snapshot.windows_startup_file_backups.value->front().registration.file_identity==original.file_identity);
    report_options.include_windows_startup_file_backups=false;
    detail::fill_windows_startup_file_backups(report_snapshot,report_options,list);
    assert(!report_snapshot.windows_startup_file_backups.value && exists(f.saved));
    // A fresh independent read recovers the backup after the previous observation is discarded.
    assert(detail::read_windows_startup_file_registration(f.entry,root).value->state==WindowsStartupFileState::retained_by_pcinsight);
    put(f.live,"do not overwrite");
    state=read();assert(state.value && state.value->state==WindowsStartupFileState::conflict && !state.value->file_identity);
    listing=list();assert(listing.value && listing.value->size()==1 && listing.value->front().registration.state==WindowsStartupFileState::conflict && !listing.value->front().registration.file_identity);
    assert(change(retained,true).error==ActionError::stale_target && get(f.live)=="do not overwrite" && get(f.saved)=="private fixture, not executable code\r\n");
    assert(DeleteFileW(f.live.c_str()));
    listing=list();assert(listing.value && listing.value->size()==1);
    // Restore using a freshly discovered row, without the earlier retained receipt.
    events.clear();result=audited.set_windows_startup_file_registered(listing.value->front().registration,true);if(!result)std::cerr<<result.message<<'\n';
    assert(events.size()==2 && events.back().enabled && events.back().result==ActionError::none);
    assert(result && exists(f.live) && !exists(f.saved) && get(f.live)=="private fixture, not executable code\r\n");
    assert(list().value->empty());
    // A completion-log failure must not claim failure/rollback of the real rename.
    AuditedActions lost_completion(std::make_unique<PrivateFileActions>(root),[](const AuditEvent& e){if(e.stage==AuditStage::completed)throw 1;});
    state=read();assert(state.value && lost_completion.set_windows_startup_file_registered(*state.value,false) && exists(f.saved) && !exists(f.live));
    assert(lost_completion.diagnostics().failed_completed_records==1);
    state=read();assert(state.value && lost_completion.set_windows_startup_file_registered(*state.value,true) && exists(f.live) && !exists(f.saved));
    assert(lost_completion.diagnostics().failed_completed_records==2);
    state=read();assert(state.value && state.value->state==WindowsStartupFileState::registered && state.value->file_identity==original.file_identity);
    // A retained observation cannot remove a newly modified file.
    const auto before_change=*state.value;assert(DeleteFileW(f.live.c_str()));put(f.live,"changed");
    assert(change(before_change,false).error==ActionError::stale_target && get(f.live)=="changed");
    state=read();assert(state.value);
    const auto locked=CreateFileW(f.live.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);assert(locked!=INVALID_HANDLE_VALUE);
    assert(!change(*state.value,false) && exists(f.live));assert(CloseHandle(locked));
    assert(SetFileAttributesW(f.live.c_str(),FILE_ATTRIBUTE_READONLY));
    const auto readonly_move=change(*state.value,false);assert(readonly_move && !exists(f.live) && exists(f.saved));
    assert(GetFileAttributesW(f.saved.c_str())&FILE_ATTRIBUTE_READONLY);
    state=read();assert(state.value && change(*state.value,true));
    assert(GetFileAttributesW(f.live.c_str())&FILE_ATTRIBUTE_READONLY);
    assert(SetFileAttributesW(f.live.c_str(),FILE_ATTRIBUTE_NORMAL));
    // A backup changed after observation cannot be silently restored.
    state=read();assert(state.value && change(*state.value,false));
    const auto before_saved_change=read();assert(before_saved_change.value);
    assert(DeleteFileW(f.saved.c_str()));put(f.saved,"altered backup");
    assert(change(*before_saved_change.value,true).error==ActionError::stale_target && !exists(f.live) && get(f.saved)=="altered backup");
    state=read();assert(state.value && change(*state.value,true));
    assert(get(f.live)=="altered backup");state=read();assert(state.value);
    assert(change(*state.value,false));
    for(int mode:{1,2,3,4,5,6,8}){
        query_mode=mode;query_calls=restarts=0;
        const auto invalid=detail::list_windows_startup_file_backups(root,directory_query);
        assert(!invalid.value && !invalid.note.empty());if(mode==1 || mode==6)assert(invalid.support==Support::permission_required);
        assert(exists(f.saved) && !exists(f.live));
    }
    query_mode=7;query_calls=restarts=0;changed_backup=f.saved;
    assert(!detail::list_windows_startup_file_backups(root,directory_query).value && get(f.saved)=="changed during listing");
    query_mode=0;assert(!detail::list_windows_startup_file_backups(root,nullptr).value);
    int resolutions=0;const detail::WindowsStartupRoot changed_context=[&]{return ++resolutions==1?f.root:f.root+L"different\\";};
    assert(!detail::list_windows_startup_file_backups(changed_context).value);
    listing=list();assert(listing.value && listing.value->size()==1 && change(listing.value->front().registration,true));state=read();assert(state.value);
    // A hostile backup directory replacement is never followed.
    assert(RemoveDirectoryW(f.backup.c_str()));put(f.backup,"not a directory");
    assert(!read().value && !list().value && !change(*state.value,false));assert(DeleteFileW(f.backup.c_str()));
    // No current file: do not fabricate an arbitrary new registration.
    assert(DeleteFileW(f.live.c_str()));assert(!change(original,true) && !exists(f.live) && !exists(f.backup));
    assert(CreateDirectoryW(f.live.c_str(),nullptr));assert(!read().value && !change(original,false));assert(RemoveDirectoryW(f.live.c_str()));
    for(const auto& name:{L"..\\escape.cmd",L"desktop.ini",L"PCInsight.Disabled.v1",L"thing:stream",L"trailing. ",L"a?b"}){
        assert(!detail::read_windows_startup_file_registration(id(f.root+name),root).value);
    }
    assert(!detail::read_windows_startup_file_registration("winfolder:common:4142",root).value);
    assert(!detail::read_windows_startup_file_registration("winfolder:user:ff",root).value);
    assert(!detail::read_windows_startup_file_registration(id(f.root)+"EDA080",root).value); // UTF-8 encoding of a surrogate is invalid.
    for(const auto& invalid_root:{L"relative",L"C:relative",L"\\\\server\\share\\",L"\\\\?\\GLOBALROOT\\Device\\HarddiskVolume1\\",L"C:\\root:stream\\"}){
        const detail::WindowsStartupRoot invalid=[&]{return invalid_root;};
        assert(!detail::read_windows_startup_file_registration(f.entry,invalid).value);
        assert(!detail::set_windows_startup_file_registered(original,false,invalid));
    }
    const detail::WindowsStartupRoot extended=[&]{return L"\\\\?\\"+f.root;};
    assert(detail::read_windows_startup_file_registration(id(L"\\\\?\\"+f.live),extended).value->state==WindowsStartupFileState::absent);
    assert(detail::read_windows_startup_file_registration(id(f.root+std::wstring(256,L'a')),root).value==std::nullopt);
    assert(!exists(f.backup));
    assert(list().value->empty() && !exists(f.backup));
    assert(CreateDirectoryW(f.backup.c_str(),nullptr));
    put(f.saved,"discovered without saved IDs");
    const auto second=f.backup+L"\\Second.cmd",second_live=f.root+L"Second.cmd",metadata=f.backup+L"\\desktop.ini",nested=f.backup+L"\\subdirectory";
    put(second,"");put(metadata,"ignored metadata");assert(CreateDirectoryW(nested.c_str(),nullptr));
    listing=list();assert(listing.value && listing.value->size()==2);
    for(const auto& item:*listing.value){const auto& row=item.registration;assert(row.state==WindowsStartupFileState::retained_by_pcinsight && row.file_identity);if(row.entry_id==id(second_live))assert(row.file_identity->size_bytes==0 && item.name=="Second.cmd");}
    assert(listing.value->front().registration.entry_id==f.entry); // deterministic exact-name ordering
    assert(change(listing.value->front().registration,true) && get(f.live)=="discovered without saved IDs");
    listing=list();assert(listing.value && listing.value->size()==1 && listing.value->front().registration.entry_id==id(second_live));
    assert(DeleteFileW(second.c_str()));assert(DeleteFileW(metadata.c_str()));assert(RemoveDirectoryW(nested.c_str()));
    assert(list().value->empty());
    {
        // Separate private common/user roots. No all-users Startup path is mutated.
        Fixture common;const auto common_entry=id(common.live,true);
        detail::WindowsStartupRoot common_root=[&]{return common.root;};
        constexpr auto scope=detail::WindowsStartupFileScope::common;
        auto common_list=[&]{return detail::list_windows_startup_file_backups(common_root,&GetFileInformationByHandleEx,scope);};
        auto common_read=[&]{return detail::read_windows_startup_file_registration(common_entry,common_root,scope);};
        assert(common_list().value->empty() && !exists(common.backup));
        put(common.live,"private common-scope contents");
        auto observed=common_read();assert(observed.value && observed.value->state==WindowsStartupFileState::registered);
        auto wrong_scope=*observed.value;wrong_scope.entry_id=id(common.live); // Correct path but wrong scope.
        assert(!detail::set_windows_startup_file_registered(wrong_scope,false,common_root,scope) && exists(common.live) && !exists(common.backup));
        assert(!detail::read_windows_startup_file_registration(common_entry,common_root).value);
        assert(!detail::set_windows_startup_file_registered(*observed.value,false,root,scope)); // Wrong root must not retarget user fixture.
        assert(exists(common.live) && !exists(common.backup));
        AuditedActions common_actions(std::make_unique<PrivateFileActions>(common_root,scope),[&](const AuditEvent& e){assert(e.target==common_entry && e.operation==AuditOperation::windows_startup_file_registration);});
        assert(common_actions.set_windows_startup_file_registered(*observed.value,false) && !exists(common.live) && get(common.saved)=="private common-scope contents");
        auto discovered=common_list();assert(discovered.value && discovered.value->size()==1 && discovered.value->front().registration.entry_id==common_entry);
        assert(list().value->empty()); // Never silently merge common backups into user listing.
        put(common.live,"do not overwrite common");
        auto conflict=common_list();assert(conflict.value && conflict.value->front().registration.state==WindowsStartupFileState::conflict);
        assert(common_actions.set_windows_startup_file_registered(discovered.value->front().registration,true).error==ActionError::stale_target);
        assert(get(common.live)=="do not overwrite common" && get(common.saved)=="private common-scope contents");
        assert(DeleteFileW(common.live.c_str()));
        assert(common_actions.set_windows_startup_file_registered(discovered.value->front().registration,true) && exists(common.live) && !exists(common.saved));
        assert(get(common.live)=="private common-scope contents");
        query_mode=1;query_calls=restarts=0;
        const auto denied=detail::list_windows_startup_file_backups(common_root,directory_query,scope);
        assert(!denied.value && denied.support==Support::permission_required);query_mode=0;
        assert(!detail::list_windows_startup_file_backups(common_root,&GetFileInformationByHandleEx,static_cast<detail::WindowsStartupFileScope>(99)).value);
    }
    const auto native_common=pcinsight::list_windows_common_startup_file_backups(); // Native Known Folder read-only.
    if(native_common.value){assert(native_common.support==Support::estimated);for(const auto& item:*native_common.value)assert(item.registration.entry_id.starts_with("winfolder:common:"));}
    std::cout<<"Native private Startup file control: checked disable/restore, recovery, conflicts, stale identities and cleanup passed; real Startup untouched\n";
}
