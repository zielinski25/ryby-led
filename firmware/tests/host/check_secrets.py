#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════════
#  check_secrets.py — [4.7.1 SECRETS-2] skan śledzonych plików repo pod kątem
#  sekretów wpisanych na stałe (Etap 2 planu upgrade). Repo jest publiczne,
#  więc NIC z poniższego nie może wrócić do źródeł, skryptów ani dokumentacji.
#  Pomijane: CHANGELOG.md (zapis historyczny — wartości były już w commitach
#  d76f552 i wcześniejszych; czyszczenie historii wymaga osobnej decyzji) oraz
#  ten skan. Dozwolone są tylko placeholdery z secrets.example.h.
#  Uruchomienie: python3 firmware/tests/host/check_secrets.py  (z korzenia repo)
# ═══════════════════════════════════════════════════════════════════════════
import pathlib, re, subprocess, sys

ROOT = pathlib.Path(subprocess.check_output(
    ["git", "rev-parse", "--show-toplevel"], text=True).strip())
SKIP = {"CHANGELOG.md", "firmware/tests/host/check_secrets.py"}

# Znane wartości, które już wyciekły (rotacja/zmiana wymagana po stronie właściciela).
KNOWN = [
    "RhKVp49q",          # Database Secret (stary)
    "Akwarium2026",      # CMD_TOKEN (stary)
    "AkwPanel2026",      # hasło OTA (stare)
    "8709162940:AA",     # token bota Telegrama (stary)
]
PATTERNS = [
    (re.compile(r"\b\d{8,10}:AA[A-Za-z0-9_-]{30,}"), "token bota Telegram"),
    (re.compile(r'define\s+(FIREBASE_SECRET|CMD_TOKEN|OTA_PASSWORD|TG_DEFAULT_BOT_TOKEN)\s+"(?!WSTAW|WKLEJ|ZMIEN|WPISZ|\s*")[^"]+"'),
     "sekret zaszyty w #define"),
    (re.compile(r'setPassword\("'), "hasło OTA w setPassword(\"...\")"),
    (re.compile(r"--auth=(?!\$\{sysenv\.)\S+"), "hasło OTA w upload_flags"),
]

def tracked():
    out = subprocess.check_output(["git", "ls-files", "-z"], cwd=ROOT)
    return [p for p in out.decode("utf-8", "replace").split("\0") if p]

fails = []
checked = 0
for rel in tracked():
    if rel in SKIP:
        continue
    path = ROOT / rel
    if not path.is_file():
        continue
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        continue  # binaria (np. grafiki) nie są źródłami sekretów
    checked += 1
    for n, line in enumerate(text.splitlines(), 1):
        for k in KNOWN:
            if k in line:
                fails.append(f"{rel}:{n}: znana wartość ({k[:4]}…)")
        commented = line.lstrip().startswith("//")  # przykłady w komentarzach = placeholdery
        for rx, what in PATTERNS:
            if commented and what.startswith("sekret zaszyty"):
                continue
            if rx.search(line):
                fails.append(f"{rel}:{n}: {what}")

print(f"secrets: {checked} plików, {len(fails)} trafień")
for f in fails:
    print("FAIL: " + f)
sys.exit(1 if fails else 0)
