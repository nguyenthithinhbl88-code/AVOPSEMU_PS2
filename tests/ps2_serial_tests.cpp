#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"unsupported access must fail");}
int main(){try{auto e=std::make_unique<ps2::Ee>();e->reset();
for(uint32_t a:{0x1000f100u,0x1000f120u,0x1000f140u,0x1000f150u}){
 ck(e->read(a,4)==0,"serial reset");e->write(a|0xa0000000,0xaabbccdd,4);e->write((a|0x80000000)+1,0x12,1);e->write(a+2,0x3456,2);ck(e->read(a,4)==0x345612dd,"serial config lanes and aliases");}
e->reset();e->write(0xb000f100,0x20,4);e->write(0xb000f120,0,4);e->write(0xb000f140,7,4);e->write(0xb000f140,0,4);e->write(0xb000f150,10,4);
ck(e->read(0x9000f100,4)==0x20&&e->serial.reset_requests==1&&e->read(0x9000f150,4)==10,"SDK style serial init");
ck(e->read(0xb000f130,4)==0&&e->read(0xb000f110,4)==0&&e->read(0xb000f1c0,1)==0,"TX never full, RX empty and no errors");
e->write(0xb000f110,~0u,4);e->write(0xb000f130,7,4);ck(e->read(0xb000f110,4)==0&&e->read(0xb000f130,4)==0,"status writes do not fabricate input/errors");
e->write(0xb000f180,'A',1);e->write(0x9000f180,0x4342,2);e->write(0x1000f180,0x0a464544,4);e->write(0xb000f180,0,1);ck(e->console=="ABCDEF\n."&&e->serial.tx_bytes==8,"shared console sink, little endian multi-byte policy and filtering");
e->write(0xb000f140,7,4);ck(e->console=="ABCDEF\n.","FIFO reset keeps historical console output");
const auto reads=e->serial.reads,writes=e->serial.writes,bytes=e->serial.tx_bytes;
for(uint32_t a:{0x1000f104u,0x1000f160u,0x1000f181u,0x1000f1c4u}){fails([&]{e->read(a,1);});fails([&]{e->write(a,0,1);});}
fails([&]{e->write(0xb000f100,0,8);});fails([&]{e->write128(0xb000f100,{0,0});});fails([&]{e->read(0xb000f180,1);});fails([&]{e->read(0xb000f1c0,4);});fails([&]{e->write(0xb000f101,0,2);});
ck(e->serial.reads==reads&&e->serial.writes==writes&&e->serial.tx_bytes==bytes,"invalid accesses do not alter serial state");
e->console.clear();for(unsigned i=0;i<17000;i++)e->write(0xb000f180,'X',1);e->write(0xb000f180,'Y',1);ck(e->console.size()==16384&&e->console.back()=='Y',"console capture bounded to 16 KiB");
const auto before=e->serial.reads;ck(e->diagnostic().find("EE SIO: LCR=20")!=std::string::npos&&e->serial.reads==before,"serial diagnostics have no MMIO side effects");
e->reset(0x9fc43374);ck(e->console.empty()&&!e->serial.tx_bytes&&!e->serial.lcr,"reset clears serial and console");
e->r[10].low=0xffffffffb000f100ull;e->r[3].low=0x20;uint32_t sw=0xad430000;for(unsigned n=0;n<4;n++)e->bios[0x43374+n]=uint8_t(sw>>(n*8));e->step();ck(e->pc==0x9fc43378&&e->serial.lcr==0x20&&e->first_fault.empty(),"reported SW advances");
struct Entry{uint32_t pc,word;bool delay;};const Entry trace[]={
{0x9fc43320u,0x30c20002u,false},
{0x9fc43324u,0x30e70001u,false},
{0x9fc43328u,0x21880u,false},
{0x9fc4332cu,0x3c0ab000u,false},
{0x9fc43330u,0x30c60003u,false},
{0x9fc43334u,0x73880u,false},
{0x9fc43338u,0x80582du,false},
{0x9fc4333cu,0x63100u,false},
{0x9fc43340u,0x31020001u,false},
{0x9fc43344u,0x11200003u,false},
{0x9fc43348u,0x354af100u,true},
{0x9fc43354u,0x34630020u,false},
{0x9fc43358u,0xc21025u,false},
{0x9fc4335cu,0x431025u,false},
{0x9fc43360u,0x471825u,false},
{0x9fc43364u,0x240403e8u,false},
{0x9fc43368u,0x52a00u,false},
{0x9fc4336cu,0x1642018u,false},
{0x9fc43370u,0x3c02b000u,false},
{0x9fc43374u,0xad430000u,false},
};
e->reset(trace[0].pc);
// Entry argument r4 equals r16 via the logged JAL delay slot; r16=47FC4 from final snapshot.
// Other routine inputs r6-r9 are zero; r5=9600 from the preceding logged ORI.
e->r[4].low=0x47fc4;e->r[5].low=0x9600;
for(const auto& x:trace)for(unsigned n=0;n<4;n++)e->bios[(x.pc&0x3fffff)+n]=uint8_t(x.word>>(8*n));
for(const auto& x:trace){ck(e->pc==x.pc&&e->delay==x.delay,"log8 SIO routine PCs/delay");e->step();}
ck(e->pc==0x9fc43378&&e->serial.lcr==0x20&&e->r[4].low==0x119315a0&&e->first_fault.empty(),"supplied serial routine reaches next PC and reproduces baud calculation");
std::cout<<"PASS: EE SIO registers, lanes/aliases, SDK init, console capture/guards and log8 routine replay to 9FC43378\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
