#!/bin/sh
# YouTube - prueba 3: HTTPS con certificados, lanzadores de otras apps y prueba de SDL2
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"
export LD_LIBRARY_PATH=/usr/trimui/lib:/usr/lib:$LD_LIBRARY_PATH

{
  echo "== fecha =="; date
  echo "== curl con --cacert =="
  curl -s -m 15 --cacert /etc/ssl/certs/ca-certificates.crt -o /dev/null -w "HTTP %{http_code}\n" https://www.youtube.com 2>&1
  echo "== curl con CURL_CA_BUNDLE =="
  CURL_CA_BUNDLE=/etc/ssl/certs/ca-certificates.crt curl -s -m 15 -o /dev/null -w "HTTP %{http_code}\n" https://www.youtube.com 2>&1
  echo "== certificados =="; ls -l /etc/ssl/certs/ca-certificates.crt 2>&1
  echo "== variables SDL =="; env | grep -i -E "^SDL|^LD_LIBRARY|^HOME|^PATH" 2>&1
  echo "== launch.sh de otras apps =="
  for a in 2048 pico BootRetroArch; do
    echo "--- $a"; head -n 40 "/mnt/SDCARD/Apps/$a/launch.sh" 2>&1
  done
  echo "== decodificacion (test.mp4 opcional) =="
  if [ -f "$DIR/test.mp4" ]; then
    ffmpeg -hide_banner -i "$DIR/test.mp4" 2>&1 | grep -E "Stream|Duration"
    ffmpeg -hide_banner -benchmark -threads 4 -i "$DIR/test.mp4" -t 15 -an -f null - 2>&1 | tail -n 6
  else
    echo "No hay test.mp4"
  fi
} > "$DIR/yt_info3.txt" 2>&1

chmod +x "$DIR/sdltest" 2>/dev/null
"$DIR/sdltest" "$DIR" > "$DIR/sdltest_stdout.txt" 2>&1
