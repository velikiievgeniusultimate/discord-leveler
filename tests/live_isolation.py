"""Optional desktop integration test: installed service and native Discord must run."""
import json, subprocess, time, os, pathlib

def pactl(*args):
    return subprocess.check_output(['pactl', *args], text=True).strip()

def streams():
    return json.loads(pactl('-f', 'json', 'list', 'sink-inputs'))

def toggle(value):
    subprocess.run(['qdbus6','io.github.DiscordLeveler','/io/github/DiscordLeveler',
                    'io.github.DiscordLeveler.setEnabled',str(value).lower()],check=True)
    time.sleep(1)

def other_routes():
    return {s['properties'].get('object.id'): (s['sink'],s['volume'],s['mute'])
            for s in streams()
            if s['properties'].get('application.process.binary','').lower() not in
            ('discord','discordcanary','discordptb','discord-leveler')}

def discord():
    return [s for s in streams() if s['properties'].get('application.process.binary')=='Discord']

toggle(False)
defaults=(pactl('get-default-sink'),pactl('get-default-source'))
others=other_routes()
original={s['properties']['object.id']:s['sink'] for s in discord()}
assert original,'Play native Discord audio before running this test'
toggle(True)
private=[s for s in json.loads(pactl('-f','json','list','sinks')) if s['name']=='discord_leveler_private']
assert len(private)==1
assert discord() and all(s['sink']==private[0]['index'] for s in discord())
assert other_routes()==others,'A non-Discord route/volume/mute changed'
assert defaults==(pactl('get-default-sink'),pactl('get-default-source'))
subprocess.run(['systemctl','--user','restart','discord-leveler.service'],check=True)
time.sleep(1.5)
runtime=pathlib.Path(os.environ.get('XDG_RUNTIME_DIR',f'/run/user/{os.getuid()}'))
saved=json.loads((runtime/'discord-leveler-routes.json').read_text())['routes']
assert saved,'Restart must preserve explicit recovery routes'
assert defaults==(pactl('get-default-sink'),pactl('get-default-source'))
toggle(False)
assert not any(s['name']=='discord_leveler_private' for s in json.loads(pactl('-f','json','list','sinks')))
assert {s['properties']['object.id']:s['sink'] for s in discord()}==original,'Discord original output was not restored'
assert other_routes()==others
assert defaults==(pactl('get-default-sink'),pactl('get-default-source'))
toggle(True)
print('PASS: Discord-only routing; other app routes/volume/mute untouched; defaults unchanged; service restart recovers routes; Off restores output and removes sink')
