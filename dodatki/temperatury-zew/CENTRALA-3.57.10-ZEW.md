# Centrala v3.57.10 — kandydat Zew 2 / Zew 3

Zastępuje wcześniejszy kandydat `v3.57.9-zew`. Podstawą pozostaje źródło **v3.57.9**, nie v3.58.x: ta wersja nie zawiera późniejszych zmian z linii 3.58.x. Zmiany funkcjonalne i ograniczenia diagnostyki, historii Firebase oraz bezpieczeństwa opisano w [CENTRALA-3.57.9-ZEW.md](CENTRALA-3.57.9-ZEW.md). Ten plik ma w źródle `FIRMWARE_VERSION = "v3.57.10"` oraz wpis do changelogu; numer umożliwia liczbowe porównanie z działającym 3.57.9. Numer 3.57.10 wybrano, aby nie używać oznaczeń istniejącej osobnej linii 3.58.x.

**To nadal kandydat:** brak pełnej kompilacji ESP32 i testów sprzętu, flash, Firebase i OTA. Testy fragmentów kodu na hoście przeszły, ale nie dowodzą bezpieczeństwa sterownika. Nie wgrywać na pracujący kocioł. Najpierw należy porównać różniący się plik użytkownika z bazą, zbudować właściwy projekt, przetestować poza instalacją i zapewnić powrót do poprzedniego firmware.

Patch `centrala-3.57.10-zew-wykresy.patch` i skrypt `buduj-centrala-3.57.10-zew.ps1` wymagają identycznego znormalizowanego pliku bazowego 3.57.9. W przypadku błędu zgodności nie obchodzić kontroli; podmiana całego źródła może usunąć lokalne poprawki i ustawienia. Skrypt nie wgrywa firmware.

## OTA

W kodzie `porownajWersje()` porównuje trzy liczbowe segmenty; 3.57.10 jest wyższe od 3.57.9. Sam numer nie publikuje aktualizacji ani jej nie instaluje. `/update` pobiera `firmware.bin` z najnowszego release GitHub projektu centrali, używając jego tagu; tag i wkompilowana wersja muszą się zgadzać. Nie utworzono ani nie opublikowano release, a gotowego bin nie ma. Inne mechanizmy OTA i ich zgodność nie zostały sprawdzone. Publikacja i test OTA wymagają osobnej weryfikacji na stanowisku testowym.
