#include "ps2_ee.hpp"
#include <memory>
#include <iostream>
#include <stdexcept>
static void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool b=false;try{f();}catch(const std::runtime_error&){b=true;}ck(b,"GS invalid access rejected");}
int main(){try{auto e=std::make_unique<ps2::Ee>();auto& g=e->gs;
 ck(e->read(0xb2001000,8)==0x551b4000,"GS CSR revision/ID and empty FIFO");
 ck(e->read(0x12001002,1)==0x1b&&e->read(0x12001003,1)==0x55,"BIOS byte reads identify modeled GS revision");
 for(auto alias:{0u,0x80000000u,0xa0000000u}){
  const auto a=0x12000080u|alias;e->write(a,0,8);
  for(unsigned i=0;i<8;i++){e->write(a+i,0x11u*(i+1),1);}
  ck(e->read(a,8)==0x8877665544332211ull,"byte lanes preserve other seven lanes");
  e->write(a+2,0xabcd,2);ck(e->read(a+2,2)==0xabcd&&e->read(a,8)==0x88776655abcd2211ull,"halfword lanes are little endian");
 }
 g.event(3);e->write(0x12001000,1,1);ck((g.regs[15]&31)==2,"CSR byte W1C affects selected flags only");
 e->write(0x12001002,0,2);ck((g.regs[15]>>16)==0x551b,"CSR identity cannot be overwritten");
 e->write(0x12001010,0,4);ck(g.irq(),"pending FINISH unmasked");
 e->write(0x12001011,2,1);ck(!g.irq()&&g.regs[16]==0x6200,"IMR byte mask applies to matching source");
 e->write(0x12001080,~0ull,8);ck(!g.regs[18],"SIGLBLID CPU writes ignored");
 g.vram[12345]=0xaabbccdd;g.draw_write(0x01,0x12345678);g.regs[0]=1;g.regs[14]=0xabcdef;
 g.gif_push(1ull|(1ull<<60),0xe); // pending A+D payload; CSR reset must not destroy GIF framing
 auto packets=g.gif_packets,words=g.gif_words;auto cold=g.resets,soft=g.soft_resets;
 e->write(0x12001001,2,1);
 ck(g.regs[15]==0x551b4000&&g.regs[16]==0x7f00&&!g.irq(),"byte CSR reset resets event/mask state");
 ck(g.vram[12345]==0xaabbccdd&&g.regs[0]==1&&g.regs[14]==0xabcdef,"soft reset preserves VRAM and display configuration");
 ck(!g.drawing[1]&&g.gif_remaining==1&&g.gif_packets==packets&&g.gif_words==words&&g.resets==cold&&g.soft_resets==soft+1,"soft reset clears drawing state and preserves GIF packet/counters");
 g.gif_push(0x11112222,1);ck(g.drawing[1]==0x11112222&&!g.gif_remaining,"GIF payload still consumed after CSR reset");
 const auto writes=g.writes;fails([&]{e->write(0x12000081,0,2);});fails([&]{e->read(0x12000008,1);});fails([&]{e->write(0x12001006,0,4);});ck(g.writes==writes,"rejected access is atomic");
 g.reset();ck(!g.vram[12345]&&!g.regs[0],"power-on reset remains distinct");
 std::cout<<"PASS: GS revision ID, byte/halfword lanes/aliases, CSR W1C/IMR, soft-reset VRAM/display/GIF preservation and strict guards\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
