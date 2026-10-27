#include "ps2_ui_layout.hpp"
#include <cassert>
#include <iostream>
int main(){for(int w:{960,1024,1280,1440,1920,2560})for(int h:{640,720,900,1440})for(bool debug:{false,true}){
 const auto l=ps2ui::layout(w,h,debug);assert(l.video.width>0&&l.video.height>0);assert(l.side.x+l.side.width<l.video.x);assert(l.video.x+l.video.width<=w-20);assert(l.video.y+l.video.height<l.status.y);assert(20+5*(l.button_width+12)+l.button_width<=w-20);if(debug)assert(l.video.x+l.video.width<l.debug.x&&l.debug.x+l.debug.width<=w-20);
 }std::cout<<"PASS: PS2 toolbar/sidebar/GS/debug layout bounds, 48 configurations\n";}
