#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"unsupported access must fail");}
ps2::Register qw(uint32_t a,uint32_t b,uint32_t c,uint32_t d){return {uint64_t(a)|(uint64_t(b)<<32),uint64_t(c)|(uint64_t(d)<<32)};}
int main(){try{auto e=std::make_unique<ps2::Ee>();e->reset();unsigned registers=0;
for(unsigned ch=0;ch<2;ch++){
 const uint32_t base=0x10003800+ch*0x400, fifo=0x10004000+ch*0x1000;
 for(unsigned i=0;i<24;i++){
  const bool valid=i!=15&&(ch||!(i==10||i==11||i==12||i==14));
  if(valid){ck(e->read(base+i*16,4)==0,"VIF reset register map");registers++;}
  else fails([&]{e->read(base+i*16,4);});
 }
 e->write((base|0xa0000000)+0x20,~0u,4);ck(e->read((base|0x80000000)+0x20,4)==7,"ERR mask and aliases");
 e->write128(fifo,qw(0,0x01000404,0x05000002,0x0700beef));
 ck(e->read(base+0x40,4)==0x404&&e->read(base+0x50,4)==2&&e->read(base+0x30,4)==0xbeef&&(e->read(base,4)&0x40),"NOP/STCYCL/STMOD/MARK command effects");
 e->write(base+0x30,0x12345678,4);ck(e->read(base+0x30,4)==0x5678&&!(e->read(base,4)&0x40),"EE MARK write masks value and clears MRK");
 for(unsigned i=4;i<24;i++){const bool valid=i!=15&&(ch||!(i==10||i==11||i==12||i==14));if(valid){auto old=e->read(base+i*16,4);e->write(base+i*16,~0u,4);ck(e->read(base+i*16,4)==old,"command-produced registers ignore CPU writes");}}
 e->write128(fifo,qw(0x20000000,0xdeadbeef,0x30000000,0x11111111));
 ck(e->read(base+0x70,4)==0xdeadbeef&&e->vif[ch].payload==3&&(e->read(base,4)&3)==1,"MASK payload and split ROW begin");
 e->write128(fifo,qw(0x22222222,0x33333333,0x44444444,0x31000000));
 e->write128(fifo,qw(0xaaaaaaaa,0xbbbbbbbb,0xcccccccc,0xdddddddd));
 for(unsigned i=0;i<4;i++){ck(e->read(base+0x100+i*16,4)==0x11111111u*(i+1),"ROW payload across packets");ck(e->read(base+0x140+i*16,4)==0xaaaaaaaau+0x11111111u*i,"COL payload across packets");}
 ck(!e->vif[ch].payload&&(e->read(base,4)&3)==0&&e->vif[ch].packets==4,"idle after all payload words");
 auto before=e->vif[ch];fails([&]{e->write128(fifo,qw(0x01000808,0,0,0x60000000));});
 ck(e->vif[ch].regs==before.regs&&e->vif[ch].packets==before.packets&&e->vif[ch].payload==before.payload,"unsupported fourth word rolls back entire quadword");
 e->write(base+0x20,0,4);before=e->vif[ch];fails([&]{e->write128(fifo,qw(0x80000000,0,0,0));});ck(e->vif[ch].regs==before.regs,"unmodeled interrupt rejected atomically");
 e->write(base+0x20,1,4);e->write128(fifo,qw(0x80000000,0,0,0));ck(!(e->read(base,4)&0xc00),"MII suppresses command interrupt/stall");
 e->write(base+0x10,6,4);ck((e->read(base,4)&0x300)==0x300&&e->read(base+0x10,4)==0,"force break and stop flags, FBRST read policy");
 before=e->vif[ch];fails([&]{e->write128(fifo,{0,0});});ck(e->vif[ch].packets==before.packets,"stalled FIFO has no progress");
 e->write(base+0x10,8,4);ck(!(e->read(base,4)&0x3f00),"stall cancel");e->write128(fifo,{0,0});
 e->write(base,0x800000,4);ck(bool(e->read(base,4)&0x800000)==bool(ch),"FDR writable on VIF1 only");
 if(ch){fails([&]{e->write128(fifo,{0,0});});e->write(base,0,4);}
 e->write128(fifo,qw(0,0,0,0x30000000));ck(e->vif[ch].payload==4,"pending payload before reset");
 e->write(base+0x10,1,4);ck(!e->vif[ch].payload&&!e->vif[ch].packets&&e->read(base,4)==0&&e->read(base+0x20,4)==0,"reset clears bank and partial stream");
 fails([&]{e->read(base+4,4);});fails([&]{e->write(base+0x20,0,2);});fails([&]{e->write(base+0x20,0,8);});fails([&]{e->write(fifo,0,4);});
}
ck(registers==42,"42 VIF register locations");
// Authored EE SQ through the VIF1 alias decodes one complete command packet.
e->reset(0x100000);e->r[1].low=0xb0005000;e->r[2]=qw(0x01000202,0x05000001,0x07001234,0);e->write(0x100000,0x7c220000,4);e->step();ck(e->vif[1].regs[4]==0x202&&e->vif[1].regs[5]==1&&e->vif[1].regs[3]==0x1234,"EE CPU SQ reaches VIF decoder");
const auto state=e->vif[1];ck(e->diagnostic().find("VIF1 configuration only")!=std::string::npos&&e->vif[1].regs==state.regs,"diagnostic snapshot");
std::cout<<"PASS: 42 VIF registers, reset/stall/FDR/RO/ERR/MARK, seven config commands, split payloads, atomic rejection and EE SQ pipeline\n";
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
