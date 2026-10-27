#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"invalid DMA access must fail");}
int main(){try{auto e=std::make_unique<ps2::Ee>();e->reset();
 for(unsigned ch:{0u,1u,2u,8u,9u}){const auto a=ps2::Ee::dma_bases[ch]+0x80;e->write(a|0xa0000000,0xabcdefef,4);ck(e->dma[ch].sadr==0x2fe0&&e->read(a|0x80000000,4)==0x2fe0,"SADR aliases/alignment and scratchpad mask");}
 for(unsigned ch=3;ch<8;ch++){e->write(ps2::Ee::dma_bases[ch]+0x80,0xdeadbeef,4);ck(e->read(ps2::Ee::dma_bases[ch]+0x80,4)==0xdeadbeef&&!e->dma[ch].sadr,"unused bootstrap slot is separate from functional SADR");}
 for(unsigned bytes:{1u,2u,8u})fails([&]{e->write(0xb0008080,0,bytes);});
 fails([&]{e->write(0xb0008084,0,4);});fails([&]{e->write(0xb0008070,0,4);});
 e->dma[0].chcr=0x101;const auto before=e->dma[0].sadr;fails([&]{e->write(0xb0008080,0,4);});ck(e->dma[0].sadr==before,"active DMA config rejected atomically");
 e->reset(0x8000d9e0);e->r[4].low=0x31f;e->r[29].low=0xffffffff80018e40ULL;
 const uint32_t tail[]={0x0080482d,0x27bdfff0,0x0000182d,0x3c048001,0x24845a78,0x00000000,0x00691007,0x30420001,0x10400008,0x24630001,0x8c820000,0xac400080};
 e->write(0x8000d9e0,0x0c00377a,4);e->write(0x8000d9e4,0x2404031f,4);e->write(0x80015a78,0xb0008000,4);
 for(unsigned i=0;i<12;i++)e->write(0x8000dde8+i*4,tail[i],4);
 for(unsigned i=0;i<14;i++)e->step();
 ck(e->pc==0x8000de18&&e->dma[0].sadr==0&&e->first_fault.empty()&&e->instructions==14,"exact user log DMAC init tail reaches 8000DE18");
 ck(!e->gs.regs[0]&&!e->dma_quadwords,"no fabricated display or transfer");
 e->reset();for(const auto& d:e->dma)ck(!d.sadr,"reset clears SADR banks");
 std::cout<<"PASS: DMA0/1/2 SADR bootstrap registers, aliases/mask/strict guards and log tail to 8000DE18\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
