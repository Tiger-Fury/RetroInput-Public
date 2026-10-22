#include "../RetroInput/Games/TheSufferingTTB/RenderingSupport.h"
#include <cassert>
#include <array>
using namespace RetroInput::TheSufferingTTB::RenderingSupport;
int main() {
    wchar_t beside[MAX_PATH] = {};
    assert(BesideModule(L"RetroInputTTBImages.log", beside));
    assert(wcsstr(beside,L"RetroInputTTBImages.log"));
    const std::uint32_t pixels[]={1,2,3,4};
    std::array<std::uint32_t,6> destination={9,9,9,9,9,9};
    RetroInput::Buttons::AtlasImage source{pixels,2,2,8,RetroInput::Buttons::PixelFormat::StraightAlphaARGB32};
    assert(CopyPixels(source,{12,destination.data()}));
    assert((destination==std::array<std::uint32_t,6>{3,4,9,1,2,9}));
    destination.fill(9);
    assert(!CopyPixels(source,{4,destination.data()}));
    assert(!CopyPixels(source,{12,nullptr}));
    assert(!CopyPixels(source,{-8,destination.data()}));
    source.strideBytes=4; assert(!CopyPixels(source,{12,destination.data()}));
    source.strideBytes=8;source.width=4097;assert(!CopyPixels(source,{12,destination.data()}));
    source.width=2;source.height=0;assert(!CopyPixels(source,{12,destination.data()}));
    source.height=2;source.pixels=nullptr;assert(!CopyPixels(source,{12,destination.data()}));
    assert((destination==std::array<std::uint32_t,6>{9,9,9,9,9,9}));
}

