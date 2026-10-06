#pragma once
#include <QJsonObject>
#include <QFileInfo>
inline bool isDiscordExecutable(const QString &name) {
 const auto n=name.toLower();return n=="discord"||n=="discordcanary"||n=="discordptb";
}
inline bool acceptsDiscord(const QJsonObject &properties,const QString &liveExecutable) {
 bool valid=false;int pid=properties.value("application.process.id").toString().toInt(&valid);
 return valid&&pid>1&&isDiscordExecutable(properties.value("application.process.binary").toString())&&isDiscordExecutable(QFileInfo(liveExecutable).fileName());
}
