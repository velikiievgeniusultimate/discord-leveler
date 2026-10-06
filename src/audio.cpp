#include "dsp.h"
#include <pulse/simple.h>
#include <pulse/error.h>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <cstdio>
int audioWorker(const QStringList &args) {
    if(args.size()!=5) return 2;
    pa_sample_spec spec{PA_SAMPLE_FLOAT32LE,48000,2};
    pa_buffer_attr attr{uint32_t(-1),480*8*3,0,480*8,480*8};
    int error=0;
    auto in=pa_simple_new(nullptr,"Discord Leveler",PA_STREAM_RECORD,args[2].toUtf8().constData(),"Discord-only capture",&spec,nullptr,&attr,&error);
    if(!in) { fprintf(stderr,"capture: %s\n",pa_strerror(error)); return 1; }
    auto out=pa_simple_new(nullptr,"Discord Leveler",PA_STREAM_PLAYBACK,args[3].toUtf8().constData(),"Leveled Discord",&spec,nullptr,&attr,&error);
    if(!out) { fprintf(stderr,"playback: %s\n",pa_strerror(error)); pa_simple_free(in); return 1; }
    Leveler dsp; Controls controls; float data[960]; int counter=0;
    const QString conf=args[4];
    while(true) {
        if(counter++%20==0) {
            QFile f(conf);
            if(f.open(QIODevice::ReadOnly)) {
                auto j=QJsonDocument::fromJson(f.readAll()).object();
                controls.target=std::clamp(float(j.value("target").toDouble(-20)),-30.f,-12.f);
                controls.maxBoost=std::clamp(float(j.value("maxBoost").toDouble(12)),0.f,24.f);
                controls.gate=std::clamp(float(j.value("gate").toDouble(-48)),-65.f,-30.f);
                controls.attack=std::clamp(float(j.value("attack").toDouble(30)),5.f,200.f);
                controls.release=std::clamp(float(j.value("release").toDouble(700)),100.f,2000.f);
                controls.ceiling=std::clamp(float(j.value("ceiling").toDouble(-1)),-6.f,-0.5f);
            }
        }
        if(pa_simple_read(in,data,sizeof(data),&error)<0) break;
        Meter m=dsp.process(data,480,controls);
        if(pa_simple_write(out,data,sizeof(data),&error)<0) break;
        if(counter%10==0) { printf("%.2f %.2f %.2f\n",m.input,m.output,m.gain); fflush(stdout); }
    }
    fprintf(stderr,"audio: %s\n",pa_strerror(error));
    pa_simple_free(out); pa_simple_free(in); return 1;
}
