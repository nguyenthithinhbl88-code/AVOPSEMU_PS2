#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"unsupported MCH access must fail");}
int main(){try{auto e=std::make_unique<ps2::Ee>();e->reset();
for(unsigned i=0;i<16;i++){
 uint32_t a=0x1000f400+i*16;ck(e->read(a,4)==0,"MCH initial slot");e->write(a|0xa0000000,0x12340000+i,4);
 if(i!=1&&i!=3&&i!=4)ck(e->read(a|0x80000000,4)==0x12340000+i,"MCH ordinary backing slot/alias");
 fails([&]{e->read(a+4,4);});fails([&]{e->write(a+4,0,4);});fails([&]{e->write(a,0,8);});fails([&]{e->write128(a,{0,0});});
}
ck(e->read(0xb000f410,4)==0&&e->mch_registers[1]==0x12340001,"F410 writes retained; reads use explicit busy policy");
e->write(0xb000f440,0,4);e->rdram_device=2;e->write(0xb000f430,0x80210040,4);ck(!e->rdram_device&&e->mch_ricm==0x210040&&e->read(0xb000f430,4)==0,"RICM INIT clears enumeration/busy");
e->write(0xb000f430,0x210000,4);ck(e->read(0xb000f440,4)==0x1f&&e->read(0xb000f440,4)==0x1f&&e->read(0xb000f440,4)==0,"two RDRAM response devices then end");
e->write(0xb000f430,0x230000,4);ck(e->read(0xb000f440,4)==0xd0d,"RDRAM CNFGA");e->write(0xb000f430,0x240000,4);ck(e->read(0xb000f440,4)==0x90,"RDRAM CNFGB");e->write(0xb000f430,0x400007,4);ck(e->read(0xb000f440,4)==7,"RDRAM DEVID");
e->write(0xb000f440,0x80,4);e->rdram_device=2;e->write(0xb000f430,0x210040,4);ck(e->rdram_device==2,"SRP suppresses enumeration reset");
const auto reads=e->mch_reads;const auto device=e->rdram_device;ck(e->diagnostic().find("MCH[1000f410]")!=std::string::npos&&e->mch_reads==reads&&e->rdram_device==device,"diagnostic does not enumerate devices/read registers");
e->reset(0x9fc4135c);ck(!e->mch_reads&&!e->mch_writes&&!e->mch_registers[1],"MCH reset");
e->r[3].low=0xffffffffb000f410ull;uint32_t sw=0xac600000;for(unsigned n=0;n<4;n++)e->bios[0x4135c+n]=uint8_t(sw>>(8*n));e->step();ck(e->pc==0x9fc41360&&e->mch_writes==1&&e->first_fault.empty(),"reported SW zero crosses F410 stop");
struct Entry{uint32_t pc,word;bool delay;};const Entry trace[]={
{0x9fc41330u,0x3c03b000u,false},
{0x9fc41334u,0x8fa8000cu,false},
{0x9fc41338u,0x3463f410u,false},
{0x9fc4133cu,0x200b82du,false},
{0x9fc41340u,0x1288c0u,false},
{0x9fc41344u,0x85400bu,false},
{0x9fc41348u,0x26103da0u,false},
{0x9fc4134cu,0x8fa40010u,false},
{0x9fc41350u,0xafa8000cu,false},
{0x9fc41354u,0x47200bu,false},
{0x9fc41358u,0xafa40010u,false},
{0x9fc4135cu,0xac600000u,false},
};
e->reset(trace[0].pc);e->r[29].low=0x70003e60;e->r[16].low=0xffffffff9fc40000ull;e->r[18].low=1;e->r[2].low=0x20;e->r[4].low=0x400;e->r[5].low=0;e->r[7].low=1;e->write(0x70003e6c,0x200,4);e->write(0x70003e70,0x10,4);
// Entry arguments/stack values reconstructed from logged producers and final snapshot.
for(const auto& x:trace)for(unsigned n=0;n<4;n++)e->bios[(x.pc&0x3fffff)+n]=uint8_t(x.word>>(8*n));
for(const auto& x:trace){ck(e->pc==x.pc&&e->delay==x.delay,"log10 tail PC/delay");e->step();}
ck(e->pc==0x9fc41360&&e->mch_writes==1&&e->r[4].low==0x20&&e->r[8].low==0x200&&e->r[16].low==0xffffffff9fc43da0ull&&e->first_fault.empty(),"reconstructed log10 tail reaches next PC");
std::cout<<"PASS: 16 MCH backing slots/guards, RDRAM protocol, snapshot/reset and log10 replay to 9FC41360\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
