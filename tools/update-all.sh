#!/bin/bash
# Holt alle vier Offbeat-Plugins auf den neuesten Stand, baut sie und installiert sie (macOS).
#
#   tools/update-all.sh             Repos neben diesem Repo (z. B. ~/Developer/offbeat-*)
#   tools/update-all.sh ~/Musik     Repos in einem anderen Ordner
#
# Bricht ab, wenn LMMS, Carla oder FL Studio laufen (sonst bleiben alte Plugins geladen bzw. die
# Dateien sind gesperrt). Fehlende Repos werden geklont. Kein Auto-Update im Plugin selbst.
set -u
HERE="$(cd "$(dirname "$0")/.." && pwd)"
BASE="${1:-$(dirname "$HERE")}"
REPOS="offbeat-engine offbeat-topline offbeat-turbo offbeat-hydraulics"

running=""
for app in lmms LMMS Carla carla "FL Studio" OsxFL; do
    pgrep -x "$app" >/dev/null 2>&1 && running="$running $app"
done
pgrep -f "FL Studio.app" >/dev/null 2>&1 && running="$running FL-Studio"
if [ -n "$running" ]; then
    echo "Bitte zuerst schließen:$running"
    echo "(Laufende DAWs halten die alten Plugins geladen; danach das Skript erneut starten.)"
    exit 1
fi

failed=""
for r in $REPOS; do
    dir="$BASE/$r"
    echo "== $r"
    if [ ! -d "$dir/.git" ]; then
        git clone --recurse-submodules "https://github.com/lukestark-glitch/$r" "$dir" || { failed="$failed $r"; continue; }
    fi
    (
        cd "$dir" &&
        git pull --ff-only &&
        git submodule update --init --recursive &&
        make -j2 &&
        make install
    ) > "$dir/update.log" 2>&1 || { echo "   FEHLER, siehe $dir/update.log"; failed="$failed $r"; continue; }
    echo "   installiert: $(git -C "$dir" describe --tags --always --dirty)"
done

echo
echo "Installierte Versionen:"
for r in $REPOS; do
    [ -d "$BASE/$r/.git" ] && printf "  %-20s %s\n" "$r" "$(git -C "$BASE/$r" describe --tags --always --dirty)"
done
if [ -n "$failed" ]; then
    echo "Nicht aktualisiert:$failed"
    exit 1
fi
echo "Fertig. DAW neu starten bzw. Plugins neu scannen."
