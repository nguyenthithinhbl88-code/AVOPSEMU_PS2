#include "ps2_ee.hpp"
#include <sstream>
namespace ps2 {
namespace {
bool valid(uint32_t address,unsigned bytes){return bytes==2&&address>=0x1a000000&&address<0x1a000024&&!(address&1);}
}
bool Ee::ba0_read(uint32_t address,unsigned bytes,uint32_t& value)const{
 if(!valid(address,bytes))return false;
 ba0_reads++;
 if(address==0x1a000006){
  value=(ba0_error?1u:0u)|(ba0_ready?2u:0u);
  // Compatibility timing, not a measured DVE bus duration. Polling is command-driven.
  if(ba0_busy_reads){ba0_phase++;if(!--ba0_busy_reads)ba0_ready=true;}
 }else value=ba0_last_writes[(address-0x1a000000)/2];
 return true;
}
bool Ee::ba0_write(uint32_t address,unsigned bytes,uint32_t value){
 if(!valid(address,bytes))return false;
 const unsigned offset=address-0x1a000000;const auto word=uint16_t(value);
 ba0_writes++;
 if(offset==6){ba0_ready=false;ba0_phase=0;ba0_busy_reads=0;return true;}
 ba0_last_writes[offset/2]=word;
 if(offset==0x10)dve_selected=uint8_t(word);
 if(offset==0xa){ba0_error=word==0;return true;}
 if(offset!=0||!(word&0x80))return true;
 ba0_commands++;ba0_ready=false;ba0_phase=0;ba0_busy_reads=0;
 const unsigned count=word&15,mode=ba0_last_writes[1];
 // Explicit bounds: selector + at most nine halfword FIFO entries; no array wrap.
 if((mode!=0x42&&mode!=0x43)||count==0||count>10){ba0_error=true;ba0_command_errors++;return true;}
 if(mode==0x43){
  // Repeated writes/readback to a selected emulated DVE register, as a functional sink.
  for(unsigned i=1;i<count;i++)dve_registers[dve_selected]=ba0_last_writes[8+i];
 }else for(unsigned i=0;i<count;i++)ba0_last_writes[8+i]=dve_registers[dve_selected];
 ba0_error=false;ba0_busy_reads=2;
 return true;
}
std::string Ee::ba0_diagnostic()const{
 std::ostringstream o;o<<"BA0/DVE experimental bridge: reads="<<std::dec<<ba0_reads<<" writes="<<ba0_writes<<" commands="<<ba0_commands<<" errors="<<ba0_command_errors<<" phase="<<ba0_phase<<" pending_status_reads="<<ba0_busy_reads<<" ready="<<ba0_ready<<" error="<<ba0_error<<" selector="<<std::hex<<unsigned(dve_selected)<<"\r\n";
 o<<"Halfword latches 1A000000..1A000022; bounded FIFO and command-driven status. DVE register semantics, serial timing and multi-byte transfer ordering remain approximate. No GS drawing or IOP-ready reply.\r\n";
 for(unsigned n=0;n<ba0_last_writes.size();n++)o<<"BA0 reg["<<std::hex<<(0x1a000000+n*2)<<"]="<<ba0_last_writes[n]<<"\r\n";
 return o.str();
}
}
