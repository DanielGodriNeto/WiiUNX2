// WiiUNX2 GPL-3.0-or-later. Host-testable disc insert/removal state machine fed by periodic samples.
#pragma once
#include <cstdint>
namespace nx {
struct DiscSample {
    enum Kind { Error, Empty, Game, Other };
    Kind kind=Error;
    uint64_t titleId=0;
    static DiscSample error() { return {Error,0}; }
    static DiscSample empty() { return {Empty,0}; }
    static DiscSample game(uint64_t id) { return {Game,id}; }
    static DiscSample other() { return {Other,0}; }
};
enum class DiscAction { None, Inserted, Changed, Removed };
class DiscMonitor {
public:
    static constexpr unsigned removalSamples=2;
    static constexpr unsigned baseDelayMs=2000;
    static constexpr unsigned maxDelayMs=16000;
    // Insertion is reported on the first valid game sample. Removal needs removalSamples consecutive
    // non-game samples. Errors and a game sample with title ID 0 never change the state.
    DiscAction update(const DiscSample &sample) {
        const bool invalid=sample.kind==DiscSample::Error || (sample.kind==DiscSample::Game && sample.titleId==0);
        if(invalid) { if(errors<1000) ++errors; return DiscAction::None; }
        errors=0;
        if(sample.kind==DiscSample::Game) return seeGame(sample.titleId);
        other=sample.kind==DiscSample::Other;
        if(id==0) return DiscAction::None;
        if(++misses<removalSamples) return DiscAction::None;
        id=0;misses=0;
        return DiscAction::Removed;
    }
    uint64_t current() const { return id; }
    // True while the last valid sample showed a disc that is not a Wii U game (for example a Wii disc).
    bool unsupportedDiscPresent() const { return other; }
    // Poll delay: constant while samples are valid, doubling per consecutive error up to a cap.
    unsigned pollDelayMs() const {
        unsigned delay=baseDelayMs;
        for(unsigned i=0;i<errors && delay<maxDelayMs;++i) delay*=2;
        return delay>maxDelayMs?maxDelayMs:delay;
    }
private:
    DiscAction seeGame(uint64_t gameId) {
        misses=0;other=false;
        if(gameId==id) return DiscAction::None;
        const DiscAction action=id==0?DiscAction::Inserted:DiscAction::Changed;
        id=gameId;
        return action;
    }
    uint64_t id=0;
    unsigned misses=0;
    unsigned errors=0;
    bool other=false;
};
}
