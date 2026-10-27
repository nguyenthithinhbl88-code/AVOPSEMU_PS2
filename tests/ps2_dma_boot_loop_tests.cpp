#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"invalid bootstrap slot access must fail");}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();e->reset();
 for(unsigned ch=0;ch<10;ch++)for(unsigned off:{0x40u,0x50u,0x80u}){
  const auto a=ps2::Ee::dma_bases[ch]+off;e->write(a|0xa0000000,0xdeadbeef,4);const auto got=e->read(a|0x80000000,4);
  const bool unused=(off==0x80)?(ch>=3&&ch<=7):ch>=3;
  ck(got==(unused?0xdeadbeefu:(off==0x80?0x3ee0u:0xdeadbee0u)),"all channel common slots aliases and masks");
  if(unused){ck(!e->dma[ch].asr0&&!e->dma[ch].asr1,"compatibility storage separate from DMA ASR");if(off==0x80)ck(!e->dma[ch].sadr,"compatibility storage separate from SADR");}
 }
 ck(e->dma_bootstrap_writes==19,"19 bounded compatibility slots");
 const auto before=e->dma_bootstrap_writes;
 for(unsigned bytes:{1u,2u,8u})fails([&]{e->write(0xb000b080,0,bytes);});
 for(unsigned off:{0x60u,0x70u,0x84u,0x90u})fails([&]{e->write(0xb000b000+off,0,4);});
 fails([&]{e->write128(0xb000b080,{0,0});});ck(e->dma_bootstrap_writes==before,"invalid access no mutation");
 e->dma[3].chcr=0x100;fails([&]{e->write(0xb000b080,0,4);});e->dma[3].chcr=0;
 ck(e->read(0xb000b080,4)==0xdeadbeef,"active DMA write rejection atomic");
 e->reset(0x8000dde8);e->r[4].low=0x31f;e->r[29].low=0xffffffff80018e40ULL;
 const uint32_t code[]={0x0080482d,0x27bdfff0,0x0000182d,0x3c048001,0x24845a78,0,0x00691007,0x30420001,0x10400008,0x24630001,0x8c820000,0xac400080,0xac400000,0xac400030,0xac400010,0xac400050,0xac400040,0x2862000a,0x1440fff3,0x24840004};
 for(unsigned i=0;i<20;i++)e->write(0x8000dde8+i*4,code[i],4);
 for(unsigned ch=0;ch<10;ch++){
  e->write(0x80015a78+ch*4,ps2::Ee::dma_bases[ch]|0xa0000000,4);
  e->write(ps2::Ee::dma_bases[ch]+0x10,0x1230,4);
  for(unsigned off:{0x40u,0x50u,0x80u})e->write(ps2::Ee::dma_bases[ch]+off,0x1230,4);
 }
 unsigned attempts=0;while(e->pc!=0x8000de38&&attempts<300){e->step();attempts++;}
 ck(e->pc==0x8000de38&&e->r[3].low==10&&e->first_fault.empty(),"entire observed firmware clear loop exits after ten channels");
 for(unsigned ch=0;ch<10;ch++){
  const bool selected=(0x31f>>ch)&1;
  for(unsigned off:{0x10u,0x30u,0x40u,0x50u,0x80u}){const auto v=e->read(ps2::Ee::dma_bases[ch]+off,4);ck(v==(selected?0u:off==0x30?0u:0x1230u),"selected channels cleared; unselected retain state");}
  ck(!e->dma[ch].chcr&&!e->dma[ch].qwc,"no DMA started or completed by config writes");
 }
 ck(!e->gs.regs[0]&&!e->gs.gif_words&&!e->dma_quadwords,"no forced video/drawing/transfer");
 // Watch successful EE writes to display controls and keep failed attempts out of history.
 e->reset(0x100000);e->r[1].low=0xb2000000;e->r[2].low=1;e->write(e->pc,0xfc220000,4);e->step();
 ck(e->gs_write_count==1&&e->gs_write_history[0].pc==0x100000&&e->gs_write_history[0].value==1,"actual SD PMODE write tracked");
 auto report=e->boot_status();ck(report.find("GS_WRITE instruction=0 PC=100000 address=12000000")!=std::string::npos&&e->gs_write_count==1,"diagnostic snapshot no MMIO side effects");
 fails([&]{e->write(0x12000008,0,8);});ck(e->gs_write_count==1,"failed GS write not recorded as success");
 e->reset();ck(!e->dma_bootstrap_writes&&!e->gs_write_count&&!e->dma_bootstrap_slots[3][2],"reset clears bootstrap/history");
 std::cout<<"PASS: full firmware DMAC clear loop through DMA3/IPU to 8000DE38, 19 bootstrap latches and GS write trace\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
