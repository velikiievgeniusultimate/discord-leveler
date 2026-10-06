#include "../src/dsp.h"
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <vector>
void check(bool ok,const char* msg) { if(!ok) {fprintf(stderr,"FAIL: %s\n",msg); std::exit(1);} }
float run(Leveler &d,float rms,int blocks, Controls c={}) {
 std::vector<float> x(960); Meter m;
 for(int b=0;b<blocks;b++) { for(int i=0;i<480;i++) x[2*i]=x[2*i+1]=rms*1.41421356f*std::sin((b*480+i)*0.131f); m=d.process(x.data(),480,c); for(float v:x) check(std::isfinite(v)&&std::abs(v)<=0.891252f,"finite and below ceiling"); }
 return m.output;
}
int main() {
 Leveler a,b; auto quiet=run(a,0.035f,400); auto loud=run(b,0.35f,400);
 check(std::abs(quiet-loud)<1.5,"quiet and loud speech converge");
 check(std::abs(quiet+20)<1.5,"target reached");
 Leveler d; std::vector<float>x(960,5); d.process(x.data(),480,{}); for(float v:x) check(std::abs(v)<=0.891252f,"sudden loud peak limited");
 x.assign(960,0); for(int i=0;i<200;i++) d.process(x.data(),480,{}); for(float v:x)check(v==0,"silence remains silent");
 Leveler e; auto tiny=run(e,0.0001f,400); check(tiny<-75,"noise gate does not amplify noise");
 x.assign(960,0.1); x[0]=std::numeric_limits<float>::quiet_NaN(); e.process(x.data(),480,{}); check(std::isfinite(x[0]),"NaN sanitized");
 puts("DSP: convergence, target, ceiling, silence, noise, NaN passed");
}
