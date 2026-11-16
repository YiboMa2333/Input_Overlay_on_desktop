// SPDX-License-Identifier: GPL-2.0-only
#include "desktop/geometry.hpp"
#include <iostream>
#include <cstdlib>
using namespace overlay::placement;
namespace {
void check(bool condition, const char* message) {
    if(!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
bool near(double a, double b) { return std::abs(a-b) < 0.001; }
}
int main() {
    const Rect start{100,100,338,148}; const Limits limits{169,74,2028,888};
    const auto centers = handles(start.w,start.h,4);
    const std::array<Handle,8> ids{Handle::NW,Handle::N,Handle::NE,Handle::E,Handle::SE,Handle::S,Handle::SW,Handle::W};
    for(unsigned i=0;i<ids.size();++i) check(hit(centers[i],338,148,1)==ids[i],"all eight handles hit");
    check(hit({150,80},338,148,1)==Handle::Move,"interior drag target");
    check(hit({-10,80},338,148,1)==Handle::None,"outside not a target");
    check(hit({8,8},676,296,2)==Handle::NW,"DPI scaled handle");
    auto result = drag(start,Handle::Move,1000,-200,true,false,limits);
    check(result.x==1100 && result.y==-100 && result.w==338 && result.h==148,"move across negative monitor coordinates");
    for(auto handle : {Handle::NW,Handle::NE,Handle::SE,Handle::SW}) {
        result=drag(start,handle,80,50,true,false,limits);
        check(near(result.w/result.h,aspect),"corner proportional original ratio");
        check(near(result.x+(handle==Handle::NW||handle==Handle::SW ? result.w : 0),
            handle==Handle::NW||handle==Handle::SW ? start.right() : start.x),"opposite horizontal anchor");
        check(near(result.y+(handle==Handle::NW||handle==Handle::NE ? result.h : 0),
            handle==Handle::NW||handle==Handle::NE ? start.bottom() : start.y),"opposite vertical anchor");
    }
    result=drag(start,Handle::E,100,20,false,false,limits);
    check(result.w==438 && result.h==148 && result.x==100 && result.y==100,"unlocked east is single axis");
    result=drag(start,Handle::N,50,-40,false,false,limits);
    check(result.w==338 && result.h==188 && result.bottom()==start.bottom(),"unlocked north anchored single axis");
    result=drag(start,Handle::E,100,0,false,true,limits);
    check(near(result.w/result.h,aspect) && near(result.y+result.h/2,174),"Shift forces original ratio with centered edge");
    result=drag({100,100,400,100},Handle::SE,100,0,false,false,limits);
    check(near(result.w/result.h,4),"default corner preserves current ratio when aspect lock disabled");
    result=drag(start,Handle::NW,100000,100000,true,false,limits);
    check(result.w>=169 && result.h>=74 && near(result.right(),438),"no flip past anchor or undersize");
    result=drag(start,Handle::SE,100000,100000,true,false,limits);
    check(result.w<=2028 && result.h<=888 && near(result.w/result.h,aspect),"bounded huge corner resize");
    const std::vector<Rect> dual{{-1920,0,1920,1080},{0,0,1920,1080}};
    check(accessible({-200,30,338,148},dual),"negative monitor position valid");
    result=recover({1850,30,338,148},dual);
    check(result.x==1850,"partially offscreen placement preserved");
    result=recover({5000,5000,338,148},dual);
    check(accessible(result,dual),"offscreen placement recovered");
    result=recover({-1800,100,338,148},{{0,0,1920,1080}});
    check(result.x==0 && accessible(result,{{0,0,1920,1080}}),"removed left monitor recovered");
    result=recover({-100,0,2028,888},{{0,0,800,600}},true);
    check(result.w<=800 && result.h<=600 && near(result.w/result.h,aspect),"resolution shrink fits proportional window");
    result=recover({100,100,338,148},{});
    check(result.x==100,"no screens does not crash");
    std::cout << "Placement geometry tests passed\n";
}
