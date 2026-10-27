#pragma once
#include <algorithm>
namespace ps2ui {
struct Rect {int x=0,y=0,width=0,height=0;};
struct Layout {Rect side,video,debug,status;int button_width=0;};
inline Layout layout(int width,int height,bool debugger){
 width=std::max(960,width);height=std::max(640,height);
 const int content=width-284,dh=height-204,dw=debugger?std::min(400,content/2):0;
 return {{20,144,224,dh},{264,144,content-(debugger?dw+16:0),dh},{width-20-dw,144,dw,dh},{20,height-46,width-40,30},std::min(164,(width-100)/6)};
}
}
