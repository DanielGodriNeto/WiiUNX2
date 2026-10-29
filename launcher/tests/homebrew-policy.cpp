#include "../src/game/HomebrewPolicy.h"
#include <cassert>
int main() {
    auto defaults=nx::parseHomebrewPolicy("{}");
    assert(!defaults.hideHomebrew&&!defaults.hideAllRPX&&defaults.preferWUHB);
    auto hidden=nx::parseHomebrewPolicy(R"({"storageitems":{"hideHomebrew":true,"hideAllRPX":true,"hideRPXIfWUHBExists":false}})");
    assert(hidden.hideHomebrew&&hidden.hideAllRPX&&!hidden.preferWUHB);
    auto visible=nx::parseHomebrewPolicy(R"({"storageitems":{"hideHomebrew":false}})");
    assert(!visible.hideHomebrew);
    auto types=nx::parseHomebrewPolicy(R"({"storageitems":{"hideHomebrew":"false","hideAllRPX":null}})");
    assert(!types.hideHomebrew&&!types.hideAllRPX);
    assert(!nx::parseHomebrewPolicy("invalid").hideHomebrew);
}
