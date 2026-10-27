#include "ps2_ee.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
void ck(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void fails(F f){bool caught=false;try{f();}catch(const std::runtime_error&){caught=true;}ck(caught,"unsupported COP2 operation must stop");}
int main(){try{
 auto e=std::make_unique<ps2::Ee>();auto one=[&](uint32_t op){e->write(e->pc,op,4);e->step();};
 e->reset(0x8000da00);e->r[29].low=0xffffffff80018e40ULL;e->r[16].low=0x7f;e->r[2]={0,0x53};
 const uint32_t program[]={0x27bdffe0,0xffbf0010,0xffb00000,0x4842e000};
 e->write(0x8000da00,0x0c0037ce,4);e->write(0x8000da04,0,4);for(unsigned i=0;i<4;i++)e->write(0x8000df38+4*i,program[i],4);
 for(unsigned i=0;i<6;i++)e->step();
 ck(e->pc==0x8000df48&&e->instructions==6&&e->first_fault.empty()&&e->r[2].low==0&&e->r[2].high==0x53,"uploaded six-instruction tail passes CFC2 FBRST at 8000DF44");
 ck(e->r[29].low==0xffffffff80018e20ULL&&e->read(0x80018e30,8)==0xffffffff8000da08ULL&&e->read(0x80018e20,8)==0x7f,"log stack and saved RA/s0 preserved");
 e->reset(0x100000);e->r[3]={0x80001234,0x55667788};one(0x48c3a800);e->r[4].high=0x12345678;one(0x4844a800);
 ck(e->r[4].low==0xffffffff80001234ULL&&e->r[4].high==0x12345678,"CFC2 sign extension retains upper 64 bits");
 e->r[3].low=0xabcdef01;one(0x48c30800);one(0x48440800);ck(e->r[4].low==0xef01,"VI1 is 16-bit");
 one(0x48c30000);one(0x48440000);ck(e->r[4].low==0,"VI0 constant zero");
 e->r[3]={0x0123456789abcdefULL,0xfedcba9876543210ULL};one(0x48a32800);one(0x48242800);
 ck(e->r[4].low==e->r[3].low&&e->r[4].high==e->r[3].high,"QMTC2/QMFC2 bit-exact 128-bit transfer");
 one(0x48a30000);one(0x48240000);ck(e->r[4].low==0&&e->r[4].high==0x3f80000000000000ULL,"VF0 constant (0,0,0,1)");
 one(0x48202800);ck(!e->r[0].low&&!e->r[0].high,"QMFC2 GPR zero protected");
 // FBRST drives only the targeted unit; no memory/VRAM/firmware patch.
 e->vu0_control[29]=0x55ff;e->vif[0].regs[0]=4;e->vif[1].regs[0]=4;e->vu_memory[2][0]=0xaa;e->vu_memory[3][0]=0xbb;
 const auto vf=e->vu0_vectors[5];e->r[3].low=0x60e;one(0x48c3e000);
 ck(e->vu0_control[28]==0x40c&&e->vu_reset_requests[0]==1&&e->vu_reset_requests[1]==1&&!e->vu0_control[29],"FBRST resets both status bytes, command bits self-clear and enable mask");
 ck(!(e->vif[0].regs[0]&4)&&!(e->vif[1].regs[0]&4)&&e->vu_memory[2][0]==0xaa&&e->vu_memory[3][0]==0xbb&&e->vu0_vectors[5].low==vf.low,"control reset preserves code/data and vector values");
 e->r[3].low=0x101;e->vu0_control[29]=0x101;one(0x48c3e000);ck(!e->vu0_control[29]&&e->vu_break_requests[0]==1&&e->vu_break_requests[1]==1,"force break consumes request");
 e->vu0_control[17]=0x123;e->vu0_control[26]=0x30;e->vu0_control[29]=0x100;one(0x48c38800);one(0x48c3d000);one(0x48c3e800);
 ck(e->vu0_control[17]==0x123&&e->vu0_control[26]==0x30&&e->vu0_control[29]==0x100,"MAC/TPC/VPU_STAT read-only");
 one(0x4844e001);ck(e->cop2_interlocks==1,"idle .I transfer accepted without fake microexecution");
 const auto moves=e->cop2_moves;auto report=e->diagnostic();ck(report.find("COP2 register transfers=")!=std::string::npos&&e->cop2_moves==moves,"diagnostic snapshot stable");
 e->reset(0x100000);e->cop0[12]&=~0x40000000u;fails([&]{one(0x4842e000);});ck(!e->cop2_moves,"CU2-disabled access rejected");
 e->reset(0x100000);e->vu0_control[29]=1;fails([&]{one(0x4842e001);});ck(e->vu0_control[29]==1&&!e->cop2_moves,"busy interlock not faked as completion");
 e->reset(0x100000);fails([&]{one(0x48c3f800);});ck(!e->cop2_moves&&!e->vu0_control[31],"CMSAR1 launch cannot be silently skipped");
 e->reset(0x100000);fails([&]{one(0x48429800);});ck(!e->cop2_moves,"reserved control index 19 strict");
 e->reset(0x100000);fails([&]{one(0x4842e002);});
 e->reset(0x100000);fails([&]{one(0x4a000000);});
 e->reset();ck(!e->cop2_moves&&!e->vu_reset_requests[0]&&!e->vu0_control[28]&&e->vu0_vectors[0].high==0x3f80000000000000ULL,"EE reset clears new register state");
 ck(!e->gs.regs[0]&&!e->gs.primitives,"COP2 init does not force GS output");
 std::cout<<"PASS: log CFC2 FBRST -> 8000DF48, COP2 control/quadword moves, constants, masks/reset, CU2, strict unsupported VU launch/interlocks\n";
 }catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
