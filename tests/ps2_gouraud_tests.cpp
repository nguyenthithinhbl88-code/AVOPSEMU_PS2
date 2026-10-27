#include "ps2_gs.hpp"
#include <iostream>
#include <algorithm>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{
 auto gp=std::make_unique<ps2::Gs>();auto& g=*gp;
 g.draw_write(0x1a,1);g.draw_write(0x4c,1ull<<16);g.draw_write(0x4e,1ull<<32);g.draw_write(0x40,(15ull<<16)|(15ull<<48));
 auto xy=[](unsigned x,unsigned y){return uint64_t(x*16)|(uint64_t(y*16)<<16);};
 auto read=[&](unsigned x,unsigned y){return g.vram[ps2::Gs::address32(0,1,x,y)];};
 auto triangle=[&](bool reverse){
  g.draw_write(0,3|8);g.draw_write(1,0x800000ff);g.draw_write(5,xy(0,0));
  g.draw_write(1,reverse?0x80ff0000:0x8000ff00);g.draw_write(5,reverse?xy(0,8):xy(8,0));
  g.draw_write(1,reverse?0x8000ff00:0x80ff0000);g.draw_write(5,reverse?xy(8,0):xy(0,8));
 };
 triangle(false);ck(read(0,0)==0x800f0fdf&&g.primitives==1&&!g.skipped_primitives,"RGBA Gouraud triangle uses pixel-center weights");
 const auto baseline=g.vram;std::fill(g.vram.begin(),g.vram.end(),0);triangle(true);ck(g.vram==baseline,"Gouraud winding/color consistency");
 g.draw_write(0,1|8);g.draw_write(1,0x80000000);g.draw_write(5,xy(0,10));g.draw_write(1,0x80ffffff);g.draw_write(5,xy(4,10));ck(read(0,10)==0x80000000&&read(2,10)==0x807f7f7f&&read(4,10)==0x80ffffff,"Gouraud line endpoints and midpoint");
 g.draw_write(0x4a,1);g.draw_write(0x4c,(uint64_t(0xff)<<32)|(1ull<<16));g.vram[ps2::Gs::address32(0,1,2,10)]=0x11;
 g.draw_write(0,1|8);g.draw_write(1,0);g.draw_write(5,xy(0,10));g.draw_write(1,0x00ffffff);g.draw_write(5,xy(4,10));ck(read(2,10)==0x807f7f11,"interpolated pixels preserve FBMSK and FBA");
 std::cout<<"PASS: GS Gouraud triangle/line, winding, alpha interpolation, mask and FBA\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
