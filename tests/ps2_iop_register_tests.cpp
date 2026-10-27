#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"unsupported access must fail");}
int main(){try{auto e=std::make_unique<ps2::Ee>();e->reset();
// All 21 configuration words, mixed lanes, both kernel aliases and no cross-word spill.
for(auto a:e->iop_config_addresses){ck(e->read(a,4)==0,"config reset policy");e->write(a|0xa0000000,0xaabbccdd,4);e->write((a|0x80000000)+1,0x12,1);e->write(a+2,0x3456,2);ck(e->read(a,4)==0x345612dd&&e->read(a+1,1)==0x12&&e->read(a+2,2)==0x3456,"config lanes/aliases");}
e->write(0xbf801470,0xabcd,2);e->write(0xbf801472,1,2);ck(e->read(0x9f801470,2)==0xabcd&&e->read(0x1f801472,2)==1,"independent bootstrap halfwords");
for(unsigned ch=0;ch<13;ch++){
 uint32_t a=ch<7?0x1f801080+ch*16:0x1f801500+(ch-7)*16;
 e->write(a,0xff123457,4);e->write(a+4,0xaabbccdd,4);e->write(a+8,0x201,4);
 ck(e->read(a,4)==0x123457&&e->read(a+4,4)==0xaabbccdd&&e->read(a+8,4)==0x201,"13 DMA config banks and 24-bit MADR");
 e->write(a+6,0x1234,2);ck(e->read(a+4,4)==0x1234ccdd,"DMA high halfword");
 const auto count=e->iop_register_writes;fails([&]{e->write(a+8,0x01000201,4);});fails([&]{e->write(a+10,0x100,2);});ck(e->read(a+8,4)==0x201&&e->iop_register_writes==count,"DMA start rejected atomically at both widths");
 if(ch==9){e->write(a+12,0xff654321,4);ck(e->read(a+12,4)==0x654321,"SIF0 TADR");}else fails([&]{e->write(a+12,0,4);});
}
ck(e->read(0xbf8010f0,4)==0x07777777,"PS2 DPCR reset");
e->write(0xbf801570,0x12345678,4);ck(e->read(0x9f801570,4)==0x12345678,"DPCR2 config");
e->iop_stat=0xaabbccdd;e->write(0xbf801072,0x0f0f,2);ck(e->iop_stat==0x0a0bccdd,"I_STAT W0C upper lane");e->write(0xbf801070,0xf0,1);ck(e->iop_stat==0x0a0bccd0,"I_STAT W0C byte preserves other lanes");
e->write(0xbf801074,0xfedcba98,4);e->write(0xbf801078,0xffffffff,4);ck(e->iop_mask==0xfedcba98&&e->iop_ctrl==1,"I_MASK/CTRL mask");
const auto reads=e->iop_register_reads;auto report=e->diagnostic();ck(e->iop_ctrl==1&&e->iop_register_reads==reads&&report.find("IOP_DMA12")!=std::string::npos,"diagnostic is side-effect free");
ck(e->read(0xbf801078,4)==1&&!e->iop_ctrl&&e->read(0xbf801078,1)==0,"I_CTRL read-clear once");
e->iop_dmac[1]=0x05000000;e->iop_dmac[3]=0x03000000;
e->write(0xbf8010f4,0x00800000,4);ck(e->iop_dmac[1]==0x05800000,"master gated off by DMACINTEN");e->write(0xbf80157c,1,4);ck((e->iop_dmac[1]&0x80000000)&& (e->iop_stat&8),"DMA summary plus IOP IRQ3");
e->write(0xbf8010f6,0x0580,2);ck((e->iop_dmac[1]&0x7f000000)==0&&(e->iop_dmac[1]&0x80000000),"DICR W1C keeps DICR2 pending summary");e->write(0xbf801574,0x03000000,4);ck(!(e->iop_dmac[1]&0x80000000)&&!(e->iop_dmac[3]&0x3f000000),"DICR2 W1C updates combined summary");
e->iop_stat=0;e->write(0xbf80157c,2,4);e->write(0xbf8010f4,0x8000,4);ck((e->iop_dmac[1]&0x80000000)&&!e->iop_stat,"bus error summary despite global disable, IRQ suppression");
e->write(0xbf801578,0xffffffff,4);ck(e->read(0xbf801578,4)==1,"DMACEN masks bits");
for(uint32_t a:{0x1f801024u,0x1f801424u,0x1f801474u,0x1f803205u,0x1f900000u})fails([&]{e->write(a,0,4);});
fails([&]{e->write(0xbf801000,0,8);});fails([&]{e->write(0xbf801001,0,2);});
// Independent IOP RAM and all four VU stores, full-width SQ/LQ-compatible memory.
const uint32_t bases[]={0x1c000000,0x11000000,0x11004000,0x11008000,0x1100c000};
const unsigned sizes[]={2*1024*1024,4096,4096,16384,16384};
for(unsigned k=0;k<5;k++){
 auto a=bases[k];ck(e->read(a,8)==0,"fresh memory zero");e->write128(a|0xa0000000,{0x1122334455667788ull,0x99aabbccddeeff00ull});
 ck(e->read(a|0x80000000,8)==0x1122334455667788ull&&e->read(a+8,8)==0x99aabbccddeeff00ull,"new memory aliases and quadword store");
 e->write(a+sizes[k]-8,0xfedcba9876543210ull,8);ck(e->read(a+sizes[k]-8,8)==0xfedcba9876543210ull,"last valid memory bytes");
 if(k!=3)fails([&]{e->write8(a+sizes[k],0);}); // VU1 code ends exactly where VU1 data begins.
}
ck(e->read(0,8)==0,"IOP RAM distinct from EE RAM");e->reset();ck(e->read(0x1c000000,8)==0x1122334455667788ull&&!e->iop_register_reads&&!e->iop_register_writes&&!e->iop_boot_1470&&!e->iop_boot_1472,"EE reset retains memory and resets peripherals");
// Authored EE program: SQ to IOP RAM, LQ from alias, then SQ into VU1 data.
e->reset(0x100000);e->r[1].low=0xbc000000;e->r[2]={0x0123456789abcdefull,0xfedcba9876543210ull};e->r[4].low=0xb100c000;
const uint32_t code[]={0x7c220000,0x78230000,0x7c830000};for(unsigned i=0;i<3;i++)e->write(0x100000+i*4,code[i],4);for(unsigned i=0;i<3;i++)e->step();
ck(e->r[3].low==e->r[2].low&&e->r[3].high==e->r[2].high&&e->read(0x1100c008,8)==e->r[2].high,"EE SQ/LQ IOP-to-VU authored pipeline");
struct Entry{uint32_t pc,word;bool delay;};const Entry trace[]={
{0x9fc42f1cu,0x0u,true},
{0x9fc42f20u,0x3c07b200u,false},
{0x9fc42f24u,0x24030007u,false},
{0x9fc42f28u,0x31c38u,false},
{0x9fc42f2cu,0x34634083u,false},
{0x9fc42f30u,0x31c38u,false},
{0x9fc42f34u,0x34634504u,false},
{0x9fc42f38u,0x34e70010u,false},
{0x9fc42f3cu,0x3c02b200u,false},
{0x9fc42f40u,0xfce30000u,false},
{0x9fc42f44u,0x34420040u,false},
{0x9fc42f48u,0x3c040007u,false},
{0x9fc42f4cu,0x3484f5b6u,false},
{0x9fc42f50u,0x42438u,false},
{0x9fc42f54u,0x34841f03u,false},
{0x9fc42f58u,0x42438u,false},
{0x9fc42f5cu,0x3484c040u,false},
{0x9fc42f60u,0x3c03b200u,false},
{0x9fc42f64u,0xfc440000u,false},
{0x9fc42f68u,0x34630050u,false},
{0x9fc42f6cu,0x3c040033u,false},
{0x9fc42f70u,0x3484a53eu,false},
{0x9fc42f74u,0x92c3cu,false},
{0x9fc42f78u,0xfc640000u,false},
{0x9fc42f7cu,0x3c02b200u,false},
{0x9fc42f80u,0x34420060u,false},
{0x9fc42f84u,0x24040001u,false},
{0x9fc42f88u,0x52bffu,false},
{0x9fc42f8cu,0x3c0600c7u,false},
{0x9fc42f90u,0x34c68006u,false},
{0x9fc42f94u,0x63438u,false},
{0x9fc42f98u,0x34c601a0u,false},
{0x9fc42f9cu,0x63438u,false},
{0x9fc42fa0u,0x34c61801u,false},
{0x9fc42fa4u,0x3c03b200u,false},
{0x9fc42fa8u,0xfc460000u,false},
{0x9fc42facu,0xa42825u,false},
{0x9fc42fb0u,0x34630020u,false},
{0x9fc42fb4u,0xfc650000u,false},
{0x9fc42fb8u,0x3c02b200u,false},
{0x9fc42fbcu,0x34420030u,false},
{0x9fc42fc0u,0x24030009u,false},
{0x9fc42fc4u,0xfc430000u,false},
{0x9fc42fc8u,0x24040007u,false},
{0x9fc42fccu,0x42438u,false},
{0x9fc42fd0u,0x34844081u,false},
{0x9fc42fd4u,0x42438u,false},
{0x9fc42fd8u,0x34844504u,false},
{0x9fc42fdcu,0xfce40000u,false},
{0x9fc42fe0u,0x1000002eu,false},
{0x9fc42fe4u,0x24020040u,true},
{0x9fc4309cu,0x11020005u,false},
{0x9fc430a0u,0x24020060u,true},
{0x9fc430a4u,0x11020003u,false},
{0x9fc430a8u,0x24020061u,true},
{0x9fc430acu,0x15020005u,false},
{0x9fc430b0u,0x0u,true},
{0x9fc430c4u,0x3c02bf80u,false},
{0x9fc430c8u,0x24040001u,false},
{0x9fc430ccu,0x34421472u,false},
{0x9fc430d0u,0x3c03bf80u,false},
{0x9fc430d4u,0xa4440000u,false},
{0x9fc430d8u,0x34631470u,false},
{0x9fc430dcu,0xa4600000u,false},
};
e->reset(trace[0].pc);e->delay=true;
// Tail starts with a logged delay-slot NOP. Subsequent branches read r8=0 (final snapshot).
for(const auto& x:trace)for(unsigned n=0;n<4;n++)e->bios[(x.pc&0x3fffff)+n]=uint8_t(x.word>>(8*n));
for(const auto& x:trace){ck(e->pc==x.pc&&e->delay==x.delay,"log7 PC/delay sequence");e->step();}
ck(e->instructions==64&&e->pc==0x9fc430e0&&e->first_fault.empty()&&e->iop_boot_1470==0&&e->iop_boot_1472==1&&e->iop_register_writes==2,"log7 tail crosses both halfword writes");
ck(e->gs.read(0x12000010,8)==0x740814504ull,"GS configuration retained");
std::cout<<"PASS: 21 config words, 13 IOP DMA banks, interrupt side effects, IOP/VU memory, EE SQ/LQ and log7 replay to 9FC430E0\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
