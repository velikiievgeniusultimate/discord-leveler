#pragma once
#include <QObject>
#include <QJsonObject>
#include <QSystemTrayIcon>
#include <QProcess>
#include <QTimer>
#include <QLocalServer>
#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QProgressBar>
#include <QMap>
inline const QString sinkName="discord_leveler_private";
QString command(const QStringList &args,bool *ok=nullptr);
QString configPath();
QString statePath();
bool discordStream(const QJsonObject &stream);
int cleanup();
class App:public QObject {
 Q_OBJECT
 Q_CLASSINFO("D-Bus Interface", "io.github.DiscordLeveler")
 Q_PROPERTY(bool enabled READ enabled NOTIFY enabledChanged)
 bool on=false, switching=false, quitting=false;
 QSystemTrayIcon tray;
 QProcess worker;
 QTimer timer;
 QLocalServer server;
 QWidget *window=nullptr;
 QLabel *status=nullptr,*levels=nullptr;
 QCheckBox *check=nullptr;
 QProgressBar *inputMeter=nullptr,*outputMeter=nullptr;
 QJsonObject config,originals;
 QString errorText,outputDevice;
 QByteArray workerData;
 int module=-1;
 void saveConfig();
 void saveRoutes();
 void paintIcon();
 void buildWindow();
 void restoreRoutes();
 bool startAudio();
public:
 App();
 ~App() override;
 bool enabled()const{return on;}
public slots:
 void toggle();
 void settings();
 void setEnabled(bool value);
 void scan();
 void shutdown();
signals:
 void enabledChanged(bool value);
};
