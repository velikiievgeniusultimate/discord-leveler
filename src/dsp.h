#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
struct Controls {
    float target=-20, maxBoost=12, gate=-48, attack=30, release=700, ceiling=-1;
};
struct Meter { float input=-90, output=-90, gain=0; };
class Leveler {
    float envelope=0, gain=1, limitGain=1;
    static float amp(float db) { return std::pow(10.f,db/20.f); }
    static float db(float a) { return 20.f*std::log10(std::max(a,0.0000316f)); }
public:
    Meter process(float *data, size_t frames, const Controls &c, float rate=48000) {
        float sum=0;
        for(size_t i=0;i<frames*2;++i) {
            if(!std::isfinite(data[i])) data[i]=0;
            sum+=data[i]*data[i];
        }
        float rms=std::sqrt(sum/std::max(size_t(1),frames*2));
        const float dt=frames/rate;
        float envCoef=std::exp(-dt/(rms>envelope?0.025f:0.09f));
        envelope=envCoef*envelope+(1-envCoef)*rms;
        // Silence never drives gain upward. Noise below the gate slowly returns to unity.
        float desired=rms>amp(c.gate)?std::clamp(amp(c.target)/std::max(envelope,0.000001f),amp(-30),amp(c.maxBoost)):1.f;
        float tau=(desired<gain?c.attack:c.release)/1000.f;
        float coef=std::exp(-1.f/(rate*std::max(tau,0.001f)));
        float peak=0;
        for(size_t f=0;f<frames;++f) {
            gain=coef*gain+(1-coef)*desired;
            for(int ch=0;ch<2;++ch) { data[f*2+ch]*=gain; peak=std::max(peak,std::abs(data[f*2+ch])); }
        }
        // One 10 ms block of lookahead: the same gain acts on both stereo channels.
        float safe=peak>amp(c.ceiling)?amp(c.ceiling)/peak:1.f;
        limitGain=safe<limitGain?safe:std::min(safe,limitGain+(1-limitGain)*(1-std::exp(-dt/0.15f)));
        float outSum=0;
        for(size_t i=0;i<frames*2;++i) { data[i]=std::clamp(data[i]*limitGain,-amp(c.ceiling),amp(c.ceiling)); outSum+=data[i]*data[i]; }
        return {db(rms),db(std::sqrt(outSum/std::max(size_t(1),frames*2))),db(gain*limitGain)};
    }
};
