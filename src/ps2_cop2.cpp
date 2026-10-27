#include "ps2_ee.hpp"
#include <sstream>
#include <stdexcept>
namespace ps2 {
namespace {
constexpr Register vf_zero{0,0x3f80000000000000ull}; // VF0 = (0,0,0,1), raw single-precision bits.
bool supported_control(unsigned index){
 return index<19||index==20||index==21||index==22||index==26||index==27||index==28||index==29||index==31;
}
}
void Ee::reset_cop2(){
 vu0_vectors={};vu0_vectors[0]=vf_zero;vu0_control={};
 cop2_moves=cop2_interlocks=0;vu_reset_requests=vu_break_requests={};
}
bool Ee::cop2_transfer(uint32_t instruction){
 const unsigned mode=(instruction>>21)&31,rt=(instruction>>16)&31,fs=(instruction>>11)&31;
 if(mode!=1&&mode!=2&&mode!=5&&mode!=6)return false;
 if(instruction&0x7fe)return false;
 if(!(cop0[12]&0x40000000u))throw std::runtime_error("EE COP2 unusable: Status.CU2 is clear");
 if((mode==2||mode==6)&&!supported_control(fs))throw std::runtime_error("EE COP2 reserved control register unsupported");
 if((instruction&1)&&(vu0_control[29]&1))throw std::runtime_error("EE COP2 interlock needs VU0 execution, not implemented");
 // CMSAR1 writes always launch a VU1 microprogram, including a zero entry address.
 if(mode==6&&fs==31)throw std::runtime_error("EE COP2 CMSAR1 launch requires VU1 microprogram execution, not implemented");
 if(mode==1){if(rt)r[rt]=fs?vu0_vectors[fs]:vf_zero;}
 else if(mode==5){if(fs)vu0_vectors[fs]=r[rt];}
 else if(mode==2){
  const auto value=fs==0?0u:vu0_control[fs];
  if(rt)r[rt].low=uint64_t(int64_t(int32_t(value)));
 }else{
  const auto value=uint32_t(r[rt].low);
  if(fs==28){
   vu0_control[28]=value&0x0c0c;
   for(unsigned unit=0;unit<2;unit++){
    const unsigned shift=unit*8;
    if(value&(1u<<shift)){vu_break_requests[unit]++;vu0_control[29]&=~(1u<<shift);vif[unit].regs[0]&=~4u;}
    if(value&(2u<<shift)){
     vu_reset_requests[unit]++;vu0_control[29]&=~(0xffu<<shift);vif[unit].regs[0]&=~4u;
    }
   }
  }else if(fs!=0&&fs!=17&&fs!=26&&fs!=29){
   // VI1..15 are 16-bit integers. Other writable controls are raw bootstrap latches.
   vu0_control[fs]=fs<16?value&0xffffu:value;
  }
 }
 cop2_moves++;if(instruction&1)cop2_interlocks++;
 return true;
}
std::string Ee::cop2_diagnostic()const{
 std::ostringstream o;o<<"COP2 register transfers="<<std::dec<<cop2_moves<<" idle interlocks="<<cop2_interlocks<<" FBRST="<<std::hex<<vu0_control[28]<<" VPU_STAT="<<vu0_control[29]<<"\r\n";
 for(unsigned unit=0;unit<2;unit++)o<<"VU"<<unit<<" reset requests="<<std::dec<<vu_reset_requests[unit]<<" force-break requests="<<vu_break_requests[unit]<<"\r\n";
 for(unsigned i=1;i<16;i++)o<<"VI"<<std::dec<<i<<"="<<std::hex<<vu0_control[i]<<"\r\n";
 o<<"COP2: CFC2/CTC2 and QMFC2/QMTC2 register subset. Raw STATUS/CLIP/R/I/Q/CMSAR0 latches; no flag pipeline or vector arithmetic. FBRST control reset preserves vector registers, microcode/data RAM and VIF upload state. No VU engine, CMSAR1/VCALL execution, LQC2/SQC2 or busy-interlock scheduling.\r\n";
 return o.str();
}
}
