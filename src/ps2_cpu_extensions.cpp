#include "ps2_ee.hpp"
#include <sstream>
#include <stdexcept>
namespace ps2 {
bool Ee::cop0_special_transfer(uint32_t instruction){
 const unsigned direction=(instruction>>21)&31,reg=(instruction>>11)&31;
 if((reg!=24&&reg!=25)||(direction!=0&&direction!=4))return false;
 const unsigned selector=instruction&0x7ff,rt=(instruction>>16)&31;
 uint32_t* value=nullptr;
 if(reg==24){
  if(selector==0||(selector>=2&&selector<=7))value=&debug_registers[selector];
 }else{
  if(selector==0)value=&pccr;
  else if(selector==1||selector==3)value=&pcr[selector>>1];
 }
 if(!value)throw std::runtime_error("EE debug/performance selector unsupported");
 if(direction==4){*value=uint32_t(r[rt].low);if(reg==25&&selector==0)*value&=0x800ffbfeu;}
 else if(rt)r[rt].low=uint64_t(int64_t(int32_t(*value)));
 if(reg==24)debug_transfers++;else perf_transfers++;
 return true;
}
void Ee::performance_tick(uint32_t instruction,uint32_t old_status,bool completed){
 if(!(pccr&0x80000000u)||(old_status&4))return;
 // One functional CPU tick per instruction attempt; not EE dual-issue timing.
 for(unsigned i=0;i<2;i++){
  const unsigned shift=i*10;
  const unsigned mode=(old_status&2)?1:((old_status&0x18)==0?2:(old_status&0x18)==8?3:4);
  if(!(pccr&(1u<<(shift+mode))))continue;
  const unsigned event=(pccr>>(shift+5))&31;
  const bool increment=event==1||(event==12&&completed);
  if(increment)pcr[i]++;
 }
 (void)instruction;
}
void Ee::synchronous_exception(unsigned code){
 const bool nested=(cop0[12]&2)!=0;
 if(!nested){cop0[14]=delay?pc-4:pc;cop0[13]=(cop0[13]&~0x80000000u)|(delay?0x80000000u:0);}
 cop0[13]=(cop0[13]&~0x3000007cu)|((code&31)<<2);
 cop0[12]|=2;
 pc=(cop0[12]&(1u<<22))?0xbfc00380u:0x80000180u;next=pc+4;delay=false;
 r[0]={};instructions++;cop0[9]++;sync_exceptions++;
}
std::string Ee::cpu_extension_diagnostic()const{
 std::ostringstream o;
 o<<"EE synchronous exceptions="<<std::dec<<sync_exceptions<<" debug transfers="<<debug_transfers<<" performance transfers="<<perf_transfers<<"\r\n";
 const char* names[]={"BPC","reserved","IAB","IABM","DAB","DABM","DVB","DVBM"};
 for(unsigned i:{0u,2u,3u,4u,5u,6u,7u})o<<names[i]<<"="<<std::hex<<debug_registers[i]<<"\r\n";
 o<<"PCCR="<<std::hex<<pccr<<" PCR0="<<pcr[0]<<" PCR1="<<pcr[1]<<"\r\n";
 o<<"Debug registers: transfer latches only; no breakpoint matching/TRIG/debug exception. Performance: events 1 and 12, functional ticks only; no overflow exception or pipeline/cache events.\r\n";
 return o.str();
}
}
