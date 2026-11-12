#pragma once
#include "pcinsight/sampler.hpp"
#include <deque>
#include <stdexcept>

namespace pcinsight::detail {
using SamplerClock = std::chrono::steady_clock;
inline constexpr auto sampler_window = std::chrono::seconds(60);

inline SamplerClock::duration sampler_interval(std::chrono::milliseconds interval) {
    if (interval.count() <= 0 ||
        interval > std::chrono::duration_cast<std::chrono::milliseconds>(SamplerClock::duration::max()))
        throw std::invalid_argument("Sampler interval must be positive and fit steady_clock");
    const auto result = std::chrono::duration_cast<SamplerClock::duration>(interval);
    if (result <= SamplerClock::duration::zero())
        throw std::invalid_argument("Sampler interval is below steady_clock resolution");
    return result;
}
inline std::size_t sampler_capacity(std::chrono::milliseconds interval, std::size_t requested) {
    (void)sampler_interval(interval);
    return requested ? requested : static_cast<std::size_t>(60000 / interval.count() + 1);
}
inline SamplerClock::time_point sampler_deadline(SamplerClock::time_point start,
                                                SamplerClock::duration interval) {
    // interval is positive and representable. Avoid overflow near the clock limit.
    return start > SamplerClock::time_point::max() - interval
        ? SamplerClock::time_point::max() : start + interval;
}
inline SamplerClock::time_point sampler_cutoff(SamplerClock::time_point now) {
    return now < SamplerClock::time_point::min() + sampler_window
        ? SamplerClock::time_point::min() : now - sampler_window;
}

// Caller synchronizes access. Retain one stale latest value outside the graph;
// history views never return it after expiration. No second snapshot copy needed.
class SamplerHistory {
    std::size_t capacity_;
    std::deque<SamplerSample> samples_;
public:
    explicit SamplerHistory(std::size_t capacity) : capacity_(capacity) {
        if (!capacity) throw std::invalid_argument("Sampler effective capacity must be positive");
    }
    void clear() { samples_.clear(); }
    void prune(SamplerClock::time_point now) {
        const auto cutoff = sampler_cutoff(now);
        while (samples_.size() > 1 && samples_.front().started_at < cutoff) samples_.pop_front();
    }
    void append(SamplerSample sample) {
        if (sample.finished_at < sample.started_at ||
            (!samples_.empty() && sample.started_at < samples_.back().finished_at))
            throw std::invalid_argument("Sampler collection clock reversed or overlapped");
        const auto now = sample.finished_at;
        samples_.push_back(std::move(sample));
        while (samples_.size() > capacity_) samples_.pop_front();
        prune(now);
    }
    std::optional<SamplerSample> latest_timed() const {
        if (samples_.empty()) return std::nullopt;
        return samples_.back();
    }
    std::optional<SystemSnapshot> latest() const {
        if (samples_.empty()) return std::nullopt;
        return samples_.back().snapshot;
    }
    std::vector<SamplerSample> timed_history(SamplerClock::time_point now) {
        prune(now); std::vector<SamplerSample> result;
        const auto cutoff = sampler_cutoff(now);
        for (const auto& s : samples_)
            if (s.started_at >= cutoff && s.started_at <= now) result.push_back(s);
        return result;
    }
    std::vector<SystemSnapshot> history(SamplerClock::time_point now) {
        prune(now); std::vector<SystemSnapshot> result;
        const auto cutoff = sampler_cutoff(now);
        for (const auto& s : samples_)
            if (s.started_at >= cutoff && s.started_at <= now) result.push_back(s.snapshot);
        return result;
    }
};
} // namespace pcinsight::detail
