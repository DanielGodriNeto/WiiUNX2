#include "../src/game/Metadata.h"
#include "../src/menu/HomeGeometry.h"
#include "../../dependencies/libgui/include/gui/Utf8.h"
#include <cassert>
#include <vector>
#include <iostream>
int main() {
    char unterminated[4]={'A','B','C','D'};
    assert(nx::bounded(unterminated,4)=="ABCD");
    assert(nx::trim(" \t\r\n").empty());
    assert(nx::xmlTitle("<menu><shortname_en type=\"string\">Mario &amp; Luigi</shortname_en></menu>")=="Mario & Luigi");
    assert(nx::xmlTitle("<shortname_en> </shortname_en><shortname_pt>Jogo</shortname_pt>")=="Jogo");
    assert(nx::xmlTitle("<longname_ja><![CDATA[Test & Name]]></longname_ja>")=="Test & Name");
    assert(nx::xmlTitle("<shortname_en_evil>Bad</shortname_en_evil>").empty());
    assert(nx::xmlTitle("<shortname_en>truncated").empty());
    assert(nx::xmlTitle(std::string("<shortname_en>")+std::string(5000,'x')+"</shortname_en>").empty());
    auto text=gui::decodeUtf8("Pok\xc3\xa9mon / Jo\xc3\xa3o");
    assert(text==L"Pok\u00e9mon / Jo\u00e3o");
    assert(gui::decodeUtf8("\xe3\x81\x82")==L"\u3042");
    assert(gui::decodeUtf8("\xc3")==L"\ufffd");
    assert(gui::decodeUtf8("\xc3Z")==L"\ufffdZ");
    assert(gui::decodeUtf8("\xed\xa0\x80")==L"\ufffd");
    assert(gui::decodeUtf8("\xf4\x90\x80\x80")==L"\ufffd");
    int16_t user[3]={0x41,0x42,0x43};assert(nx::utf16Name(user,3)==L"ABC");
    std::vector<uint8_t> tga(18+128*128*4);
    tga[2]=2;tga[12]=128;tga[14]=128;tga[16]=32;
    assert(nx::validIconTga(tga.data(),tga.size()));
    for(size_t size=0;size<tga.size();++size) assert(!nx::validIconTga(tga.data(),size));
    tga[13]=16;assert(!nx::validIconTga(tga.data(),tga.size()));tga[13]=0;
    std::vector<uint8_t> rle(23);rle[2]=10;rle[12]=2;rle[14]=1;rle[16]=32;rle[18]=0x81;
    assert(nx::validIconTga(rle.data(),rle.size()));
    rle[18]=0x82;assert(!nx::validIconTga(rle.data(),rle.size()));
    rle[18]=0x81;assert(!nx::validIconTga(rle.data(),rle.size()-1));
    assert(nx::hitAccount(230,333,0,12)==0);
    assert(nx::hitAccount(1050,333,0,12)==5);
    assert(nx::hitAccount(230,333,1,12)==6);
    assert(nx::hitAccount(1050,333,1,12)==11);
    assert(nx::hitAccount(1050,333,1,7)==-1);
    assert(nx::hitAccount(312,333,0,12)==-1); // inter-card gap
    assert(nx::hitAccount(230,270,0,12)==-1);
    assert(nx::hitAccount(230,333,0,0)==-1);
    std::cout<<"Metadata fallback, Unicode, truncated/oversized/RLE icons and account paging passed\n";
}
