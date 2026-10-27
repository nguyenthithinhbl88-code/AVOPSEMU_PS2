#include "ps2_ee.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F call){bool caught=false;try{call();}catch(const std::runtime_error&){caught=true;}ck(caught,"unknown address/unsupported width must remain an error");}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();e->reset();
 for(uint32_t address:{0x1f803204u,0x9f803204u,0xbf803204u})ck(e->read(address,1)==0,"IOP page3 latch reset value and KSEG aliases");
 ck(e->iop_boot_3204_reads==3,"read counter records each byte access");
 e->write(0xbf803204,0x1a5,1);ck(e->read(0x9f803204,1)==0xa5&&e->iop_boot_3204_writes==1,"byte latch shared across aliases");
 const auto reads=e->iop_boot_3204_reads,writes=e->iop_boot_3204_writes;
 fails([&]{e->read(0xbf803204,2);});fails([&]{e->read(0xbf803204,4);});fails([&]{e->read(0xbf803204,8);});
 fails([&]{e->write(0xbf803204,0,2);});fails([&]{e->write(0xbf803204,0,4);});fails([&]{e->write128(0xbf803200,{0,0});});
 fails([&]{e->read(0xbf803203,1);});fails([&]{e->read(0xbf803205,1);});fails([&]{e->write(0xbf803205,0,1);});
 ck(e->iop_boot_3204_reads==reads&&e->iop_boot_3204_writes==writes&&e->iop_boot_3204==0xa5,"invalid widths/neighbors have no latch side effects");
 e->reset(0x9fc42b2c);ck(e->iop_boot_3204==0&&!e->iop_boot_3204_reads&&!e->iop_boot_3204_writes,"EE reset clears bootstrap latch/counters");
 e->r[2].low=0xffffffffbf803204ull;e->r[3]={~0ull,0x1122334455667788ull};
 const uint32_t lbu=0x90430000;for(unsigned n=0;n<4;n++)e->bios[0x42b2c+n]=uint8_t(lbu>>(n*8));
 e->step();ck(e->pc==0x9fc42b30&&!e->r[3].low&&e->r[3].high==0x1122334455667788ull&&e->first_fault.empty(),"reported LBU advances and zero-extends without changing GPR high half");
 struct Entry{uint32_t pc,word;bool delay;};const Entry trace[]={ {0x9fc42b10u,0x00000000u,false},
 {0x9fc42b14u,0x00000000u,false},
 {0x9fc42b18u,0x00000000u,false},
 {0x9fc42b1cu,0x0461fffau,false},
 {0x9fc42b20u,0x24040020u,true},
 {0x9fc42b08u,0x00000000u,false},
 {0x9fc42b0cu,0x2463ffffu,false},
 {0x9fc42b10u,0x00000000u,false},
 {0x9fc42b14u,0x00000000u,false},
 {0x9fc42b18u,0x00000000u,false},
 {0x9fc42b1cu,0x0461fffau,false},
 {0x9fc42b20u,0x24040020u,true},
 {0x9fc42b08u,0x00000000u,false},
 {0x9fc42b0cu,0x2463ffffu,false},
 {0x9fc42b10u,0x00000000u,false},
 {0x9fc42b14u,0x00000000u,false},
 {0x9fc42b18u,0x00000000u,false},
 {0x9fc42b1cu,0x0461fffau,false},
 {0x9fc42b20u,0x24040020u,true},
 {0x9fc42b08u,0x00000000u,false},
 {0x9fc42b0cu,0x2463ffffu,false},
 {0x9fc42b10u,0x00000000u,false},
 {0x9fc42b14u,0x00000000u,false},
 {0x9fc42b18u,0x00000000u,false},
 {0x9fc42b1cu,0x0461fffau,false},
 {0x9fc42b20u,0x24040020u,true},
 {0x9fc42b08u,0x00000000u,false},
 {0x9fc42b0cu,0x2463ffffu,false},
 {0x9fc42b10u,0x00000000u,false},
 {0x9fc42b14u,0x00000000u,false},
 {0x9fc42b18u,0x00000000u,false},
 {0x9fc42b1cu,0x0461fffau,false},
 {0x9fc42b20u,0x24040020u,true},
 {0x9fc42b08u,0x00000000u,false},
 {0x9fc42b0cu,0x2463ffffu,false},
 {0x9fc42b10u,0x00000000u,false},
 {0x9fc42b14u,0x00000000u,false},
 {0x9fc42b18u,0x00000000u,false},
 {0x9fc42b1cu,0x0461fffau,false},
 {0x9fc42b20u,0x24040020u,true},
 {0x9fc42b08u,0x00000000u,false},
 {0x9fc42b0cu,0x2463ffffu,false},
 {0x9fc42b10u,0x00000000u,false},
 {0x9fc42b14u,0x00000000u,false},
 {0x9fc42b18u,0x00000000u,false},
 {0x9fc42b1cu,0x0461fffau,false},
 {0x9fc42b20u,0x24040020u,true},
 {0x9fc42b08u,0x00000000u,false},
 {0x9fc42b0cu,0x2463ffffu,false},
 {0x9fc42b10u,0x00000000u,false},
 {0x9fc42b14u,0x00000000u,false},
 {0x9fc42b18u,0x00000000u,false},
 {0x9fc42b1cu,0x0461fffau,false},
 {0x9fc42b20u,0x24040020u,true},
 {0x9fc42b08u,0x00000000u,false},
 {0x9fc42b0cu,0x2463ffffu,false},
 {0x9fc42b10u,0x00000000u,false},
 {0x9fc42b14u,0x00000000u,false},
 {0x9fc42b18u,0x00000000u,false},
 {0x9fc42b1cu,0x0461fffau,false},
 {0x9fc42b20u,0x24040020u,true},
 {0x9fc42b24u,0x3c02bf80u,false},
 {0x9fc42b28u,0x34423204u,false},
 {0x9fc42b2cu,0x90430000u,false}};
 e->reset(trace[0].pc);std::fill(e->bios.begin(),e->bios.end(),0);
 unsigned branches=0;for(const auto& item:trace){if(item.word==0x0461fffa)branches++;for(unsigned n=0;n<4;n++)e->bios[(item.pc&0x3fffff)+n]=uint8_t(item.word>>(n*8));}
 // Tail begins inside the countdown loop. Derive the entry value from the logged trips.
 e->r[3].low=branches-2;
 for(const auto& item:trace){ck(e->pc==item.pc&&e->delay==item.delay,"tail replay follows logged PCs and branch delay slots");e->step();}
 ck(e->pc==0x9fc42b30&&e->instructions==64&&!e->r[3].low&&e->r[4].low==0x20&&e->iop_boot_3204_reads==1&&e->first_fault.empty(),"64-attempt countdown/read replay reaches the next PC");
 ck(e->diagnostic().find("IOP_BOOT_3204=")!=std::string::npos,"diagnostic exposes narrow model and counters");
 std::cout<<"PASS: IOP bootstrap byte latch aliases/reset/readback, strict widths/neighbors, reported LBU and 64-instruction user tail replay to 9FC42B30\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
