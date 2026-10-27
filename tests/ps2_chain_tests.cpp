#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool yes=false;try{f();}catch(const std::runtime_error&){yes=true;}ck(yes,"expected explicit error");}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();
 auto reset=[&](){e->reset();e->write(0x1000e000,1,4);};
 auto tag=[&](uint32_t at,unsigned id,unsigned qwc,uint32_t address=0,bool irq=false,unsigned pce=0){
  e->write128((at&0x80000000u)?0x70000000u+(at&0x3fff):at,{uint64_t(qwc)|(uint64_t(id)<<28)|(uint64_t(irq)<<31)|(uint64_t(pce)<<26)|(uint64_t(address)<<32),0});
 };
 auto packet=[&](uint32_t at,uint32_t value){const auto cpu=(at&0x80000000u)?0x70000000u+(at&0x3fff):at;e->write128(cpu,{1ull|(1ull<<60),14});e->write128(cpu+16,{0xffffffff00000000ull|value,0x62});};
 auto start=[&](uint32_t at,uint32_t chcr=0x105){e->write(0x1000a030,at,4);e->write(0x1000a000,chcr,4);};
 auto run=[&](){for(unsigned n=0;n<200&&(e->dma[2].chcr&0x100);n++)e->hardware_tick();ck(!(e->dma[2].chcr&0x100),"chain completes within bounded bus ticks");};
 reset();tag(0x1000,1,2);packet(0x1010,1);tag(0x1030,3,2,0x2000);packet(0x2000,2);
 tag(0x1040,2,2,0x3000);packet(0x1050,3);tag(0x3000,5,2,0x4000);packet(0x3010,4);
 tag(0x4000,6,2);packet(0x4010,5);tag(0x3030,7,2);packet(0x3040,6);start(0x1000);run();
 ck(e->dma_tags==6&&e->dma_quadwords==12&&uint32_t(e->gs.regs[18]>>32)==6,"CNT/REF/NEXT/CALL/RET/END data reaches GS in order");
 ck(e->dma[2].asr0==0x3030&&!(e->dma[2].chcr&0x30)&&(e->dmac[1]&4),"CALL return address, ASP pop and completion IRQ");
 reset();tag(0x1000,0,2,0x2000);packet(0x2000,11);start(0x1000);run();ck(e->dma_tags==1&&uint32_t(e->gs.regs[18]>>32)==11&&e->dma[2].tadr==0x1010,"REFE external payload and termination");
 reset();tag(0x1000,4,2,0x2000);packet(0x2000,12);tag(0x1010,7,0);start(0x1000);run();ck(e->dma_tags==2&&uint32_t(e->gs.regs[18]>>32)==12,"REFS works without stall control");
 reset();tag(0x1000,3,2,0x2000,true);packet(0x2000,13);start(0x1000,0x185);run();ck(e->dma_tags==1&&uint32_t(e->gs.regs[18]>>32)==13&&e->dma[2].tadr==0x1010,"TIE stops after IRQ tag payload");
 reset();packet(0x2000,14);e->write(0x1000a010,0x2000,4);e->write(0x1000a020,2,4);start(0x1000,0x70000105);run();ck(!e->dma_tags&&uint32_t(e->gs.regs[18]>>32)==14,"resumed END payload finishes without refetching a tag");
 reset();tag(0x80003ff0,7,2);packet(0x80000000,15);start(0x80003ff0);run();ck(uint32_t(e->gs.regs[18]>>32)==15&&e->dma[2].madr==0x80000020,"scratchpad tag and inline payload wrap across 16 KiB");
 reset();tag(0x1000,7,2);packet(0x1010,16);start(0x1000);e->write(0x10003010,1,4);for(unsigned n=0;n<20;n++)e->hardware_tick();ck(!e->dma_tags,"GIF mask pauses chain before tag fetch");e->write(0x10003010,0,4);run();ck(uint32_t(e->gs.regs[18]>>32)==16,"unmask resumes parser/chain");
 reset();tag(0x1000,5,0,0x2000);tag(0x2000,5,0,0x3000);tag(0x3000,6,0);tag(0x2010,6,0);tag(0x1010,7,0);start(0x1000);run();ck(e->dma_tags==5&&e->dma[2].asr0==0x1010&&e->dma[2].asr1==0x2010&&!(e->dma[2].chcr&0x30),"nested CALL/RET uses both return slots");
 reset();e->dmac[2]=1u<<18;tag(0x1000,1,0,0,false,3);tag(0x1010,7,0,0,false,2);start(0x1000);run();ck(!(e->dmac[2]&0x80000000u)&&e->dma_tags==2,"tag PCE enables then disables D_PCR priority control");
 reset();tag(0x1000,5,0,0x2000);start(0x1000,0x125);e->hardware_tick();fails([&]{e->hardware_tick();});ck(e->dma[2].tadr==0x1000&&!e->dma_tags,"CALL stack overflow is atomic");
 reset();tag(0x1000,6,0);start(0x1000);run();ck(e->dma_tags==1,"RET on empty stack terminates");
 reset();start(uint32_t(e->ram.size()));e->hardware_tick();fails([&]{e->hardware_tick();});ck((e->dmac[1]&0x8000)&&!(e->dma[2].chcr&0x100)&&!e->dma_tags,"tag fetch BUSERR retains failed progress");
 reset();tag(0x1000,2,0,0x1000);start(0x1000);for(unsigned n=0;n<1000;n++)e->hardware_tick();ck(e->dma_tags==500&&(e->dma[2].chcr&0x100),"zero-length cycle yields after every tag");
 e->chain_demo();for(unsigned n=0;n<500;n++)e->step();ck(e->dma_tags==2&&e->dma_quadwords==32&&e->gs.primitives==6&&e->pc>=0x100050&&e->gs.scanout().enabled,"actual EE start/wait program drives DMA/GIF/GS");
 std::cout<<"PASS: all eight GIF source-chain tags, CALL/RET, IRQ/TIE, resume, scratchpad wrap, pause, bounded cycles/BUSERR and EE-to-GS program\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
