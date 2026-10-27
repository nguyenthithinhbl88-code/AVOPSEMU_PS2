#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool b,const char* why){if(!b)throw std::runtime_error(why);}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();
 auto one=[&](uint32_t op){e->write(e->pc,op,4);e->step();};
 e->reset(0x100000);
 for(unsigned sel:{0u,2u,3u,4u,5u,6u,7u}){
  e->r[3]={0x80000000u+sel,0x11223344};e->r[4].high=0x55667788;
  one(0x4083c000|sel);one(0x4004c000|sel);
  ck(e->r[4].low==uint64_t(int64_t(int32_t(0x80000000u+sel)))&&e->r[4].high==0x55667788,"debug selector and sign extension preserve upper GPR");
 }
 ck(e->debug_transfers==14,"seven real debug registers");
 e->r[3].low=0xffffffff;one(0x4083c800);ck(e->pccr==0x800ffbfe,"PCCR writable bits");
 e->r[3].low=123;one(0x4083c801);e->r[3].low=456;one(0x4083c803);one(0x4004c801);ck(e->r[4].low==123,"PCR0 independent");one(0x4004c803);ck(e->r[4].low==456,"PCR1 independent");
 // Enable only kernel events: model cycles and completed instructions.
 e->reset(0x100000);e->cop0[12]=0;e->pccr=0x80000000u|4|(1<<5)|(4<<10)|(12<<15);
 one(0);one(0);ck(e->pcr[0]==2&&e->pcr[1]==2,"functional counters increment");
 e->cop0[12]=0x10;one(0);ck(e->pcr[0]==2&&e->pcr[1]==2,"mode gates");
 e->cop0[12]=4;one(0);ck(e->pcr[0]==2,"ERL inhibits counting");
 e->cop0[12]=0;e->pccr=0;one(0);ck(e->pcr[0]==2,"CTE stop");
 e->reset(0x100000);e->r[3].low=0x1234;bool failed=false;
 try{one(0x4083c001);}catch(const std::runtime_error&){failed=true;}ck(failed&&e->debug_registers[0]==0&&e->pc==0x100000,"reserved debug selector rejected atomically");
 e->reset(0x100000);failed=false;try{one(0x4083c802);}catch(const std::runtime_error&){failed=true;}ck(failed&&!e->pccr,"invalid perf selector rejected");
 // General exception routing + ERET, including branch delay and nested EXL.
 e->reset(0x100000);e->cop0[12]=0;one(0x0000000c);
 ck(e->pc==0x80000180&&e->cop0[14]==0x100000&&((e->cop0[13]>>2)&31)==8&&(e->cop0[12]&2)&&e->first_fault.empty(),"SYSCALL enters guest handler");
 e->cop0[14]+=4;one(0x42000018);ck(e->pc==0x100004&&!(e->cop0[12]&2),"ERET resumes guest");
 e->reset(0x100000);e->cop0[12]=0;one(0x10000003);one(0x0000000d);
 ck(e->pc==0x80000180&&e->cop0[14]==0x100000&&(e->cop0[13]&0x80000000)&&((e->cop0[13]>>2)&31)==9,"BREAK delay slot EPC/BD");
 one(0x0000000c);ck(e->cop0[14]==0x100000&&(e->cop0[13]&0x80000000),"nested exception retains EPC/BD");
 e->reset(0x100000);one(0x0000000d);ck(e->pc==0xbfc00380,"BEV bootstrap vector");
 // Signed overflow must not write even the destination's low lane.
 for(bool wide:{false,true})for(bool subtract:{false,true}){
  e->reset(0x100000);e->cop0[12]=0;e->r[1].low=wide?0x7fffffffffffffffULL:0x7fffffff;e->r[2].low=subtract?~0ULL:1;e->r[3]={0x7788,0xaabb};
  one((1u<<21)|(2u<<16)|(3u<<11)|(wide?(subtract?0x2e:0x2c):(subtract?0x22:0x20)));
  ck(e->r[3].low==0x7788&&e->r[3].high==0xaabb&&((e->cop0[13]>>2)&31)==12&&e->pc==0x80000180,"ADD/SUB signed overflow atomic");
 }
 e->reset(0x100000);e->cop0[12]=0;e->r[1].low=~0ULL;e->r[2].low=4;one((1u<<21)|(2u<<16)|(3u<<11)|0x20);ck(e->r[3].low==3,"ADD nonoverflow");
 e->reset(0x100000);e->cop0[12]=0;e->r[1].low=0x7fffffff;one(0x20230001);ck(((e->cop0[13]>>2)&31)==12,"ADDI overflow");
 e->reset(0x100000);e->cop0[12]=0;e->r[1].low=0x7fffffffffffffffULL;one(0x60230001);ck(((e->cop0[13]>>2)&31)==12,"DADDI overflow");
 // Every register/immediate trap family, both true and false conditions.
 for(unsigned fn:{0x30u,0x31u,0x32u,0x33u,0x34u,0x36u})for(bool take:{false,true}){
  e->reset(0x100000);e->cop0[12]=0;const bool less=fn==0x32||fn==0x33;const bool equal=fn==0x34;
  e->r[1].low=less?(take?1:3):equal?2:3;e->r[2].low=equal?(take?2:1):fn==0x36?(take?2:3):less?2:(take?2:4);
  one((1u<<21)|(2u<<16)|fn);ck((e->sync_exceptions!=0)==take,"register trap condition");
 }
 for(unsigned rt:{8u,9u,10u,11u,12u,14u})for(bool take:{false,true}){
  e->reset(0x100000);e->cop0[12]=0;e->r[1].low=rt==10||rt==11?(take?1:3):rt==12?(take?2:3):rt==14?(take?3:2):(take?3:1);
  one(0x04000000|(1u<<21)|(rt<<16)|2);ck((e->sync_exceptions!=0)==take,"immediate trap condition");
 }
 ck(e->diagnostic().find("PCCR=")!=std::string::npos,"extended diagnostic present");
 e->reset();ck(!e->pccr&&!e->pcr[0]&&!e->debug_registers[0]&&!e->sync_exceptions,"reset clears extension state");
 std::cout<<"PASS: seven COP0 debug latches, three performance registers/selectors, mode gates, guest SYSCALL/BREAK/TRAP/overflow and ERET\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
