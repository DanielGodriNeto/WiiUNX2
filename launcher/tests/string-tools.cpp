// WiiUNX2 GPL-3.0-or-later. Host tests for StringTools split/format bounds.
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "../src/utils/StringTools.cpp"
int main() {
    using T = StringTools;
    assert((T::stringSplit("a,b", ",") == std::vector<std::string>{"a", "b"}));
    assert((T::stringSplit("abc", "") == std::vector<std::string>{"abc"}));
    assert((T::stringSplit("", "") == std::vector<std::string>{""}));
    std::string big(600, 'x');
    assert(std::strlen(T::fmt("%s", big.c_str())) == 511);
    assert(std::strcmp(T::fmt("%d-%s", 7, "ok"), "7-ok") == 0);
    assert(T::strfmt("%s", big.c_str()).size() == 511);
    std::string out;
    assert(T::strprintf(out, "%s", big.c_str()) == 511 && out.size() == 511);
    assert(std::wcslen(T::wfmt("%s", big.c_str())) == 511);
    assert(std::wcscmp(T::wfmt("n=%d", 5), L"n=5") == 0);
}
