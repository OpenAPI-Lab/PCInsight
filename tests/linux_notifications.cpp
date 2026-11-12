#include "../src/linux_notification_ledger.hpp"
#include <cassert>
#include <iostream>
#include <limits>
using namespace pcinsight;
using namespace pcinsight::detail;
int main() {
    using namespace std::chrono;
    LinuxNotificationObserver observer;
    assert(!observer.report().value);
    assert(observer.start().error == ActionError::invalid_argument);
    LinuxNotificationOptions options; options.consent_to_monitor_notification_traffic = true;
    options.bus_socket_path = "/tmp/unsafe;unixexec:path=/no";
    assert(observer.start(options).error == ActionError::invalid_argument);
    assert(!observer.poll(0).value && !observer.poll(4097).value);
    observer.stop();
    auto moved = std::move(observer);
    assert(!observer.report().value && observer.start(options).error == ActionError::invalid_argument);
    assert(!observer.poll().value); observer.stop();
    assert(valid_notification_socket("/tmp/private/bus"));
    for (const auto* path : {"", "relative", "/tmp/a,b", "/tmp/a%3b", "/tmp/a b", "/tmp/a\nb"}) assert(!valid_notification_socket(path));
    assert(valid_notification_unique_name(":1.22") && !valid_notification_unique_name(":1.2.3"));
    assert(valid_notification_desktop_hint("org.test.App") && !valid_notification_desktop_hint("../app"));
    LinuxNotificationLedger ledger;
    assert(!ledger.report().value);
    const auto now = LinuxNotificationLedger::Clock::now(); const auto wall = system_clock::now();
    ledger.begin(":1.1", now, wall);
    ledger.unconfirmed(); assert(ledger.report().value->unconfirmed_no_reply_calls == 1 && ledger.report().value->apps.empty());
    assert(ledger.report().value && ledger.report().value->apps.empty());
    ledger.call(":1.2", 42, "org.test.App", 0, now);
    assert(ledger.report().value->pending_calls == 1);
    ledger.reply(":1.9", ":1.2", 42, true, 17); // wrong source cannot claim a Notify acceptance
    ledger.reply(":1.1", ":1.3", 42, true, 17); // wrong recipient, same serial
    assert(ledger.report().value->apps.empty());
    ledger.reply(":1.1", ":1.2", 42, true, 17);
    ledger.reply(":1.1", ":1.2", 42, true, 17); // duplicate reply is not another notification
    auto report = ledger.report(); assert(report.value->apps.size() == 1 && report.value->apps[0].accepted_notify_calls == 1);
    assert(report.support == Support::estimated && report.value->apps[0].declared_desktop_entry.support == Support::estimated);
    ledger.call(":1.2", 43, "org.test.App", 17, now);
    ledger.reply(":1.1", ":1.2", 43, true, 17);
    ledger.call(":1.3", 1, std::nullopt, 0, now);
    ledger.reply(":1.1", ":1.3", 1, true, 18);
    ledger.call(":1.4", 1, "org.test.App", 0, now);
    ledger.reply(":1.1", ":1.4", 1, false, 0);
    report = ledger.report(); assert(report.value->rejected_calls == 1 && report.value->apps.size() == 2);
    assert(report.value->apps[0].accepted_notify_calls == 2 && report.value->apps[0].replacement_calls == 1);
    assert(!report.value->apps[1].declared_desktop_entry.value); // no app-name/username guess
    ledger.call(":1.5", 1, "org.test.Pending", 0, now);
    ledger.tick(now + seconds(30), wall + seconds(30));
    assert(ledger.report().value->expired_unresolved_calls == 1 && ledger.report().value->pending_calls == 0);
    ledger.reply(":1.1", ":1.5", 1, true, 19); assert(ledger.report().value->apps.size() == 2);
    ledger.running = false;
    ledger.call(":1.7", 1, "org.test.Stopped", 0, now + seconds(31));
    assert(ledger.report().value->apps.size() == 2 && !ledger.report().value->running);
    ledger.begin(":1.8", now, wall); assert(ledger.report().value->apps.empty());
    ledger.call(":1.2", 1, "org.test.Bad", 17, now); ledger.reply(":1.8", ":1.2", 1, true, 18);
    assert(!ledger.report().value); // invalid replacement identity invalidates period
    ledger.begin(":1.1", now, wall);
    ledger.call(":1.2", 1, "org.test.App", 0, now); ledger.reply(":1.1", ":1.2", 1, true, 1);
    ledger.apps.begin()->second.accepted_notify_calls = UINT64_MAX;
    ledger.call(":1.2", 2, "org.test.App", 0, now); ledger.reply(":1.1", ":1.2", 2, true, 2);
    assert(!ledger.report().value); // overflow is absence, not wrapped zero
    ledger.begin(":1.1", now, wall);
    ledger.call(":1.2", 1, "org.test.App", 0, now); ledger.call(":1.2", 1, "org.test.App", 0, now);
    assert(!ledger.report().value);
    ledger.begin(":1.1", now, wall);
    for (unsigned i = 1; i <= 1024; ++i) ledger.call(":1.2", i, "org.test.App", 0, now);
    assert(ledger.report().value); ledger.call(":1.2", 1025, "org.test.App", 0, now); assert(!ledger.report().value);
    ledger.begin(":1.1", now, wall); ledger.tick(now - seconds(1), wall); assert(!ledger.report().value);
    ledger.begin(":1.1", now, wall); ledger.call(":1.2", 0, "org.test.App", 0, now); assert(!ledger.report().value);
    ledger.begin(":1.1", now, wall); ledger.call(":1.2", 1, "../bad-hint", 0, now); assert(!ledger.report().value);
    ledger.begin(":1.1", now, wall);
    for (unsigned i = 0; i < 2048; ++i) ledger.apps[{":1.9", "org.test." + std::to_string(i)}] = {};
    ledger.call(":1.2", 1, "org.test.Overflow", 0, now); ledger.reply(":1.1", ":1.2", 1, true, 1); assert(!ledger.report().value);
    ledger.begin(":1.1", now, wall); ledger.no_reply = UINT64_MAX; ledger.unconfirmed(); assert(!ledger.report().value);
    ledger.begin("not-a-unique-name", now, wall); assert(!ledger.report().value);
#ifndef __linux__
    options.bus_socket_path.clear();
    assert(moved.start(options).error == ActionError::not_supported);
    options.bus_socket_path = "/tmp/private/bus";
    assert(moved.start(options).error == ActionError::not_supported && !moved.report().value);
#endif
    std::cout << "PASS notification consent, identity/reply correlation, unknown attribution, periods, expiry, duplicates/overflow/budgets and safe stop\n";
}
