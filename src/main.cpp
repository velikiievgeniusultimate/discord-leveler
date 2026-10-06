#include "app.h"
#include <QApplication>
#include <QCoreApplication>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>
#include <cstdio>
int audioWorker(const QStringList &args);
int main(int argc,char **argv){
 if(argc>1&&QString(argv[1])=="--audio"){QCoreApplication app(argc,argv);return audioWorker(app.arguments());}
 if(argc>1&&QString(argv[1])=="--cleanup"){QCoreApplication app(argc,argv);return cleanup();}
 QApplication app(argc,argv);app.setApplicationName("discord-leveler");app.setApplicationVersion("0.1.0");app.setQuitOnLastWindowClosed(false);
 const QString verb=app.arguments().value(1,"--settings").mid(2);
 QLocalSocket socket;socket.connectToServer("discord-leveler");
 if(socket.waitForConnected(250)){socket.write(verb.toUtf8());socket.waitForBytesWritten(1000);return 0;}
 QLockFile lock(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation)+"/discord-leveler.lock");lock.setStaleLockTime(0);
 if(!lock.tryLock()){fprintf(stderr,"Discord Leveler already running\n");return 1;}
 QLocalServer::removeServer("discord-leveler");
 App controller;
 if(verb=="settings")controller.settings();else if(verb=="off")controller.setEnabled(false);else if(verb=="on")controller.setEnabled(true);
 return app.exec();
}
