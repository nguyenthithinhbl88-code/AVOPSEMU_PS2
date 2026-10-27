#include "ps2_ee.hpp"
#include <iostream>
#include <algorithm>
#include <memory>
#include <random>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
uint32_t opcode(unsigned sub,unsigned dest=3){return (0x1cu<<26)|(1u<<21)|(2u<<16)|(dest<<11)|(sub<<6)|0x28;}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();e->reset();
 e->r[1]={0x00000001ffffffffull,0x7fffffff80000000ull};e->r[2]={0xffffffff00000001ull,0x0000000180000000ull};
 ck(e->execute_mmi(opcode(16)),"PADDUW decoded");ck(e->r[3].low==~0ull&&e->r[3].high==0x80000000ffffffffull,"unsigned word saturation at max, including bit31 inputs");
 e->execute_mmi(opcode(17));ck(e->r[3].low==0xfffffffeull&&e->r[3].high==0x7ffffffe00000000ull,"PSUBUW clamps underflow at zero");
 std::mt19937 random(0x7000d428);unsigned comparisons=0;
 for(unsigned width:{8u,16u,32u})for(unsigned subtract=0;subtract<2;subtract++)for(unsigned repeat=0;repeat<300;repeat++){
  ps2::Register a{uint64_t(random())|(uint64_t(random())<<32),uint64_t(random())|(uint64_t(random())<<32)};
  ps2::Register b{uint64_t(random())|(uint64_t(random())<<32),uint64_t(random())|(uint64_t(random())<<32)},expected{};
  const uint64_t maximum=(1ull<<width)-1;
  for(unsigned bit=0;bit<128;bit+=width){
   const auto x=((bit<64?a.low:a.high)>>(bit&63))&maximum,y=((bit<64?b.low:b.high)>>(bit&63))&maximum;
   uint64_t answer;if(subtract)answer=x>y?x-y:0;else answer=x>maximum-y?maximum:x+y;
   (bit<64?expected.low:expected.high)|=answer<<(bit&63);comparisons++;
  }
  const unsigned sub=(width==32?16:width==16?20:24)+subtract,dest=repeat%4;
  e->r[1]=a;e->r[2]=b;e->r[3]={0x1234,0x5678};ck(e->execute_mmi(opcode(sub,dest)),"saturating group decoded");
  if(dest)ck(e->r[dest].low==expected.low&&e->r[dest].high==expected.high,"packed saturation including overlapping source/destination");
  else ck(!e->r[0].low&&!e->r[0].high,"zero register remains immutable");
 }
 e->r[3]={0x1234,0x5678};ck(!e->execute_mmi(opcode(18))&&e->r[3].low==0x1234&&e->r[3].high==0x5678,"nearby unimplemented MMI opcode still rejects atomically");
 // Replay inserted below: exact attempted instruction sequence from user's v32 log.
 struct Entry{uint32_t pc,word;bool delay;};
 const Entry trace[]={  {0xbfc00000u,0x401a7800u,false},
  {0xbfc00004u,0x00000000u,false},
  {0xbfc00008u,0x2b410059u,false},
  {0xbfc0000cu,0x14200005u,false},
  {0xbfc00010u,0x00000000u,true},
  {0xbfc00014u,0x3c1abfc0u,false},
  {0xbfc00018u,0x375a0800u,false},
  {0xbfc0001cu,0x03400008u,false},
  {0xbfc00020u,0x00000000u,true},
  {0xbfc00800u,0x3c1a0007u,false},
  {0xbfc00804u,0x375a3003u,false},
  {0xbfc00808u,0x409a8000u,false},
  {0xbfc0080cu,0x0000040fu,false},
  {0xbfc00810u,0x3c1a7040u,false},
  {0xbfc00814u,0x409a6000u,false},
  {0xbfc00818u,0x0000040fu,false},
  {0xbfc0081cu,0x40804800u,false},
  {0xbfc00820u,0x241a0001u,false},
  {0xbfc00824u,0x409a5800u,false},
  {0xbfc00828u,0x0000040fu,false},
  {0xbfc0082cu,0x2402ffffu,false},
  {0xbfc00830u,0x3c03b000u,false},
  {0xbfc00834u,0x3463f500u,false},
  {0xbfc00838u,0xac620000u,false},
  {0xbfc0083cu,0x24030000u,false},
  {0xbfc00840u,0x24040000u,false},
  {0xbfc00844u,0x3c057000u,false},
  {0xbfc00848u,0x3c068000u,false},
  {0xbfc0084cu,0x34c60007u,false},
  {0xbfc00850u,0x24070007u,false},
  {0xbfc00854u,0x40830000u,false},
  {0xbfc00858u,0x40842800u,false},
  {0xbfc0085cu,0x40855000u,false},
  {0xbfc00860u,0x40861000u,false},
  {0xbfc00864u,0x40871800u,false},
  {0xbfc00868u,0x0000040fu,false},
  {0xbfc0086cu,0x42000002u,false},
  {0xbfc00870u,0x0000040fu,false},
  {0xbfc00874u,0x7000d428u,false}};
 e->reset(0xbfc00000);std::fill(e->bios.begin(),e->bios.end(),0);
 for(const auto& item:trace){const auto offset=item.pc-0xbfc00000;for(unsigned n=0;n<4;n++)e->bios[offset+n]=uint8_t(item.word>>(n*8));}
 for(const auto& item:trace){ck(e->pc==item.pc&&e->delay==item.delay,"replay follows logged control flow");e->step();}
 ck(e->pc==0xbfc00878&&e->instructions==39&&e->first_fault.empty(),"BIOS trace advances past BFC00874");
 ck(!e->r[26].low&&!e->r[26].high,"PADDUW r26,r0,r0 clears the whole 128-bit register");
 ck(e->tlb[0].written&&e->tlb[0].hi==0x70000000&&e->tlb[0].lo0==0x80000007&&e->cop0[9]==23,"prior BIOS TLB/COP0 initialization retained");
 std::cout<<"PASS: 6 unsigned saturating operations, "<<comparisons<<" lane checks/alias/zero, unsupported neighbor and 39-instruction user BIOS replay to BFC00878\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
