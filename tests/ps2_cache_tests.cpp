#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool v,const char* m){if(!v)throw std::runtime_error(m);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"unsupported CACHE must fail");}
void code(ps2::Ee& e,uint32_t word){e.write(0x100000,word,4);e.pc=0x100000;e.next=0x100004;e.delay=false;e.first_fault.clear();}
int main(){try{auto e=std::make_unique<ps2::Ee>();e->reset();
 for(unsigned operation:{7u,12u,20u,22u,24u,26u,28u}){
  e->r[1].low=0xffffffff80000200ull;e->cop0[28]=0x12345678;e->cop0[29]=0xabcdef01;e->write(0x1fc,0x11223344,4);
  code(*e,0xbc200000u|(operation<<16)|0xfffc);auto registers=e->r;const auto count=e->instructions;e->step();
  ck(e->pc==0x100004&&e->instructions==count+1&&e->cache_operations[operation]==1&&e->cache_last_address==0x800001fc,"cache decode/signed offset/normal progress");
  for(unsigned n=0;n<32;n++)ck(e->r[n].low==registers[n].low&&e->r[n].high==registers[n].high,"maintenance preserves GPRs");
  ck(e->read(0x1fc,4)==0x11223344&&e->cop0[28]==0x12345678&&e->cop0[29]==0xabcdef01,"maintenance preserves memory and tags in coherent model");
  e->r[1].low=0xb000f180;code(*e,0xbc200000u|(operation<<16));auto reads=e->serial.reads,writes=e->serial.writes;e->step();ck(e->serial.reads==reads&&e->serial.writes==writes&&e->console.empty(),"cache operand never reads/writes MMIO");
 }
 for(unsigned operation:{0u,1u,2u,3u,16u,17u,18u,19u,31u}){
  code(*e,0xbc200000u|(operation<<16));const auto before=e->cache_operations;const auto count=e->instructions;fails([&]{e->step();});ck(e->cache_operations==before&&e->pc==0x100000&&e->instructions==count&&!e->first_fault.empty(),"unknown/tag/data operations rejected atomically");
 }
 for(unsigned ksu:{8u,16u}){e->cop0[12]=ksu;code(*e,0xbc270000);auto before=e->cache_operations;fails([&]{e->step();});ck(e->cache_operations==before,"user/supervisor cache rejected");}
 for(unsigned exception:{2u,4u}){e->cop0[12]=16|exception;code(*e,0xbc270000);e->step();ck(e->pc==0x100004,"EXL/ERL privilege override");}
 e->reset(0xbfc00904);const uint32_t ixin=0xbf470000;for(unsigned n=0;n<4;n++)e->bios[0x904+n]=uint8_t(ixin>>(8*n));e->step();ck(e->pc==0xbfc00908&&e->cache_operations[7]==1&&e->cache_last_address==0&&e->first_fault.empty(),"reported IXIN advances to next BIOS instruction");
 // Exact final seven attempted instructions supplied by the user, starting after the RAM-clear routine returned.
 struct Entry{uint32_t pc,word;bool delay;};const Entry tail[]={
  {0xbfc008d4,0x3c1d7000,false},{0xbfc008d8,0x37bd3ff0,false},{0xbfc008dc,0x0ff0023f,false},{0xbfc008e0,0,true},
  {0xbfc008fc,0x241a0000,false},{0xbfc00900,0x0000040f,false},{0xbfc00904,0xbf470000,false}};
 e->reset(tail[0].pc);for(const auto& t:tail)for(unsigned n=0;n<4;n++)e->bios[(t.pc&0x3fffff)+n]=uint8_t(t.word>>(8*n));
 for(const auto& t:tail){ck(e->pc==t.pc&&e->delay==t.delay,"user tail PC/delay sequence");e->step();}
 ck(e->pc==0xbfc00908&&e->instructions==7&&e->r[29].low==0x70003ff0&&e->r[31].low==0xffffffffbfc008e4ull&&e->cache_operations[7]==1,"seven-attempt BIOS tail replay");
 // Authored program writes new code into RAM, performs SYNC/IXIN, then executes the stored instruction.
 e->reset(0x100000);e->r[1].low=0x200;e->r[2].low=0x2403002a;
 const uint32_t program[]={0xac220000,0x0000040f,0xbc270000,0x00200008,0};
 for(unsigned n=0;n<5;n++){e->write(0x100000+n*4,program[n],4);}for(unsigned n=0;n<6;n++){e->step();}
 ck(e->pc==0x204&&e->r[3].low==42&&e->read(0x200,4)==0x2403002a,"store/SYNC/IXIN/JR executes new RAM code in coherent interpreter");
 e->reset();e->write(0x100000,0x10000002,4);e->write(0x100004,0xbc270000,4);e->step();e->step();ck(e->pc==0x10000c&&e->cache_operations[7]==1,"CACHE executes in branch delay slot");
 ck(e->diagnostic().find("CACHE IXIN=1")!=std::string::npos,"cache diagnostics expose scope/counters");e->reset();for(auto count:e->cache_operations)ck(!count,"cache reset counters");
 std::cout<<"PASS: seven coherent cache-maintenance ops, privilege/unknown/MMIO guards, reported IXIN + BIOS tail, self-modifying code and delay slots\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
