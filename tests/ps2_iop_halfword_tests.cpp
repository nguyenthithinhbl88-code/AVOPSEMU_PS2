#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"unsupported access must fail");}
int main(){try{auto e=std::make_unique<ps2::Ee>();e->reset();
for(uint32_t a:{0x1f801472u,0x9f801472u,0xbf801472u})ck(e->read(a,2)==0,"reset/aliases");
e->write(0xbf801472,0x12345678,2);ck(e->read(0x9f801472,2)==0x5678,"halfword truncation and shared aliases");
const auto reads=e->iop_boot_1472_reads,writes=e->iop_boot_1472_writes;
for(unsigned n:{1u,4u,8u}){fails([&]{e->read(0xbf801472,n);});fails([&]{e->write(0xbf801472,0,n);});}
fails([&]{e->read8(0xbf801472);});fails([&]{e->write8(0xbf801473,0);});fails([&]{e->write(0xbf801470,0,4);});fails([&]{e->write128(0xbf801470,{0,0});});
for(uint32_t a:{0xbf80146cu,0xbf801474u}){fails([&]{e->read(a,2);});fails([&]{e->write(a,0,2);});}
ck(e->iop_boot_1472==0x5678&&e->iop_boot_1472_reads==reads&&e->iop_boot_1472_writes==writes,"faults preserve latch/counters");
e->reset(0x9fc430d4);ck(!e->iop_boot_1472&&!e->iop_boot_1472_reads&&!e->iop_boot_1472_writes,"reset clears latch");
e->r[2].low=0xffffffffbf801472ull;e->r[4]={1,0xdeadbeef};uint32_t sh=0xa4440000;for(unsigned n=0;n<4;n++)e->bios[0x430d4+n]=uint8_t(sh>>(8*n));e->step();
ck(e->pc==0x9fc430d8&&e->iop_boot_1472==1&&e->iop_boot_1472_writes==1&&e->r[4].high==0xdeadbeef&&e->first_fault.empty(),"reported SH advances, writes 1 and preserves source");
struct Entry{uint32_t pc,word;bool delay;};const Entry trace[]={
{0x9fc42f14u,0x0u,true},
{0x9fc42f18u,0x10e00033u,false},
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
};
e->reset(trace[0].pc);e->delay=true;
// Entry snapshot unavailable: r7 nonzero is inferred from the logged BEQ not taken; r8=0 from final snapshot.
e->r[7].low=1;
for(const auto& x:trace)for(unsigned n=0;n<4;n++)e->bios[(x.pc&0x3fffff)+n]=uint8_t(x.word>>(8*n));
for(const auto& x:trace){ck(e->pc==x.pc&&e->delay==x.delay,"logged PC/delay sequence");e->step();}
ck(e->instructions==64&&e->pc==0x9fc430d8&&e->first_fault.empty()&&e->iop_boot_1472==1&&e->iop_boot_1472_writes==1,"64-attempt replay crosses fault");
ck(e->gs.read(0x12000010,8)==0x740814504ull&&e->gs.read(0x12000040,8)==0x7f5b61f03c040ull&&e->gs.read(0x12000060,8)==0xc7800601a01801ull,"GS timing writes from supplied trace");
ck(e->diagnostic().find("IOP_BOOT_1472=1")!=std::string::npos,"diagnostic latch");
std::cout<<"PASS: IOP 1472 strict halfword latch and supplied 64-attempt GS/SH replay to 9FC430D8\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
