#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════════
#  check_tg_menu.py — [4.5.0] kontrola menu Telegrama (Etap 5, pkt 2) bez ESP.
#  Wyciąga literał JSON z sendTelegramInlineMenu() (Ryby_LED_fi_S3.cpp),
#  podstawia atrapy za %s i sprawdza:
#    - JSON jest poprawny i ma strukturę inline_keyboard,
#    - rozmiar po podstawieniu mieści się w PL_CAP (bufor PSRAM),
#    - każdy callback_data (poza "noop") ma gałąź w dispatcherze,
#    - każda akcja występuje raz (brak duplikatów),
#    - każdy raport (status/temp/lux/harmonogram/update/...) jest w JEDNYM kliknięciu,
#    - wiersze "noop" to nagłówki (jeden przycisk, bez akcji).
#  Uruchomienie: python3 firmware/tests/host/check_tg_menu.py [ścieżka do .cpp]
# ═══════════════════════════════════════════════════════════════════════════
import json
import pathlib
import re
import sys

HERE = pathlib.Path(__file__).resolve().parent
SRC = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else HERE / "../../src/Ryby_LED_fi_S3.cpp"
text = SRC.read_text(encoding="utf-8")

fails = []


def check(cond, msg):
    if not cond:
        fails.append(msg)


# 1) Wycinek z sendTelegramInlineMenu(): od snprintf(_pl, PL_CAP do argumentów.
a = text.index("snprintf(_pl, PL_CAP,")
b = text.index("tgChatId.c_str(), safeHeader.c_str()", a)
span = text[a:b]

cap_m = re.search(r"const size_t PL_CAP = (\d+);", text)
check(cap_m is not None, "brak deklaracji PL_CAP")
PL_CAP = int(cap_m.group(1)) if cap_m else 0

# 2) Literały C (linie, które nie są komentarzem) → skonkatenowany szablon.
literals = []
for line in span.splitlines():
    st = line.strip()
    if not st or st.startswith("//") or st.startswith("snprintf"):
        continue
    for m in re.finditer(r'"((?:[^"\\]|\\.)*)"', st):
        lit = m.group(1)
        lit = lit.replace('\\"', '"').replace("\\\\", "\\")
        literals.append(lit)
template = "".join(literals)

# 3) Liczba %s = liczba argumentów (chat_id, text, 3 etykiety).
placeholders = template.count("%s")
check(placeholders == 5, "oczekiwano 5 × %%s, jest %d" % placeholders)

# Atrapy za kolejne %s (chat_id, text, 3 etykiety) — JSON musi być poprawny.
filled = template
for val in ["1234", "🐟 test", "LED: WŁ ->wyłącz", "MANUAL ->AUTO", "Powiad. ->wł"]:
    filled = filled.replace("%s", val, 1)

try:
    data = json.loads(filled)
except json.JSONDecodeError as e:
    print("FAIL: JSON menu nie parsuje się:", e)
    sys.exit(1)

kb = data["reply_markup"]["inline_keyboard"]
check(isinstance(kb, list) and kb, "pusty inline_keyboard")

# 4) Rozmiar po podstawieniu < PL_CAP (z zapasem na zakończenie NUL).
size_real = len(filled.encode("utf-8"))
check(size_real + 1 < PL_CAP, "menu %d B nie mieści się w PL_CAP=%d" % (size_real, PL_CAP))

# 5) Dispatcher: znane callbacki.
dispatch = set(re.findall(r'cbData == "([a-z0-9]+)"', text))
dispatch.add("noop")

actions = []
for row in kb:
    check(1 <= len(row) <= 2, "wiersz z %d przyciskami (max 2)" % len(row))
    for btn in row:
        cb = btn.get("callback_data", "")
        check(0 < len(cb.encode("utf-8")) <= 64, "callback_data poza limitem 1..64 B: %r" % cb)
        check(cb in dispatch, "callback '%s' bez gałęzi w dispatcherze" % cb)
        if cb == "noop":
            check(len(row) == 1, "nagłówek noop nie może być w parze")
            check(btn["text"].startswith("—") and btn["text"].endswith("—"), "nagłówek bez ramki: %r" % btn["text"])
        else:
            actions.append(cb)

check(len(actions) == len(set(actions)), "zduplikowane akcje w menu: %r" % actions)

# 6) Każdy raport z planu ma przycisk (jedno kliknięcie).
for needed in ["status", "temp", "light", "schedule", "ota", "critlog", "logs", "energy", "adapt", "sensorhist"]:
    check(needed in actions, "brak przycisku dla raportu '%s'" % needed)

# 7) Brak podmenu: żadne callback_data nie wskazuje na inne menu.
check(not any(a.startswith("menu") or a.startswith("sub") for a in actions), "podmenu w menu (narusza DoD)")

# 8) Sekcje z planu (nagłówki) obecne.
headers = [btn["text"] for row in kb for btn in row if btn.get("callback_data") == "noop"]
for name in ["STATUS", "CZUJNIKI", "HARMONOGRAM", "DIAGNOSTYKA", "LOGI", "OTA"]:
    check(any(name in h for h in headers), "brak sekcji %s" % name)

if fails:
    for f in fails:
        print("FAIL:", f)
    sys.exit(1)

print("OK: menu Telegrama: %d wierszy, %d akcji, %d B (PL_CAP=%d), %d nagłówków sekcji."
      % (len(kb), len(actions), size_real, PL_CAP, len(headers)))
