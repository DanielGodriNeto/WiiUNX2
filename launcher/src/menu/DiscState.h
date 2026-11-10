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
enum class DiscAction { None, Inserted, Changed, Removed, Unsupported };
class DiscMonitor {
public:
    static constexpr unsigned removalSamples=2;
    static constexpr unsigned baseDelayMs=2000;
    static constexpr unsigned maxDelayMs=16000;
    // Insertion is reported on the first valid game sample. Removal needs removalSamples consecutive
    // non-game samples. Errors and a game sample with title ID 0 never change the state. An Other sample
    // with no game held reports Unsupported once until an Empty or Game sample arrives.
    DiscAction update(const DiscSample &sample) {
        const bool invalid=sample.kind==DiscSample::Error || (sample.kind==DiscSample::Game && sample.titleId==0);
        if(invalid) { if(errors<1000) ++errors; return DiscAction::None; }
        errors=0;
        if(sample.kind==DiscSample::Game) { unsupportedSeen=false; return seeGame(sample.titleId); }
        if(sample.kind==DiscSample::Empty) unsupportedSeen=false;
        return seeNonGame(sample.kind==DiscSample::Other);
    }
    uint64_t current() const { return id; }
    // Poll delay: constant while samples are valid, doubling per consecutive error up to a cap.
    unsigned pollDelayMs() const {
        unsigned delay=baseDelayMs;
        for(unsigned i=0;i<errors && delay<maxDelayMs;++i) delay*=2;
        return delay>maxDelayMs?maxDelayMs:delay;
    }
private:
    DiscAction seeGame(uint64_t gameId) {
        misses=0;
        if(gameId==id) return DiscAction::None;
        const DiscAction action=id==0?DiscAction::Inserted:DiscAction::Changed;
        id=gameId;
        return action;
    }
    // Empty and Other samples count toward removal of a held game. With no game held, an Other sample
    // reports Unsupported once per unsupported period; an Empty sample ends the period.
    DiscAction seeNonGame(bool other) {
        if(id!=0) {
            if(++misses<removalSamples) return DiscAction::None;
            id=0;misses=0;
            return DiscAction::Removed;
        }
        if(!other || unsupportedSeen) return DiscAction::None;
        unsupportedSeen=true;
        return DiscAction::Unsupported;
    }
    uint64_t id=0;
    unsigned misses=0;
    unsigned errors=0;
    bool unsupportedSeen=false;
};
}
