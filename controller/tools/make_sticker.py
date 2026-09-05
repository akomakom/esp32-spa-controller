#!/usr/bin/env python3
"""Generate a printable QR sticker for handing off the hot tub controller.

Reads the SoftAP name/password and mDNS hostname straight out of the shared
secrets.h so the sticker always matches what the firmware actually does, then
emits a self-contained HTML file (QR codes inlined as SVG) you can print.

Usage:
    python3 tools/make_sticker.py                # writes tools/sticker.html
    python3 tools/make_sticker.py -o /tmp/x.html
    python3 tools/make_sticker.py --secrets ../secrets.h

Requires: segno  (pip install segno)
"""
import argparse
import re
import sys
from pathlib import Path

import segno


def parse_define(text, name):
    """Return the value of a `#define NAME "value"` or `#define NAME NULL`.

    Returns None when the macro is absent or explicitly NULL.
    """
    m = re.search(r'^\s*#define\s+' + re.escape(name) + r'\s+(.+?)\s*$',
                  text, re.MULTILINE)
    if not m:
        return None
    val = m.group(1).strip()
    if val == "NULL":
        return None
    if val.startswith('"') and val.endswith('"'):
        return val[1:-1]
    return val


def qr_svg(data):
    """Render QR `data` to a responsive inline SVG string.

    omitsize=True makes segno emit a viewBox (and no fixed width/height), so the
    caller's CSS can scale it to any box. Without it segno hard-codes pixel width/
    height and no viewBox, which clips the code to its top-left corner when scaled.
    """
    import io
    buf = io.BytesIO()
    segno.make(data, error='m').save(
        buf, kind='svg', scale=1, border=2,
        dark='#04233f', light='#ffffff', svgclass=None, xmldecl=False,
        omitsize=True)
    return buf.getvalue().decode('utf-8')


def build_html(ap_name, ap_pass, host, home_url=None, ip=None):
    wifi_payload = "WIFI:T:{sec};S:{ssid};{pw};;".format(
        sec="WPA" if ap_pass else "nopass",
        ssid=ap_name,
        pw="P:" + ap_pass if ap_pass else "",
    )
    # Address for QR "B" (on the home network). Default is <host>.local, but .local
    # does not resolve in Android/Linux browsers — pass --home-url with something that
    # does (e.g. http://hottub via the router's DHCP DNS, or a reserved IP).
    url = home_url if home_url else "http://{}.local".format(host)
    wifi_qr = qr_svg(wifi_payload)
    url_qr = qr_svg(url)
    pass_line = ("Password: <b>{}</b>".format(ap_pass) if ap_pass
                 else "No password needed")
    # Optional fallback IP shown in plain text, in case name resolution fails.
    ip_line = ('<br>Or by address: <span class="big">http://{}</span>'.format(ip)
               if ip else "")

    return """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Hot Tub Control &mdash; Sticker</title>
<style>
  :root {{ --ink:#04233f; --accent:#2f9bff; --line:#c9d4de; }}
  * {{ box-sizing: border-box; }}
  body {{ font-family: -apple-system, Segoe UI, Roboto, Helvetica, Arial, sans-serif;
         color: var(--ink); margin: 0; padding: 24px; background: #eef2f6; }}
  .sticker {{ width: 4in; margin: 0 auto; background: #fff; border: 2px solid var(--ink);
             border-radius: 16px; overflow: hidden; }}
  .head {{ background: var(--ink); color: #fff; text-align: center; padding: 12px 10px; }}
  .head h1 {{ margin: 0; font-size: 20px; letter-spacing: .02em; }}
  .head p {{ margin: 3px 0 0; font-size: 12px; opacity: .85; }}
  .method {{ padding: 14px 16px; }}
  .method + .method {{ border-top: 1px dashed var(--line); }}
  .step {{ display: flex; gap: 12px; align-items: center; }}
  .num {{ flex: 0 0 26px; height: 26px; border-radius: 50%; background: var(--accent);
         color: #04233f; font-weight: 800; display: flex; align-items: center;
         justify-content: center; font-size: 15px; }}
  .title {{ font-weight: 800; font-size: 15px; }}
  .qrwrap {{ display: flex; gap: 14px; align-items: center; margin-top: 10px; }}
  .qr {{ flex: 0 0 auto; width: 116px; height: 116px; }}
  .qr svg {{ width: 100%; height: 100%; display: block; }}
  .detail {{ font-size: 13px; line-height: 1.4; }}
  .detail .big {{ font-size: 16px; font-weight: 800; }}
  .foot {{ text-align: center; font-size: 11px; color: #5b6b78; padding: 8px; }}
  .print {{ text-align: center; margin: 18px 0; }}
  .print button {{ font-size: 15px; padding: 8px 18px; border-radius: 8px;
                  border: 1px solid var(--ink); background: var(--accent);
                  color: var(--ink); font-weight: 700; cursor: pointer; }}
  @media print {{
    body {{ background: #fff; padding: 0; }}
    .print {{ display: none; }}
    .sticker {{ border-color: #000; }}
  }}
</style>
</head>
<body>
  <div class="print"><button onclick="window.print()">Print this sticker</button></div>
  <div class="sticker">
    <div class="head">
      <h1>&#9832; Hot Tub Control</h1>
      <p>Scan a code with your phone camera</p>
    </div>

    <div class="method">
      <div class="step">
        <div class="num">A</div>
        <div class="title">Easiest &mdash; connect directly</div>
      </div>
      <div class="qrwrap">
        <div class="qr">{wifi_qr}</div>
        <div class="detail">
          Scan to join the hot tub's own WiFi.<br>
          Network: <span class="big">{ap_name}</span><br>
          {pass_line}<br>
          The control page opens on its own.
        </div>
      </div>
    </div>

    <div class="method">
      <div class="step">
        <div class="num">B</div>
        <div class="title">On the home WiFi</div>
      </div>
      <div class="qrwrap">
        <div class="qr">{url_qr}</div>
        <div class="detail">
          Once it's on the house network, open<br>
          <span class="big">{url}</span><br>
          from any phone or computer at home.{ip_line}
        </div>
      </div>
    </div>

    <div class="foot">Keep this label on or near the hot tub.</div>
  </div>
</body>
</html>
""".format(wifi_qr=wifi_qr, url_qr=url_qr, ap_name=ap_name,
           pass_line=pass_line, url=url, ip_line=ip_line)


def main():
    here = Path(__file__).resolve().parent
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--secrets", default=str(here.parent.parent / "secrets.h"),
                    help="path to secrets.h (default: repo-root secrets.h)")
    ap.add_argument("-o", "--out", default=str(here / "sticker.html"),
                    help="output HTML path (default: tools/sticker.html)")
    ap.add_argument("--home-url",
                    help="URL for the home-network QR (e.g. http://hottub or "
                         "http://192.168.1.50). Defaults to http://<MDNS_HOSTNAME>.local, "
                         "which does NOT work on Android/Linux browsers.")
    ap.add_argument("--ip",
                    help="Fallback IP address to print (e.g. 192.168.1.50). Shown as "
                         "http://<ip> in the home-network instructions for when name "
                         "resolution fails. Reserve it in your router's DHCP first.")
    args = ap.parse_args()

    secrets = Path(args.secrets)
    if not secrets.exists():
        sys.exit("secrets.h not found at {} (try --secrets)".format(secrets))
    text = secrets.read_text()

    ap_name = parse_define(text, "SOFTAP_WIFI_NAME") or "hottubctl"
    ap_pass = parse_define(text, "SOFTAP_WIFI_PASS")
    host = parse_define(text, "MDNS_HOSTNAME") or "hottub"

    html = build_html(ap_name, ap_pass, host, args.home_url, args.ip)
    Path(args.out).write_text(html)
    home = args.home_url if args.home_url else "http://{}.local".format(host)
    print("Wrote {}".format(args.out))
    print("  SoftAP   : {} ({})".format(ap_name, "open" if not ap_pass else "password-protected"))
    print("  Home QR  : {}".format(home))
    if not args.home_url:
        print("  NOTE: .local does not resolve in Android/Linux browsers. "
              "Re-run with --home-url http://hottub (or a reserved IP) once you know "
              "what your router serves.")


if __name__ == "__main__":
    main()
