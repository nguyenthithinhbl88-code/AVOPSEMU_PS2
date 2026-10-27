#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"unsupported VIF/DMA must fail");}
ps2::Register qw(uint32_t a,uint32_t b,uint32_t c,uint32_t d){return {uint64_t(a)|(uint64_t(b)<<32),uint64_t(c)|(uint64_t(d)<<32)};}
void ticks(ps2::Ee& e,unsigned n){for(unsigned i=0;i<n;i++)e.hardware_tick();}
int main(){try{auto e=std::make_unique<ps2::Ee>();
for(unsigned ch=0;ch<2;ch++){
 e->reset();const uint32_t fifo=0x10004000+ch*0x1000,code=ch?0x11008000:0x11000000,data=ch?0x1100c000:0x11004000;
 e->write128(fifo,qw(0x01000101,0x040003ff,0,0));ck(e->vif[ch].regs[9]==(ch?1023u:255u),"ITOPS address masks");
 if(ch){e->write128(fifo,qw(0x03000010,0x02000020,0,0));ck(e->vif[ch].regs[10]==0x10&&e->vif[ch].regs[11]==0x20&&e->vif[ch].regs[12]==0x10,"BASE/OFFSET set TOPS to BASE and clear DBF");}
 e->write128(fifo,qw(0x4a020001,0x11223344,0x55667788,0x99aabbcc));ck(e->vif[ch].payload==1&&e->read(code+8,8)==0x5566778811223344ull,"MPG partial packet writes program words");
 e->write128(fifo,qw(0xddeeff00,0,0,0));ck(!e->vif[ch].payload&&e->read(code+16,8)==0xddeeff0099aabbccull&&!e->vif[ch].regs[6],"MPG payload spans quadwords");
 e->write128(fifo,qw(0x6c020002,1,2,3));e->write128(fifo,qw(4,5,6,7));e->write128(fifo,qw(8,0,0,0));
 for(unsigned i=0;i<8;i++)ck(e->read(data+32+i*4,4)==i+1,"UNPACK V4-32 raw words");
 if(ch){e->write128(fifo,qw(0x6c018001,9,10,11));e->write128(fifo,qw(12,0,0,0));ck(e->read(data+0x110,4)==9&&e->read(data+0x11c,4)==12,"UNPACK uses TOPS when FLG set");}
 // A completed MPG followed by an unsupported command in the same packet must roll back memory too.
 e->write128(fifo,qw(0,0,0,0x4a010000));const auto old=e->read(code,8);const auto before=e->vif[ch];
 fails([&]{e->write128(fifo,qw(0xaaaa,0xbbbb,0x14000000,0));});ck(e->read(code,8)==old&&e->vif[ch].payload==before.payload&&e->vif[ch].packets==before.packets,"packet rollback includes staged VU memory");
 e->write(0x10003810+ch*0x400,1,4);e->write128(fifo,qw(0x01000201,0,0,0));fails([&]{e->write128(fifo,qw(0x6c010000,0,0,0));});
 e->write(0x10003810+ch*0x400,1,4);e->write128(fifo,qw(0x05000001,0,0,0));fails([&]{e->write128(fifo,qw(0x6c010000,0,0,0));});
 // NUM zero means 256 microinstructions (512 words). Program-address wrap stays inside selected VU.
 e->write(0x10003810+ch*0x400,1,4);e->write128(fifo,qw(0,0,0,0x4a00ffff));ck(e->vif[ch].payload==512&&!e->vif[ch].regs[6],"MPG NUM=0");
 for(unsigned i=0;i<128;i++){e->write128(fifo,qw(i*4+1,i*4+2,i*4+3,i*4+4));}ck(!e->vif[ch].payload&&!e->vif[ch].regs[6],"zero count upload completes boundedly");
}
for(unsigned ch=0;ch<2;ch++){
 e->reset();const uint32_t base=ps2::Ee::dma_bases[ch];e->write128(0x200,qw(0x4a010000,0xaabbccdd,0x11223344,0));
 e->write(base+0x10,0x200,4);e->write(base+0x20,1,4);e->write(base,0x101,4);ticks(*e,2);ck(e->dma[ch].qwc==1,"DMA waits global enable");
 e->write(0x1000e000,1,4);e->write(0x1000f590,0x10000,4);ticks(*e,2);ck(e->dma[ch].qwc==1,"DMA disable gate");e->write(0x1000f590,0,4);
 e->write(0x10003810+ch*0x400,2,4);ticks(*e,2);ck(e->dma[ch].qwc==1,"VIF stall pauses DMA");e->write(0x10003810+ch*0x400,8,4);ticks(*e,2);
 ck(!(e->dma[ch].chcr&0x100)&&e->dma[ch].qwc==0&&e->dma[ch].madr==0x210&&(e->dmac[1]&(1u<<ch))&&e->read(ch?0x11008000:0x11000000,8)==0x11223344aabbccddull,"normal DMA uploads actual VU code and completes channel");
 e->write(0x1000e010,(1u<<ch),4);ck(!(e->dmac[1]&(1u<<ch)),"per-channel completion W1C");
 e->write(base+0x10,0x02000000,4);e->write(base+0x20,1,4);e->write(base,0x101,4);fails([&]{ticks(*e,2);});ck(!(e->dma[ch].chcr&0x100)&&(e->dmac[1]&0x8000)&&e->dma[ch].qwc==1,"BUSERR on VIF DMA source bound, no false completion");
 e->reset();fails([&]{e->write(base,0x105,4);});ck(!e->dma[ch].chcr,"unsupported VIF chain rejects before STR");
}
e->reset();fails([&]{e->write(0x10009000,0x100,4);});ck(!e->dma[1].chcr,"VIF1 reverse direction rejected");
// Authored EE starts VIF1 normal DMA and polls completion with BC0; real uploaded bytes are checked.
e->reset(0x100000);e->write128(0x200,qw(0x4a010000,0x01020304,0x05060708,0));
const uint32_t program[]={0x3c081001,0x24090200,0xad099010,0x24090001,0xad099020,0xad09e000,0x24090002,0xad09e020,0x24090101,0xad099000,0x4100ffff,0,0x2402002a};
for(unsigned i=0;i<13;i++){e->write(0x100000+i*4,program[i],4);}
for(unsigned i=0;i<40&&e->r[2].low!=42;i++){e->step();}
ck(e->r[2].low==42&&e->read(0x11008000,8)==0x0506070801020304ull,"EE-to-DMAC-to-VIF-to-VU memory authored integration");
std::cout<<"PASS: VIF address commands, MPG/limited V4-32 UNPACK, staged rollback, normal DMA0/1 gates/errors/IRQ completion and authored EE pipeline\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
