#include "../src/menu/Reconcile.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
using V=std::vector<size_t>;
using L=std::vector<nx::ListEntry>;
static nx::ListEntry E(uint64_t id,const std::string &name,const std::string &path,uint32_t version) {
    return nx::ListEntry{id,name,path,version,0};
}
static std::string num(uint64_t id) {return std::to_string(id);}
int main() {
    // T1-T7: classify and usableName for one entry
    assert(nx::classify(E(1,"A","p",1),E(1,"A","p",1))==nx::Change::Unchanged);
    assert(nx::usableName(E(1,"A","p",1)));
    assert(nx::classify(E(1,"A","p",1),E(1,"B","p",1))==nx::Change::Renamed);
    assert(nx::classify(E(1,"A","p",1),E(1,"A","p",2))==nx::Change::Replaced);
    assert(nx::classify(E(1,"A","p",1),E(1,"A","q",1))==nx::Change::Replaced);
    assert(nx::classify(E(1,"A","p",1),E(1,"","p",1))==nx::Change::Unchanged);
    assert(!nx::usableName(E(1,"","p",1)));
    assert(nx::fallbackName(1)=="Software 0000000000000001");
    assert(nx::fallbackName(0xabcdef)=="Software 0000000000ABCDEF");
    assert(nx::classify(E(1,"A","p",1),E(1,"Software 0000000000000001","p",1))==nx::Change::Unchanged);
    assert(!nx::usableName(E(1,"Software 0000000000000001","p",1)));
    assert(nx::classify(E(1,"A","p",1),E(1,"","p",2))==nx::Change::Replaced);
    assert(!nx::usableName(E(1,"","p",2)));

    // R1-R13: reconcile over small lists
    {
        L cur{E(1,"A","p",1),E(2,"B","p",1)};
        nx::Reconciled r=nx::reconcile(cur,cur,true);
        assert(r.removed.empty()&&r.added.empty()&&r.renamed.empty()&&r.replaced.empty()); // R1
    }
    {
        L cur{E(1,"A","p",1)}, fresh{E(1,"A","p",1),E(2,"B","p",1)};
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.added==V{1}&&r.removed.empty()&&r.renamed.empty()&&r.replaced.empty())); // R2
    }
    {
        L cur{E(1,"A","p",1),E(2,"B","p",1)}, fresh{E(1,"A","p",1)};
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.removed==V{1}&&r.added.empty()&&r.renamed.empty()&&r.replaced.empty())); // R3
    }
    {
        L cur{E(1,"A","p",1),E(2,"B","p",1)}, fresh{E(1,"A","p",1)};
        nx::Reconciled r=nx::reconcile(cur,fresh,false);
        assert(r.removed.empty()&&r.added.empty()&&r.renamed.empty()&&r.replaced.empty()); // R4
    }
    {
        L cur{E(1,"A","p",1)}, fresh{E(1,"A2","p",1)};
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.renamed==V{0}&&r.removed.empty()&&r.added.empty()&&r.replaced.empty())); // R5
    }
    {
        L cur{E(1,"A","p",1),E(2,"B","p",1)}, fresh{E(2,"B2","p",1),E(1,"A","p",1)};
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.renamed==V{0}&&r.removed.empty()&&r.added.empty()&&r.replaced.empty())); // R5b
    }
    {
        L cur{E(1,"A","p",1)}, fresh{E(1,"A","p",2)};
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.replaced==V{0}&&r.renamed.empty()&&r.removed.empty()&&r.added.empty())); // R6
    }
    {
        L cur{E(1,"A","p1",1)}, fresh{E(1,"A","p2",1)};
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.replaced==V{0}&&r.renamed.empty()&&r.removed.empty()&&r.added.empty())); // R7
    }
    {
        L cur{E(1,"A","p",1)}, freshEmpty{E(1,"","p",1)}, freshFallback{E(1,"Software 0000000000000001","p",1)};
        nx::Reconciled r=nx::reconcile(cur,freshEmpty,true);
        assert(r.removed.empty()&&r.added.empty()&&r.renamed.empty()&&r.replaced.empty()); // R8
        nx::Reconciled f=nx::reconcile(cur,freshFallback,true);
        assert(f.removed.empty()&&f.added.empty()&&f.renamed.empty()&&f.replaced.empty()); // R8
    }
    {
        L cur{E(1,"A","p",1)}, fresh{E(1,"A","p",1),E(1,"Z","p",1),E(2,"B","p",1)};
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.added==V{2}&&r.renamed.empty()&&r.removed.empty()&&r.replaced.empty())); // R9
    }
    {
        L cur{E(1,"A","p",1)}, fresh{E(1,"A2","p",1),E(1,"Z","p",1)};
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.renamed==V{0}&&r.added.empty()&&r.removed.empty()&&r.replaced.empty())); // R9b
    }
    {
        L cur{E(1,"A","p",1),E(1,"A","p",1),E(2,"B","p",1)}, fresh{E(1,"A","p",1),E(2,"B","p",1)};
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.removed==V{1}&&r.added.empty()&&r.renamed.empty()&&r.replaced.empty())); // R10
        nx::Reconciled f=nx::reconcile(cur,fresh,false);
        assert((f.removed==V{1}&&f.added.empty()&&f.renamed.empty()&&f.replaced.empty())); // R10
    }
    {
        L cur{E(1,"A","p",1),E(2,"B","p",1)}, fresh;
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.removed==V{0,1}&&r.added.empty()&&r.renamed.empty()&&r.replaced.empty())); // R11
        nx::Reconciled f=nx::reconcile(cur,fresh,false);
        assert(f.removed.empty()&&f.added.empty()&&f.renamed.empty()&&f.replaced.empty()); // R11
    }
    {
        L cur, fresh{E(1,"A","p",1),E(2,"B","p",1),E(3,"C","p",1)};
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.added==V{0,1,2}&&r.removed.empty()&&r.renamed.empty()&&r.replaced.empty())); // R12
    }
    {
        L cur, fresh;
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert(r.removed.empty()&&r.added.empty()&&r.renamed.empty()&&r.replaced.empty()); // R13
    }

    // R14: large lists, the result must not depend on fresh order
    {
        L cur, freshA, freshB;
        for(uint64_t id=1;id<=5000;++id) cur.push_back(E(id,"n","p",1));
        freshA.push_back(E(6000,"n","p",1));
        for(uint64_t id=5000;id>0;--id) if(id!=2500) freshA.push_back(E(id,"n","p",1));
        for(uint64_t id=5000;id>0;--id) if(id!=2500) freshB.push_back(E(id,"n","p",1));
        freshB.push_back(E(6000,"n","p",1));
        nx::Reconciled a=nx::reconcile(cur,freshA,true);
        assert((a.removed==V{2499}&&a.added==V{0}&&a.renamed.empty()&&a.replaced.empty())); // R14 A
        nx::Reconciled b=nx::reconcile(cur,freshB,true);
        assert((b.removed==V{2499}&&b.added==V{4999}&&b.renamed.empty()&&b.replaced.empty())); // R14 B
    }

    // R15: fixed mixed pattern over 50 ids, checked against a map/set oracle
    {
        L cur, fresh;
        for(uint64_t id=1;id<=50;++id) cur.push_back(E(id,"N"+num(id),"p",1));
        for(uint64_t id=50;id>0;--id) {
            if(id%7==0) continue;
            if(id%5==0) fresh.push_back(E(id,"N"+num(id),"p",2));
            else if(id%4==0) fresh.push_back(E(id,"N"+num(id),"q",1));
            else if(id%3==0) fresh.push_back(E(id,"R"+num(id),"p",1));
            else fresh.push_back(E(id,"N"+num(id),"p",1));
        }
        for(uint64_t id=51;id<=55;++id) fresh.push_back(E(id,"N"+num(id),"p",1));
        nx::Reconciled r=nx::reconcile(cur,fresh,true);
        assert((r.removed==V{6,13,20,27,34,41,48}));
        assert((r.added==V{43,44,45,46,47}));
        std::set<uint64_t> wantRenamed{3,6,9,18,27,33,39};
        std::set<uint64_t> wantReplaced{4,5,8,10,12,15,16,20,24,25,30,32,36,40,44,45,48,50};
        assert(r.renamed.size()==wantRenamed.size());
        for(size_t i:r.renamed) assert(wantRenamed.count(fresh[i].id)==1);
        assert(r.replaced.size()==wantReplaced.size());
        for(size_t i:r.replaced) assert(wantReplaced.count(fresh[i].id)==1);

        std::map<uint64_t,nx::ListEntry> applied;
        for(const auto &e:cur) applied.emplace(e.id,e);
        std::set<uint64_t> freshIds;
        for(const auto &e:fresh) {
            freshIds.insert(e.id);
            applied.emplace(e.id,e);
        }
        for(auto it=applied.begin();it!=applied.end();) {
            if(freshIds.count(it->first)) ++it;
            else it=applied.erase(it);
        }
        std::set<uint64_t> want;
        for(const auto &kv:applied) want.insert(kv.first);
        std::set<size_t> removedSet(r.removed.begin(),r.removed.end());
        std::set<uint64_t> got;
        for(size_t i=0;i<cur.size();++i) if(!removedSet.count(i)) got.insert(cur[i].id);
        for(size_t i:r.added) got.insert(fresh[i].id);
        assert(got==want);
    }

    // S1-S6: restoreSelection
    auto idOf=[](uint64_t v) {return v;};
    {
        std::vector<uint64_t> items{10,20,30};
        nx::Selection s=nx::restoreSelection(items,30,false,idOf);
        assert(s.index==2&&s.settled); // S1
    }
    {
        std::vector<uint64_t> items{40,30,10,20};
        nx::Selection s=nx::restoreSelection(items,30,false,idOf);
        assert(s.index==1&&s.settled); // S2
    }
    {
        std::vector<uint64_t> items{10,20};
        nx::Selection s=nx::restoreSelection(items,99,false,idOf);
        assert(s.index==0&&!s.settled); // S3
        nx::Selection t=nx::restoreSelection(items,99,true,idOf);
        assert(t.index==0&&t.settled); // S4
        nx::Selection z=nx::restoreSelection(items,0,false,idOf);
        assert(z.index==0&&z.settled); // S5
    }
    {
        std::vector<uint64_t> items;
        nx::Selection s=nx::restoreSelection(items,30,false,idOf);
        assert(s.index==0&&!s.settled); // S6
        nx::Selection t=nx::restoreSelection(items,30,true,idOf);
        assert(t.index==0&&t.settled); // S6
    }
    std::cout << "Return refresh reconciliation passed\n";
}
