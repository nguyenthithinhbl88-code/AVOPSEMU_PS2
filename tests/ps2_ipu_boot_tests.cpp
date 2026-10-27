#include "ps2_ee.hpp"
#include <memory>
#include <iostream>
#include <stdexcept>
static void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool b=false;try{f();}catch(const std::runtime_error&){b=true;}ck(b,"unsupported IPU access rejected");}
int main(){try{auto e=std::make_unique<ps2::Ee>();
 for(auto alias:{0u,0x80000000u,0xa0000000u}){
  e->reset();auto a=0x10002000u|alias;
  ck(e->read(a,8)==0&&e->read(a+0x10,4)==0&&e->read(a+0x20,4)==0&&e->read(a+0x30,8)==0,"IPU reset registers/aliases");
  e->write(a+0x10,0x07f30000,4);ck(e->read(a+0x10,4)==0x07f10000,"CTRL writable bits and invalid precision reference policy");
  e->write(a,0x12345678000000ffull,8);ck(e->ipu.bp==127&&e->ipu.commands==1&&(e->intc_stat&256),"BCLR low32 command and 7bit BP with IRQ8");
  e->write(0x1000f000,256,4);ck(!(e->intc_stat&256),"IPU INTC acknowledge");
  e->write(a,0x91ff01ab,4);ck(e->ipu.threshold0==0x1ab&&e->ipu.threshold1==0x1ff&&(e->intc_stat&256),"SETTH stores documented threshold fields and signals completion");
  e->write(a+0x20,0xffffffff,4);e->write(a+0x30,~0ull,8);ck(e->read(a+0x20,4)==127&&e->read(a+0x30,8)==0,"BP/TOP read-only state");
  const auto before=e->ipu;const auto irq=e->intc_stat;
  fails([&]{e->write(a,0x10000000,4);});fails([&]{e->write128(a,{0,0});});fails([&]{e->read(a+0x10,8);});fails([&]{e->read(a,2);});fails([&]{e->read(a+0x40,4);});
  ck(e->ipu.commands==before.commands&&e->ipu.writes==before.writes&&e->intc_stat==irq,"unsupported decode rejected without completion or mutation");
  e->write(a+0x10,0x40000000,4);ck(!e->ipu.control&&!e->ipu.bp&&e->ipu.resets==1&&e->ipu.threshold0==0x1ab,"soft reset clears control/BP, retains SETTH thresholds reference policy");
 }
 // Authored EE startup code resets/configures IPU, issues BCLR, reads BP.
 e->reset(0x100000);e->r[1].low=0x10002000;e->r[2].low=0x40000000;e->r[3].low=0x27;
 const uint32_t code[]={0xac220010,0xac230000,0x8c240020};for(unsigned i=0;i<3;i++){e->write(0x100000+i*4,code[i],4);}
 for(unsigned i=0;i<3;i++){e->step();}ck(e->r[4].low==0x27&&e->pc==0x10000c,"CPU-to-IPU bootstrap path");
 const auto reads=e->ipu.reads;e->diagnostic();ck(e->ipu.reads==reads,"snapshot never reads MMIO");
 std::cout<<"PASS: IPU reset/config, BCLR/SETTH IRQ8, masks/aliases/widths/read-only registers, atomic unsupported guards and EE startup fixture\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
