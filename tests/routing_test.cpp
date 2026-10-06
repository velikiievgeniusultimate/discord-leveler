#include "../src/routing.h"
#include <cstdio>
#include <cstdlib>
void check(bool ok,const char*msg){if(!ok){fprintf(stderr,"FAIL: %s\n",msg);std::exit(1);}}
int main(){
 QJsonObject p{{"application.process.id","3139"},{"application.process.binary","Discord"},{"application.name","WEBRTC VoiceEngine"}};
 check(acceptsDiscord(p,"/opt/discord/Discord"),"native Discord voice accepted");
 check(!acceptsDiscord(p,"/usr/lib/chromium/chromium"),"Discord metadata on browser rejected");
 check(!acceptsDiscord(p,""),"missing process rejected");
 p["application.process.binary"]="chromium";
 check(!acceptsDiscord(p,"/usr/lib/chromium/chromium"),"generic WEBRTC browser rejected");
 p["application.process.binary"]="firefox";check(!acceptsDiscord(p,"/usr/lib/firefox/firefox"),"Firefox excluded");
 p["application.process.binary"]="GenshinImpact.exe";check(!acceptsDiscord(p,"/usr/bin/wine"),"game excluded");
 p["application.process.binary"]="Discord";p["application.process.id"]="2";check(!acceptsDiscord(p,""),"Flatpak namespace ambiguity rejected");
 p["application.process.id"]="nonsense";check(!acceptsDiscord(p,"/opt/discord/Discord"),"invalid PID rejected");
 p["application.process.id"]="3139";p["application.process.binary"]="DiscordCanary";check(acceptsDiscord(p,"/opt/discord-canary/DiscordCanary"),"Canary accepted");
 puts("Routing: native Discord accepted; browser, games, missing/invalid processes excluded");
}
