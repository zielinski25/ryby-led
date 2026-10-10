# 08 — Design aplikacji Ryby LED (Android i wersja HTML)

Status: wdrożone w kodzie (`android/app/src/main/java/com/rybyled/panel/ui/Theme.kt`, `App.kt`).
Do potwierdzenia przez właściciela po zrzucie ekranu z telefonu (patrz „Otwarte decyzje”).

## 1. Cel

Aplikacja ma wyglądać profesjonalnie i spójnie z resztą produktów: **Sterownik Pieca C.O.** (Centrala PRO).
Piec jest referencją wizualną, bo to ten sam autor, ta sama rodzina sterowników i ten sam sposób zdalnego
sterowania przez Firebase. Poniżej: co konkretnie robi Piec, co mówią zewnętrzne zasady, co było źle w pierwszej
wersji Ryby i co wdrażam.

## 2. Analiza Pieca (źródło: repo `Sterownik-Pieca-C.O.`, gałąź `arena/539ab1b6-sterownik-pieca-c-o`)

| Obszar | Co robi Piec | Pliki |
|---|---|---|
| Paleta | Tła granatowo-czarne (`#060D18`, `#0C1829`), nie czysta czerń. Akcent cyjan `#00D4F5`, pomarańcz `#FF9F43` (ciepło), zieleń `#4ADE80` (OK), żółty `#FBBF24`, czerwień `#FF5F78` (błąd). | `ui/theme/Color.kt` |
| Obramowania | Półprzezroczyste (`rgba(80,109,132,.22)`), a nie twarde kreski. | `Color.kt` |
| Pasek górny | Gradient `#071421 → #0A1928`, marka wielkimi literami, chip stanu `● LIVE` / `○ BRAK SESJI`. | `ui/components/Chrome.kt` (`TopBar`, `Chip`) |
| Hero | Duża temperatura (ExtraBold), gradient `#0F2438 → #07131F`, etykieta cyjanem. | `Hmi.kt`, `Chrome.kt` |
| Kafelki | Gradient, zaokrąglenie `Dimens.radiusTile`, **obramowanie w kolorze stanu** (czerwony/żółty/zielony/szary), poblask w kolorze akcentu, pasek postępu. | `Hmi.kt` (`Tile`) |
| Nawigacja | 5 pozycji, aktywna pozycja w cyjanie, tło i obramowanie w przezroczystości. | `Chrome.kt` (`NavBar`) |
| Typografia | ExtraBold dla nagłówków i wartości, etykiety sekcji WIELKIMI literami z odstępem między znakami. | `ui/theme` (`Txt`) |
| Potwierdzenia | Wybrane akcje wymagają potwierdzenia (sprawdzone dla Ryby: restart, zapis domyślnych). | ekrany Pieca |

Wymiary i promienie są zapisane w `Dimens` w Piecu. Przy przenoszeniu do Ryby przyjęto te same promienie (18 dp kafelki, 16 dp panele).

## 3. Zasady zewnętrzne (research)

**Ciemny motyw**
- Nie używać czystej czerni (`#000000`) jako tła: wysoki kontrast męczy oczy. Lepsze są ciemnoszare z lekkim odcieniem
  niebieskim, np. `#060D18` ([Toptal](https://www.toptal.com/designers/ui/dark-ui-design), [UX Planet](https://uxplanet.org/8-tips-for-dark-theme-design-8dfc2f8f7ab6)).
- Głębię w ciemnym motywie buduje się **jaśniejszymi powierzchniami** wyżej w hierarchii, a nie cieniami
  ([Toptal](https://www.toptal.com/designers/ui/dark-ui-design)). Piec robi to gradientami kafelków i hero.
- Nasycone kolory na ciemnym tle „wibrują”; lepiej używać jaśniejszych odcieni ([UX Planet](https://uxplanet.org/8-tips-for-dark-theme-design-8dfc2f8f7ab6)).

**Kontrast i dostępność**
- WCAG 2.2 AA: tekst zwykły **4,5:1**, duży tekst (od 18 pt, lub 14 pt pogrubiony) **3:1**, elementy UI i ikony
  informacyjne **3:1** ([Web Accessibility Checker](https://web-accessibility-checker.com/en/blog/color-contrast-checker-wcag-guide)).
- Stan nie może być przekazany wyłącznie kolorem; błąd potrzebuje też tekstu lub ikony
  ([TestParty](https://testparty.ai/blog/color-contrast-requirements)). Stąd chipy typu „● LIVE” z tekstem.

**Dotyk i układ**
- Cel dotykowy minimum 48 dp (Material); odstęp między elementami ok. 8 px ([Edesignify](https://edesignify.com/blogs/tap-targets-and-touch-zones-mobile-ux-that-works)).
- W aplikacjach smart home najpierw pokazuje się **stan**, a dopiero potem sterowanie. Wyraźna informacja zwrotna po
  każdej akcji buduje zaufanie ([DesignMonks](https://www.designmonks.co/blog/home-automation-app-ui-design)).
- Akcje ryzykowne (restart, wyłączenia) wymagają potwierdzenia ([ESP32.co.uk](https://esp32.co.uk/iphone-apple-watch-the-ultimate-home-assistant-control-panel/)).

## 4. Audyt pierwszej wersji Ryby (co było źle)

- Płaskie karty bez hierarchii: każdy blok wyglądał tak samo, brak „bohatera” ekranu.
- Brak marki i stanu połączenia w pasku górnym (stan był w środku ekranu).
- Kafelki bez stanu w obramowaniu: wartość alarmowa wyglądała tak samo jak zwykła.
- Brak wyraźnego „bohatera” ekranu (temperatura nie była wyróżniona).
- Etykiety pomocnicze i wiele elementów o tej samej wadze wizualnej.
- Ikona aplikacji była prowizoryczna (zastąpiona ikoną adaptacyjną).

## 5. Decyzje (wdrożone)

1. **Ten sam system wizualny co Piec**: nazwy tokenów kolorów (`Pal`), wymiarów (`Dimens`) i stylów tekstu (`Txt`) jak w Piecu, w `ui/Theme.kt`.
2. **Pasek górny**: marka „RYBY LED”, tytuł ekranu, chip stanu (`● LIVE` / `● OPÓŹNIONE` / `○ BRAK DANYCH`).
3. **Hero** na ekranie głównym: temperatura płytki 1, pigułki zasilania i trybu. Przy opóźnionych danych obramowanie
   zmienia się na żółte.
4. **Kafelki z kolorem stanu w obramowaniu** (neutralny gdy brak zdarzenia). Wartości w 22 sp, etykiety w 13 sp.
5. **Nawigacja**: aktywna pozycja cyjanowa z pigułką (jak Piec), ikony Material zamiast tekstowych znaczników.
6. **Przyciski**: akcja główna cyjanowa z ciemnym tekstem, drugorzędne z obramowaniem, ryzykowne (restart) czerwone.
7. **Potwierdzenia**: restart i zapis domyślnych suwaków wymagają potwierdzenia.
8. **Wykres energii**: słupki cyjanowe, dzisiejszy dzień w pełnym kolorze.

### Świadome odejścia od Pieca

- **Większe teksty**: etykiety Pieca mają ok. 8,8–10 sp. Na telefonie w Ryby etykiety mają 12–13 sp, treść 15 sp.
- **Kontrast etykiet pomocniczych**: Piec używa `#5A7A99` (kontrast 3,97:1 na `#0C1829`, poniżej 4,5:1). Ryby używa
  `#7D98AD` (5,91:1). Wygląd pozostaje ten sam.
- **Ikony Material** zamiast własnych SVG Pieca. Ikony Pieca są ręcznie rysowane, co wymagałoby osobnego projektu (decyzja do potwierdzenia).

## 6. Kontrast (zmierzony, WCAG)

| Tekst | Tło | Kontrast | Wynik |
|---|---|---|---|
| Text `#E6F0F7` | Surface `#0C1829` | 15,42:1 | AA |
| TextDim `#8EA6BA` | Surface | 7,06:1 | AA |
| TextDim2 `#7D98AD` | Surface | 5,91:1 | AA |
| Cyan `#00D4F5` | Bg `#060D18` | 10,88:1 | AA |
| Live `#4ADE80` | Surface | 10,23:1 | AA |
| Warn `#FBBF24` | Surface | 10,68:1 | AA |
| Err `#FF5F78` | Surface | 6,08:1 | AA |
| OnCyan `#060D18` | Cyan | 10,88:1 | AA |
| (Piec) TextDim2 `#5A7A99` | Surface | 3,97:1 | **poniżej AA** |

Wszystkie 16 par z kodu są sprawdzane automatycznie w `firmware/tests/host/check_android_app.js` (sekcja 6c).

## 7. Mapa ekranów

- **Przegląd**: hero (temperatura), kafelki temperatur, lux, moc, przełączniki zasilania i pompy, tryb AUTO/RĘCZNY.
- **Oświetlenie**: 5 suwaków PWM (0–1023), „Zastosuj”, „Wczytaj z ESP”, LED 100 % / wyłączony, zapis domyślnych z potwierdzeniem.
- **Pompa**: przełącznik pompy i przedziały pracy (tylko odczyt).
- **Energia**: hero z zużyciem dziś, kafelki tygodnia i miesiąca, wykres słupkowy, czas świecenia, taryfa.
- **Ustawienia**: adres bazy, Database Secret i CMD_TOKEN (pola z gwiazdkami), test połączenia, czyszczenie sekretów z telefonu, restart ESP.

## 8. Otwarte decyzje (do potwierdzenia)

1. Czy akcent **cyjan** z Pieca jest OK także dla aplikacji Ryby (obecnie tak).
2. Czy **większe teksty** niż w Piecu są akceptowalne (obecnie tak, dla czytelności).
3. Czy **ikony Material** wystarczą, czy wolisz ręcznie rysowane ikony jak w Piecu (wymaga osobnego projektu).
4. Czy ekran Przegląd ma pokazywać jeszcze temperaturę wody / powietrza z innych czujników (zależy od firmware).

## 9. Ograniczenia i weryfikacja

- Zmiany **nie zostały jeszcze obejrzane na telefonie** (sandbox nie ma Androida). Kontrola wizualna należy do właściciela.
- Kontrast i spójność palety są sprawdzane automatycznie; układ i odstępy wymagają zrzutu ekranu.
- Kompilację sprawdza Android Studio (`build_apk.ps1` / Run). Ostatni znany błąd (brakujący import) został już naprawiony.

## 10. Źródła

- Piec, paleta i komponenty (`Hmi.kt`, `Chrome.kt`, `Color.kt`): https://github.com/zielinski25/Sterownik-Pieca-C.O. (gałąź `arena/539ab1b6-sterownik-pieca-c-o`, `android/app/src/main/java/com/sterownikco/pro/ui/theme/Color.kt`, `ui/components/Chrome.kt`, `ui/components/Hmi.kt`)
- Toptal, dark UI: https://www.toptal.com/designers/ui/dark-ui-design
- UX Planet, dark theme: https://uxplanet.org/8-tips-for-dark-theme-design-8dfc2f8f7ab6
- WCAG 2.2 kontrast: https://web-accessibility-checker.com/en/blog/color-contrast-checker-wcag-guide
- TestParty, kontrast i stany: https://testparty.ai/blog/color-contrast-requirements
- Edesignify, cele dotykowe: https://edesignify.com/blogs/tap-targets-and-touch-zones-mobile-ux-that-works
- DesignMonks, UI smart home: https://www.designmonks.co/blog/home-automation-app-ui-design
- ESP32.co.uk, potwierdzenia akcji: https://esp32.co.uk/iphone-apple-watch-the-ultimate-home-assistant-control-panel/
