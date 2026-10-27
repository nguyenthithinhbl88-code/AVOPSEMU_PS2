#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
uint32_t op(unsigned code,unsigned rs=1,unsigned rt=2,unsigned imm=0){return (code<<26)|(rs<<21)|(rt<<16)|imm;}
uint32_t mmi(unsigned fn,unsigned sub=0,unsigned dest=3){return op(0x1c)|(dest<<11)|(sub<<6)|fn;}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();e->reset();
 auto one=[&](uint32_t ins){e->pc=0x100000;e->next=e->pc+4;e->delay=false;e->first_fault.clear();e->write(e->pc,ins,4);e->step();};
 for(unsigned size:{4u,8u})for(unsigned n=0;n<size;n++)for(bool left:{false,true}){
  for(unsigned byte=0;byte<16;byte++)e->write8(0x2000+byte,uint8_t(0x80+byte));
  const uint64_t previous=0x123456789abcdef0ull;e->r[1].low=0x2000+n;e->r[2]={previous,0x11223344};
  uint64_t expected=previous;const unsigned first=left?size-1-n:0,count=left?n+1:size-n;
  for(unsigned k=0;k<count;k++){const unsigned dst=first+k,src=left?k:n+k;expected=(expected&~(255ull<<(dst*8)))|(uint64_t(0x80+src)<<(dst*8));}
  if(size==4)expected=uint64_t(int64_t(int32_t(expected)));
  one(op(size==4?(left?0x22:0x26):(left?0x1a:0x1b)));
  ck(e->r[2].low==expected&&e->r[2].high==0x11223344,"unaligned load byte oracle and high-half preservation");
  e->r[2].low=previous;
  one(op(size==4?(left?0x2a:0x2e):(left?0x2c:0x2d)));
  for(unsigned k=0;k<16;k++){
   uint8_t want=uint8_t(0x80+k);
   if(left&&k<=n)want=uint8_t(previous>>((size-1-n+k)*8));
   if(!left&&k>=n&&k<size)want=uint8_t(previous>>((k-n)*8));
   ck(e->read8(0x2000+k)==want,"unaligned store byte oracle and adjacent memory");
  }
 }
 e->r[1]={0x0123456789abcdef,0x1122334455667788};e->r[2]={0xfedcba9876543210,0x8877665544332211};
 one(mmi(9,18));ck(e->r[3].low==0&&e->r[3].high==0x0022224444222200,"PAND full 128-bit result");
 one(mmi(0x29,18,1));ck(e->r[1].low==~0ull&&e->r[1].high==0x9977775555777799,"POR destination aliases source");
 e->r[1]={11,22};e->r[2]={33,44};one(mmi(9,14,2));ck(e->r[2].low==33&&e->r[2].high==11,"PCPYLD source capture");
 e->r[1]={11,22};e->r[2]={33,44};one(mmi(0x29,14));ck(e->r[3].low==22&&e->r[3].high==44,"PCPYUD order");
 e->r[1]={~0ull,~0ull};e->r[2]={0x0101010101010101,0x0101010101010101};one(mmi(8,8));ck(!e->r[3].low&&!e->r[3].high,"PADDB wraps each lane without carry");
 one(mmi(8,0));ck(e->r[3].low==0x0101010001010100ull,"PADDW lane boundaries");
 e->r[1]={0x800000007fffffff,0};e->r[2]={0,0};one(mmi(8,2));ck(e->r[3].low==0xffffffff,"PCGTW signed comparison");
 one(mmi(0x28,2));ck(e->r[3].high==~0ull,"PCEQW compares all four words");
 e->hi.low=55;e->lo.low=66;e->r[1].low=uint64_t(-7ll);e->r[2].low=3;one(mmi(0x1a));ck(e->lo.high==uint64_t(-2ll)&&e->hi.high==uint64_t(-1ll)&&e->hi.low==55&&e->lo.low==66,"DIV1 independent accumulator bank");
 one(mmi(0x12));ck(e->r[3].low==uint64_t(-2ll),"MFLO1");
 e->r[1].low=0xffffffff;one((1u<<21)|0x29);one(3u<<11|0x28);ck(e->r[3].low==0xffffffff,"MTSA/MFSA zero extended register");
 e->r[1].low=6;one(op(1,1,24,3));ck(e->sa==5,"MTSAB");
 e->r[1]={0x1716151413121110,0x1f1e1d1c1b1a1918};e->r[2]={0x0706050403020100,0x0f0e0d0c0b0a0908};
 for(unsigned shift=0;shift<16;shift++){e->sa=shift;one(mmi(0x28,27));for(unsigned n=0;n<16;n++){auto half=n<8?e->r[3].low:e->r[3].high;ck(uint8_t(half>>((n&7)*8))==n+shift,"QFSRV byte funnel across 128-bit boundary");}}
 e->cop0[12]=0;one(0x42000038);ck(e->cop0[12]&0x10000,"EI kernel sets EIE");one(0x42000039);ck(!(e->cop0[12]&0x10000),"DI clears EIE");
 e->cop0[12]=0x10;one(0x42000038);ck(!(e->cop0[12]&0x10000),"EI user mode ignored without EDI");e->cop0[12]|=0x20000;one(0x42000038);ck(e->cop0[12]&0x10000,"EDI permits EI");
 e->cop0[12]=0;e->dmac[2]=4;e->dmac[1]=0;one(op(0x10,8,1,2));ck(e->next==0x100008&&e->delay,"BC0T waits for selected DMA completion");e->dmac[1]=4;one(op(0x10,8,1,2));ck(e->next==0x10000c,"BC0T takes after DMA completion");
 e->dmac[1]=0;one(op(0x10,8,3,2));ck(e->pc==0x100008&&!e->delay,"BC0TL false annuls delay slot");
 e->r[1].low=0xdeadbeef;one(op(0x33));ck(e->first_fault.empty(),"PREF hint does not dereference address");
 std::cout<<"PASS: unaligned 32/64-bit loads/stores, 128-bit MMI, HI1/LO1, SA/QFSRV, EI/DI and DMA condition branches\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
