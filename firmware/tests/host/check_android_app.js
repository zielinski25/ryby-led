// check_android_app.js — kontrakt aplikacji Android (android/) z firmware i z panel/ryby-mobile.html.
// Bez Gradle/JDK: sprawdza źródła Kotlin statycznie (nie kompiluje). Build robisz w Android Studio.
// Uruchomienie: node firmware/tests/host/check_android_app.js   (4.7.2+build.272)
"use strict";
const fs = require("fs");
const path = require("path");

const ROOT = path.resolve(__dirname, "..", "..", "..");
const AND = path.join(ROOT, "android");
const SRC = path.join(AND, "app", "src", "main");
const JAVA = path.join(SRC, "java", "com", "rybyled", "panel");
const read = (p) => fs.readFileSync(p, "utf8");

const FW = read(path.join(ROOT, "firmware", "src", "Ryby_LED_fi_S3.cpp"));
const HTML = read(path.join(ROOT, "panel", "ryby-mobile.html"));
const LOGIC = read(path.join(JAVA, "core", "Logic.kt"));
const STATUS = read(path.join(JAVA, "core", "RybyStatus.kt"));
const RTDB = read(path.join(JAVA, "core", "Rtdb.kt"));
const VM = read(path.join(JAVA, "ui", "AppViewModel.kt"));
const UI = read(path.join(JAVA, "ui", "App.kt"));
const PREFS = read(path.join(JAVA, "core", "AppPrefs.kt"));
const GRADLE_APP = read(path.join(AND, "app", "build.gradle.kts"));
const MANIFEST = read(path.join(SRC, "AndroidManifest.xml"));

let pass = 0, fail = 0;
function check(cond, label) {
  if (cond) { pass++; console.log("PASS: " + label); }
  else { fail++; console.log("FAIL: " + label); }
}
const eqList = (a, b) => a.length === b.length && a.every((x, i) => x === b[i]);

// ───── 1) komendy i progi zgodne z firmware i z HTML ─────
const kotlinPlain = (() => {
  const m = LOGIC.match(/val PLAIN_CMDS = listOf\(([\s\S]*?)\)/);
  return m ? [...m[1].matchAll(/"([a-z_0-9]+)"/g)].map((x) => x[1]) : [];
})();
const htmlPlain = (() => {
  const m = HTML.match(/var PLAIN_CMDS = \[([\s\S]*?)\];/);
  return m ? [...m[1].matchAll(/"([a-z_0-9]+)"/g)].map((x) => x[1]) : [];
})();
check(kotlinPlain.length === 10, "Logic.kt: 10 komend bez argumentów (" + kotlinPlain.length + ")");
check(eqList(kotlinPlain, htmlPlain), "PLAIN_CMDS w Kotlin = PLAIN_CMDS w ryby-mobile.html (kolejność też)");
for (const c of kotlinPlain) {
  check(FW.includes(c), "firmware zna komendę: " + c);
}

const kotlinCmdsUsed = [...(VM + UI).matchAll(/sendPlain\("([a-z_0-9]+)"\)/g)].map((x) => x[1]);
check(kotlinCmdsUsed.length >= 8, "UI wysyła komendy przez sendPlain (" + kotlinCmdsUsed.length + " wywołań)");
for (const c of [...new Set(kotlinCmdsUsed)]) {
  check(kotlinPlain.includes(c), "sendPlain(\"" + c + "\") jest na liście PLAIN_CMDS");
}

check(/const val STALE_MS = 150_000L/.test(LOGIC), "STALE_MS = 150 s (jak w HTML i firmware)");
check(/const val PWM_MAX = 1023/.test(LOGIC), "PWM_MAX = 1023");
check(/const val PWM_CHANNELS = 5/.test(LOGIC), "5 kanałów PWM");
check(/fun pwmCommand\(values: List<Double\?>\): String = "pwm " \+ pwmArgs\(values\)/.test(LOGIC), "komenda pwm: \"pwm a b c d e\"");
check(/fun autosaveCommand\(values: List<Double\?>\): String = "autosave " \+ pwmArgs\(values\)/.test(LOGIC), "komenda autosave: \"autosave a b c d e\"");
check(/fun nextTs\(nowMs: Long, lastTs: Long\): Long = max\(nowMs, lastTs \+ 1\)/.test(LOGIC), "ts rośnie (max(now, last+1))");
check(/fun connectionState/.test(LOGIC) && /STALE_MS/.test(LOGIC), "stan połączenia: online / opóźnione / brak");

// ───── 2) adres bazy i endpointy ─────
const dbUrl = (LOGIC.match(/const val DEFAULT_DB_URL = "([^"]+)"/) || [])[1];
const htmlDb = (HTML.match(/var DEFAULT_DB_URL = "([^"]+)"/) || [])[1];
check(!!dbUrl && dbUrl === htmlDb, "DEFAULT_DB_URL taki sam jak w ryby-mobile.html");
check(/\/aquarium\/status\.json\?auth=/.test(RTDB), "odczyt: /aquarium/status.json?auth=");
check(/\/aquarium\/commands\.json\?auth=/.test(RTDB), "zapis: /aquarium/commands.json?auth=");
check(/put\("cmd", cmd\)\.put\("token", token\)\.put\("ts", ts\)/.test(RTDB), "body komendy: {cmd, token, ts}");
check(/HttpError\(code\)/.test(RTDB) && /zły Database Secret/.test(VM), "HTTP 401 pokazuje czytelny błąd");

// ───── 3) pola statusu czytane przez aplikację istnieją w firmware ─────
const readKeys = [...STATUS.matchAll(/(?:\.num|\.opt|\.optJSONArray|\.optString|\.optJSONObject|\?:\s*d\.num)\(\s*"([A-Za-z_0-9]+)"/g)]
  .map((x) => x[1]);
const uniqKeys = [...new Set(readKeys)];
check(uniqKeys.length >= 30, "parser czyta " + uniqKeys.length + " pól statusu");
// Pola, które firmware zapisuje pod innym kluczem lub wyprowadza — świadomie pomijane.
const notInFirmware = ["lux", "wifi_rssi"];
for (const k of uniqKeys) {
  if (notInFirmware.includes(k)) continue;
  check(FW.includes(k), "firmware publikuje pole statusu: " + k);
}

// ───── 4) bezpieczeństwo ─────
check(/android:usesCleartextTraffic="false"/.test(MANIFEST), "manifest: cleartext zablokowany (tylko HTTPS)");
check(!/usesCleartextTraffic="true"/.test(MANIFEST), "manifest: brak usesCleartextTraffic=true");
const perms = [...MANIFEST.matchAll(/uses-permission android:name="([^"]+)"/g)].map((x) => x[1]);
check(eqList(perms, ["android.permission.INTERNET"]), "tylko uprawnienie INTERNET (bez lokalizacji i kontaktów)");
check(/getSharedPreferences\("ryby_prefs", Context\.MODE_PRIVATE\)/.test(PREFS), "ustawienia w prywatnych SharedPreferences");
check(/sp\.getString\(KEY_SECRET, ""\)/.test(PREFS) && /sp\.getString\(KEY_TOKEN, ""\)/.test(PREFS), "sekrety domyślnie puste (nic w kodzie)");
check(!/android:allowBackup="true"/.test(MANIFEST), "brak kopii zapasowej danych aplikacji (allowBackup=false)");
check(!/\bLog\.[divwe]\(/.test(VM + RTDB + PREFS), "brak logowania sekretów (Log.*) w kodzie sieci i ustawień");
// Znane wyciekłe wartości NIE są tu wpisywane (publiczne repo). Pilnuje ich check_secrets.py (krok 0).
check(!/\bfor\s*\(\s*.*innerHTML/.test(UI), "UI Compose: brak HTML (dane jako Text)");

// ───── 5) wersja i build ─────
const appVer = (GRADLE_APP.match(/versionName = "([^"]+)"/) || [])[1];
const verFile = read(path.join(ROOT, "version.txt")).trim();
check(appVer === verFile, "versionName aplikacji = version.txt (" + appVer + " = " + verFile + ")");
check(/versionCode = 4272/.test(GRADLE_APP), "versionCode = 4272 (4.7.2)");
check(/applicationId = "com\.rybyled\.panel"/.test(GRADLE_APP), "applicationId com.rybyled.panel");
check(/minSdk = 26/.test(GRADLE_APP) && /targetSdk = 35/.test(GRADLE_APP), "minSdk 26, targetSdk 35");
check(/versionCode = 4272/.test(GRADLE_APP) && FW.includes("4.7.2"), "wersja aplikacji zgodna z firmware 4.7.2");
const wrapperJar = path.join(AND, "gradle", "wrapper", "gradle-wrapper.jar");
check(fs.existsSync(wrapperJar) && fs.readFileSync(wrapperJar).slice(0, 2).toString() === "PK", "gradle-wrapper.jar jest archiwum (PK)");
check(/gradle-8\.9-bin\.zip/.test(read(path.join(AND, "gradle", "wrapper", "gradle-wrapper.properties"))), "Gradle 8.9 (jak w Piecu)");

// ───── 6) pliki projektu ─────
for (const f of [
  path.join(AND, "settings.gradle.kts"), path.join(AND, "build.gradle.kts"), path.join(AND, "gradlew"),
  path.join(SRC, "java", "com", "rybyled", "panel", "MainActivity.kt"),
  path.join(JAVA, "core", "Rtdb.kt"), path.join(JAVA, "core", "AppPrefs.kt"),
  path.join(JAVA, "ui", "AppViewModel.kt"), path.join(JAVA, "ui", "App.kt"),
  path.join(SRC, "res", "values", "themes.xml"), path.join(SRC, "res", "drawable", "ic_launcher.xml"),
]) {
  check(fs.existsSync(f), "plik istnieje: " + path.relative(ROOT, f));
}
check(/MainActivity/.test(MANIFEST) && /android:name="\.MainActivity"/.test(MANIFEST), "manifest wskazuje .MainActivity");

// ───── 6b) importy: każdy użyty symbol Compose/Kotlin musi być zaimportowany (błędy z Android Studio) ─────
const IMPORT_OF = {
  rememberSaveable: "androidx.compose.runtime.saveable.rememberSaveable",
  mutableIntStateOf: "androidx.compose.runtime.mutableIntStateOf",
  mutableStateOf: "androidx.compose.runtime.mutableStateOf",
  remember: "androidx.compose.runtime.remember",
  LaunchedEffect: "androidx.compose.runtime.LaunchedEffect",
  rememberScrollState: "androidx.compose.foundation.rememberScrollState",
  verticalScroll: "androidx.compose.foundation.verticalScroll",
  PasswordVisualTransformation: "androidx.compose.ui.text.input.PasswordVisualTransformation",
  viewModel: "androidx.lifecycle.viewmodel.compose.viewModel",
  roundToInt: "kotlin.math.roundToInt",
};
const uiCode = UI.replace(/^import .*$/gm, "");
for (const [sym, imp] of Object.entries(IMPORT_OF)) {
  if (new RegExp("\\b" + sym + "\\b").test(uiCode)) {
    check(UI.includes("import " + imp + "\n"), "App.kt importuje " + sym);
  }
}
check([LOGIC, STATUS, RTDB, VM, UI, PREFS].every((t) =>
  (t.match(/\{/g) || []).length === (t.match(/\}/g) || []).length), "klamry {} zbalansowane we wszystkich plikach Kotlin");

// ───── 7) nawigacja: 5 zakładek jak w HTML ─────
const tabs = (UI.match(/TAB_LABELS = listOf\(([\s\S]*?)\)/) || [])[1] || "";
check([...tabs.matchAll(/"([^"]+)"/g)].length === 5, "5 zakładek: Główna, Światło, Pompa, Energia, Ustawienia");

console.log("\nandroid app: " + pass + " PASS, " + fail + " FAIL");
process.exit(fail === 0 ? 0 : 1);
