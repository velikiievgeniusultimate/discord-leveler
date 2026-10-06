#include "app.h"
#include "routing.h"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QPainter>
#include <QMenu>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QLocalSocket>
#include <QDBusConnection>
#include <QGroupBox>
#include <QFont>
#include <QSet>
QString command(const QStringList &args,bool *ok) {
 QProcess p; p.start("pactl",args);
 bool success=p.waitForFinished(2000)&&p.exitCode()==0&&p.exitStatus()==QProcess::NormalExit;
 if(ok)*ok=success;
 return QString::fromUtf8(success?p.readAllStandardOutput():p.readAllStandardError()).trimmed();
}
QString configPath(){return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)+"/discord-leveler/settings.json";}
QString statePath(){return QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)+"/discord-leveler-routes.json";}
static QJsonObject readObject(const QString &path) {QFile f(path);return f.open(QIODevice::ReadOnly)?QJsonDocument::fromJson(f.readAll()).object():QJsonObject{};}
static void writeObject(const QString &path,const QJsonObject &j) {QDir().mkpath(QFileInfo(path).absolutePath());QSaveFile f(path);if(f.open(QIODevice::WriteOnly)){f.write(QJsonDocument(j).toJson());f.commit();}}
static QJsonArray list(const QString &type){return QJsonDocument::fromJson(command({"-f","json","list",type}).toUtf8()).array();}
static QString identity(const QJsonObject &s){ auto p=s.value("properties").toObject();return p.value("object.id").toString()+":"+s.value("client").toVariant().toString()+":"+p.value("application.process.id").toString(); }
bool discordStream(const QJsonObject &s) {
 const auto p=s.value("properties").toObject();
 const QString pid=p.value("application.process.id").toString();
 const QString actual=QFileInfo("/proc/"+pid+"/exe").symLinkTarget();
 return acceptsDiscord(p,actual);
}
static QString sinkById(int id){for(auto v:list("sinks")){auto s=v.toObject();if(s.value("index").toInt()==id)return s.value("name").toString();}return {};}
int cleanup() {
 auto state=readObject(statePath()); auto originals=state.value("routes").toObject();
 auto sinks=list("sinks"); int sinkId=-1;int module=-1;
 for(auto v:sinks){auto s=v.toObject();if(s.value("name").toString()==sinkName){sinkId=s.value("index").toInt();module=s.value("owner_module").toVariant().toInt();}}
 if(sinkId>=0) {
  QString fallback=command({"get-default-sink"}); if(fallback==sinkName)fallback.clear();
  for(auto v:list("sink-inputs")){
   auto s=v.toObject();if(s.value("sink").toInt()!=sinkId)continue;
   QString id=QString::number(s.value("index").toInt());auto old=originals.value(identity(s)).toObject();
   QString target=old.value("identity").toString()==identity(s)?old.value("sink").toString():fallback;
   if(target.isEmpty()||target==sinkName) target=fallback;
   if(!target.isEmpty()){bool ok;command({"move-sink-input",id,target},&ok);if(!ok&&!fallback.isEmpty())command({"move-sink-input",id,fallback});}
  }
  if(module>=0)command({"unload-module",QString::number(module)});
 }
 QFile::remove(statePath()); return 0;
}
App::App() {
 config=readObject(configPath());
 cleanup();
 buildWindow();
 auto menu=new QMenu(window);
 auto toggleAction=menu->addAction("On / Off");connect(toggleAction,&QAction::triggered,this,&App::toggle);
 auto settingsAction=menu->addAction("Настройки…");connect(settingsAction,&QAction::triggered,this,&App::settings);
 menu->addSeparator();auto exit=menu->addAction("Выход");connect(exit,&QAction::triggered,this,&App::shutdown);
 tray.setContextMenu(menu);
 connect(&tray,&QSystemTrayIcon::activated,this,[this](auto reason){if(reason==QSystemTrayIcon::Trigger)toggle();});
 paintIcon();tray.show();
 server.listen("discord-leveler");
 connect(&server,&QLocalServer::newConnection,this,[this]{while(server.hasPendingConnections()){
  auto socket=server.nextPendingConnection();connect(socket,&QLocalSocket::readyRead,this,[this,socket]{auto msg=socket->readAll().trimmed();if(msg=="toggle")toggle();else if(msg=="on")setEnabled(true);else if(msg=="off")setEnabled(false);else if(msg=="quit")shutdown();else settings();socket->disconnectFromServer();});connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
 }});
 auto bus=QDBusConnection::sessionBus();bus.registerService("io.github.DiscordLeveler");bus.registerObject("/io/github/DiscordLeveler",this,QDBusConnection::ExportAllSlots|QDBusConnection::ExportAllProperties|QDBusConnection::ExportAllSignals);
 connect(&timer,&QTimer::timeout,this,&App::scan);timer.start(500);
 connect(&worker,&QProcess::readyReadStandardOutput,this,[this]{
  workerData+=worker.readAllStandardOutput();int pos;
  while((pos=workerData.indexOf('\n'))>=0){auto line=workerData.left(pos);workerData.remove(0,pos+1);auto values=line.split(' ');if(values.size()==3){float in=values[0].toFloat(),out=values[1].toFloat(),gain=values[2].toFloat();inputMeter->setValue(int(in+60));outputMeter->setValue(int(out+60));levels->setText(QString("Вход: %1 dBFS  ·  Выход: %2 dBFS  ·  Усиление: %3 dB").arg(in,0,'f',1).arg(out,0,'f',1).arg(gain,0,'f',1));}}
 });
 connect(&worker,qOverload<int,QProcess::ExitStatus>(&QProcess::finished),this,[this](int,QProcess::ExitStatus){if(on&&!switching){errorText="Обработка остановилась: "+QString::fromUtf8(worker.readAllStandardError()).trimmed();setEnabled(false);tray.showMessage("Discord Leveler",errorText,QSystemTrayIcon::Warning);}});
 connect(qApp,&QCoreApplication::aboutToQuit,this,[this]{quitting=true;setEnabled(false);});
 if(config.value("enabled").toBool(true))setEnabled(true);
}
App::~App(){setEnabled(false);delete window;}
void App::saveConfig(){writeObject(configPath(),config);}
void App::saveRoutes(){writeObject(statePath(),{{"routes",originals}});}
void App::paintIcon(){
 QPixmap pix(64,64);pix.fill(Qt::transparent);QPainter p(&pix);p.setRenderHint(QPainter::Antialiasing);
 QColor col=on?QColor("#60df9e"):QColor("#9aa4b2");p.setPen(QPen(col,3));p.setBrush(on?QColor("#24483c"):QColor("#333a46"));p.drawEllipse(QRectF(13,3,38,38));
 p.setPen(QPen(col,4,Qt::SolidLine,Qt::RoundCap));p.drawLine(32,11,32,22);p.drawArc(QRectF(22,13,20,20),-40*16,-280*16);
 p.setPen(col);p.setFont(QFont("sans-serif",12,QFont::DemiBold));p.drawText(QRect(0,43,64,20),Qt::AlignCenter,on?"On":"Off");tray.setIcon(QIcon(pix));tray.setToolTip(QString("Discord Leveler · %1\nНажатие: переключить · ПКМ: настройки").arg(on?"On":"Off"));
 if(check){QSignalBlocker b(check);check->setChecked(on);} emit enabledChanged(on);
}
void App::buildWindow(){
 window=new QWidget;window->setWindowTitle("Discord Leveler — Настройки");window->resize(550,600);
 window->setStyleSheet("QWidget {background:#171a24;color:#e7eaf3;font-size:14px;} QGroupBox {border:1px solid #343949;border-radius:10px;margin-top:18px;padding:16px;} QGroupBox::title {subcontrol-origin:margin;left:16px;} QDoubleSpinBox {background:#252a39;border:1px solid #454c62;border-radius:6px;padding:6px;min-width:110px;} QPushButton {background:#343c58;border:0;border-radius:8px;padding:10px;} QProgressBar {border:0;background:#292e3e;border-radius:4px;height:9px;} QProgressBar::chunk{background:#60df9e;border-radius:4px;}");
 auto root=new QVBoxLayout(window);root->setContentsMargins(24,24,24,24);root->setSpacing(16);
 auto title=new QLabel("Discord Leveler");title->setStyleSheet("font-size:26px;font-weight:700;");root->addWidget(title);
 auto subtitle=new QLabel("Ровнее голоса. Спокойнее разговор.");subtitle->setStyleSheet("color:#a8b1c7;");root->addWidget(subtitle);
 check=new QCheckBox("Выравнивать звук Discord");root->addWidget(check);connect(check,&QCheckBox::toggled,this,&App::setEnabled);
 status=new QLabel;status->setWordWrap(true);root->addWidget(status);
 auto group=new QGroupBox("Обработка речи");auto form=new QFormLayout(group);
 struct Field{const char*key;const char*label;double min,max,def;const char*unit;};
 const Field fields[]={{"target","Целевая громкость",-30,-12,-20," dBFS"},{"maxBoost","Максимальное усиление",0,24,12," dB"},{"gate","Не усиливать ниже",-65,-30,-48," dBFS"},{"attack","Реакция на громкий голос",5,200,30," мс"},{"release","Подъём тихого голоса",100,2000,700," мс"},{"ceiling","Предел пиков",-6,-0.5,-1," dBFS"}};
 for(auto f:fields){auto spin=new QDoubleSpinBox;spin->setRange(f.min,f.max);spin->setDecimals(f.max<=0?1:0);spin->setSuffix(QString::fromUtf8(f.unit));spin->setValue(config.value(f.key).toDouble(f.def));form->addRow(QString::fromUtf8(f.label),spin);connect(spin,qOverload<double>(&QDoubleSpinBox::valueChanged),this,[this,key=QString(f.key)](double v){config[key]=v;saveConfig();});spin->setObjectName(f.key);}
 root->addWidget(group);
 auto meters=new QGroupBox("Уровень звука");auto mv=new QVBoxLayout(meters);mv->addWidget(new QLabel("До обработки"));inputMeter=new QProgressBar;inputMeter->setRange(0,60);inputMeter->setTextVisible(false);mv->addWidget(inputMeter);mv->addWidget(new QLabel("После обработки"));outputMeter=new QProgressBar;outputMeter->setRange(0,60);outputMeter->setTextVisible(false);mv->addWidget(outputMeter);levels=new QLabel("Ожидание речи…");levels->setStyleSheet("font-size:11px;color:#a8b1c7;");mv->addWidget(levels);root->addWidget(meters);
 auto note=new QLabel("Только настольный Discord. Браузер, игры и микрофон не обрабатываются. Одновременные голоса Discord поступают общей смесью.");note->setWordWrap(true);note->setStyleSheet("font-size:12px;color:#a8b1c7;");root->addWidget(note);
 auto reset=new QPushButton("Вернуть мягкие настройки");root->addWidget(reset);connect(reset,&QPushButton::clicked,this,[this]{QMap<QString,double> defaults{{"target",-20},{"maxBoost",12},{"gate",-48},{"attack",30},{"release",700},{"ceiling",-1}};for(auto it=defaults.begin();it!=defaults.end();++it)window->findChild<QDoubleSpinBox*>(it.key())->setValue(it.value());});
}
void App::settings(){window->show();window->raise();window->activateWindow();}
void App::toggle(){setEnabled(!on);}
bool App::startAudio(){
 bool ok;outputDevice=command({"get-default-sink"},&ok);
 if(!ok||outputDevice.isEmpty()||outputDevice==sinkName){errorText="Нет доступного устройства вывода";return false;}
 auto id=command({"load-module","module-null-sink","sink_name="+sinkName,"rate=48000","channels=2","channel_map=front-left,front-right","sink_properties=device.description=Discord_Leveler_Private device.class=filter priority.session=0"},&ok);
 if(!ok){errorText="Не удалось создать обработку: "+id;return false;}
 module=id.toInt();saveConfig();
 worker.setProgram(QCoreApplication::applicationFilePath());worker.setArguments({"--audio",sinkName+".monitor",outputDevice,configPath()});worker.start();
 if(!worker.waitForStarted(1500)){errorText="Не удалось запустить аудиопроцесс";command({"unload-module",QString::number(module)});module=-1;return false;}
 return true;
}
void App::setEnabled(bool value){
 if(switching||value==on)return;
 switching=true;
 if(value){errorText.clear();if(startAudio())on=true;}
 else {
  on=false;restoreRoutes();worker.kill();worker.waitForFinished(1500);
  if(module>=0) command({"unload-module",QString::number(module)});
  module=-1;originals={};QFile::remove(statePath());
  levels->setText("Обработка выключена");inputMeter->setValue(0);outputMeter->setValue(0);
 }
 config["enabled"]=on;if(!quitting)saveConfig();switching=false;paintIcon();scan();
}
void App::restoreRoutes(){
 auto streams=list("sink-inputs");QString fallback=command({"get-default-sink"});
 for(auto v:streams){auto s=v.toObject();QString id=QString::number(s.value("index").toInt());auto old=originals.value(identity(s)).toObject();if(old.value("identity").toString()!=identity(s))continue;bool ok;command({"move-sink-input",id,old.value("sink").toString()},&ok);if(!ok&&!fallback.isEmpty()&&fallback!=sinkName)command({"move-sink-input",id,fallback});}
}
void App::scan(){
 if(!on){status->setText(errorText.isEmpty()?"Off · Discord играет напрямую в наушники":errorText);return;}
 if(worker.state()==QProcess::NotRunning){errorText="Аудиопроцесс недоступен";setEnabled(false);return;}
 int count=0,sinkId=-1;for(auto v:list("sinks")){auto s=v.toObject();if(s.value("name").toString()==sinkName)sinkId=s.value("index").toInt();}
 if(sinkId<0){errorText="Аудиосервер перезапустился. Нажмите On ещё раз.";setEnabled(false);return;}
 auto streams=list("sink-inputs");
 for(auto v:streams){auto s=v.toObject();if(s.value("sink").toInt()==sinkId&&!discordStream(s)){
  errorText="Посторонний поток на приватном выходе. Обработка отключена.";
  setEnabled(false);return;
 }}
 QSet<QString> live;for(auto v:streams)live.insert(identity(v.toObject()));
 for(auto key:originals.keys())if(!live.contains(key))originals.remove(key);
 saveRoutes();
 for(auto v:streams){auto s=v.toObject();if(!discordStream(s))continue;count++;
  QString id=QString::number(s.value("index").toInt());
  if(s.value("sink").toInt()!=sinkId){
   const QString original=sinkById(s.value("sink").toInt());if(original.isEmpty())continue;
   originals[identity(s)]=QJsonObject{{"sink",original},{"identity",identity(s)}};saveRoutes();
   bool ok;command({"move-sink-input",id,sinkName},&ok);if(!ok)errorText="Не удалось направить Discord в обработку";
  }
 }
 // Follow a changed default output for our playback only, never alter a system default.
 QString current=command({"get-default-sink"});
 if(current==sinkName){errorText="Приватный выход выбран системным устройством. Обработка отключена.";setEnabled(false);return;}
 if(!current.isEmpty()&&current!=sinkName&&current!=outputDevice){
  switching=true;restoreRoutes();worker.kill();worker.waitForFinished(1500);outputDevice=current;worker.setArguments({"--audio",sinkName+".monitor",outputDevice,configPath()});worker.start();worker.waitForStarted(1000);originals={};saveRoutes();switching=false;
 }
 status->setText(!errorText.isEmpty()?errorText:count?QString("On · Обрабатывается потоков Discord: %1").arg(count):"On · Ожидание звука Discord");
}
void App::shutdown(){quitting=true;setEnabled(false);qApp->quit();}
