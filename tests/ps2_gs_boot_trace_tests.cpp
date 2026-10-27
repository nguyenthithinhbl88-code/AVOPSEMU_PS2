#include "ps2_ee.hpp"
#include <memory>
#include <iostream>
#include <stdexcept>
static void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool b=false;try{f();}catch(const std::runtime_error&){b=true;}ck(b,"probe access guard");}
int main(){try{auto e=std::make_unique<ps2::Ee>();
 for(auto alias:{0u,0x80000000u,0xa0000000u}){ck(e->read(0x1f80146eu|alias,1)==0,"reported byte probe and aliases");}
 ck(e->read8(0xbf80146e)==0&&e->iop_probe_146e_reads==4,"public byte path is consistent");
 const auto reads=e->iop_probe_146e_reads;fails([&]{e->read(0xbf80146e,2);});fails([&]{e->read(0xbf80146f,1);});fails([&]{e->write(0xbf80146e,3,1);});ck(e->iop_probe_146e_reads==reads,"unknown widths/writes/neighbors have no probe side effects");
 e->reset(0x8000d6e0);e->r[2].low=0xffffffffbf80146eull;e->r[3].low=0x12001000;e->write(0xd6e0,0x90430000,4);e->step();
 ck(e->pc==0x8000d6e4&&!e->r[3].low&&e->first_fault.empty()&&e->iop_probe_146e_reads==1,"reported LBU advances and zero extends");
 // Exact last 14 attempted instructions in the user's uploaded log.
 const uint32_t code[]={0x3c021000,0x24040001,0x34423000,0x3c031200,0xac440000,0x34631000,0x24020200,0xfc620000,0xf,0x3c018002,0xac202618,0x3c02bf80,0x3442146e,0x90430000};
 e->reset(0x8000d6ac);e->r[29].low=0xffffffff80018e50ull;e->r[31].low=0xffffffff8000d6acull;e->gs.vram[10]=0x12345678;e->write(0x80022618,0xdeadbeef,4);
 for(unsigned i=0;i<14;i++){e->write(0x8000d6ac+4*i,code[i],4);}
 const auto resets=e->gs.soft_resets;for(unsigned i=0;i<14;i++){e->step();}
 ck(e->pc==0x8000d6e4&&e->instructions==14&&!e->r[3].low&&e->read(0x80022618,4)==0,"14-instruction GS-init tail writes RAM and reaches next PC");
 ck(e->gs.soft_resets==resets+1&&e->gs.vram[10]==0x12345678&&e->gs.regs[0]==0&&!e->gs.gif_words,"actual CSR reset does not fabricate display/drawing");
 const auto count=e->iop_probe_146e_reads;e->diagnostic();ck(e->iop_probe_146e_reads==count,"diagnostics preserve probe counters");
 std::cout<<"PASS: exact GS-init 14-instruction log tail through LBU BF80146E to 8000D6E4, probe isolation and no fabricated display\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
