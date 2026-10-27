#include "ps2_gs.hpp"
#include <iostream>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{ps2::Gs g;const auto at=ps2::Gs::address32(0,1,1,1);
g.draw_write(0x1a,1);g.draw_write(0x4c,1ull<<16);g.draw_write(0x4e,1ull<<32);g.draw_write(0x40,8ull<<16|8ull<<48);
auto draw=[&](uint32_t src,uint32_t dst,uint64_t test,uint64_t alpha,unsigned mode=0){g.vram[at]=dst;g.draw_write(0x47,test);g.draw_write(0x42,alpha);g.draw_write(0,mode);g.draw_write(1,src);g.draw_write(5,16ull|(16ull<<16));return g.vram[at];};
for(unsigned method=0;method<8;method++)for(unsigned value:{127u,128u,129u}){
 bool pass=method==1||(method==2&&value<128)||(method==3&&value<=128)||(method==4&&value==128)||(method==5&&value>=128)||(method==6&&value>128)||(method==7&&value!=128);
 const uint32_t src=(value<<24)|0x010203,dst=0xaabbccdd;
 for(unsigned fail=0;fail<4;fail++){
  auto actual=draw(src,dst,1|(method<<1)|(128<<4)|(fail<<12),0);
  const uint32_t expected=pass||fail==1?src:fail==3?(dst&0xff000000)|(src&0xffffff):dst;
  ck(actual==expected,"all alpha test modes, boundary values and failure actions with Z masked");
 }
}
ck(draw(0x400000c8,0x80142864,0,0x44,0x40)==0x400a1496,"source-over integer blending at alpha 64/128");
g.draw_write(0x46,1);ck((draw(0x800000c8,0x80000064,0,(128ull<<32)|0x68,0x40)&255)==255,"COLCLAMP upper saturation");
g.draw_write(0x46,0);ck((draw(0x800000c8,0x80000064,0,(128ull<<32)|0x68,0x40)&255)==44,"COLCLAMP wrap");
g.draw_write(0x46,1);ck((draw(0x800000c8,0,0,(128ull<<32)|0xa2,0x40)&255)==0,"negative blend clamp");
g.draw_write(0x46,0);ck((draw(0x80000001,0,0,(1ull<<32)|0xa2,0x40)&255)==255,"negative products round down before wrap");
g.draw_write(0x49,1);ck(draw(0x400000c8,0x80142864,0,0x44,0x40)==0x400000c8,"PABE bypass for source alpha bit7 clear");
g.draw_write(0x49,0);ck(draw(0x80112233,0x00123456,0x4000,0)==0x80112233&&draw(0x80112233,0x80123456,0x4000,0)==0x80123456,"DATE destination alpha method zero");
ck(draw(0x80112233,0x80123456,0xc000,0)==0x80112233&&draw(0x80112233,0x00123456,0xc000,0)==0x00123456,"DATE method one");
g.draw_write(0x4c,(1ull<<16)|(1ull<<24));ck(draw(0x80112233,0xaa123456,0xc000,0)==0xaa112233,"PSMCT24 ignores DATE and preserves alpha");g.draw_write(0x4c,1ull<<16);
g.draw_write(0x4a,1);ck(draw(0x40112233,0xaa123456,1|(128<<4),0)==0xaa123456,"alpha test occurs before FBA forcing");ck(draw(0x40112233,0,0,0)==0xc0112233,"FBA after tests");g.draw_write(0x4a,0);
ck(draw(0x80112233,0x12345678,0x10000,0)==0x12345678&&draw(0x80112233,0,0x30000,0)==0x80112233,"Z NEVER/ALWAYS with writes masked");
const auto skipped=g.skipped_primitives;draw(0x80112233,0,0x50000,0);ck(g.skipped_primitives==skipped+1,"actual depth comparisons remain explicit unsupported");
g.draw_write(0x4c,(1ull<<16)|(0x00ff0000ull<<32));ck(draw(0x80112233,0xaa123456,0,0)==0x80122233,"FBMSK composes with new pixel pipeline");
std::cout<<"PASS: 96 alpha boundary/fail cases, ALPHA/COLCLAMP/PABE, DATE/FBA, masked depth modes and framebuffer masks\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
