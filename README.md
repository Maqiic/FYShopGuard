# FYShopGuard: separater TikTok-Shop-/Ads-Filter (EXPERIMENTELL)

**Ziel:** Shop-Videos, die ASJTok `Remove ads` nicht als Werbung erkennt, aus dem FY-Feed herausfiltern. Keine Garantie: ungetestet auf realem iPhone. **Es liegt keine fertig nutzbare `.dylib` bei.**

## Wie der Filter vorgeht
- Sucht in `AWEAwemeModel` und dessen ursprünglichen JSON-Daten nach *eindeutigen* Produkt-/Shop-Signalen (`shop_anchor`, `product_info`, `product_list`, `shop_info`, `is_ecom` usw.).
- Prüft Anchor-Klassen für `AWEAwemeAnchorGoodsSeedingModel` sowie Produktanker, ohne pauschal alle Anchors zu löschen.
- Versucht erkannte Modelle vor dem Anzeigen aus Aweme-Listen zu entfernen. Bei bereits sichtbaren Treffern versucht er, zum nächsten Video zu springen.
- Filtert klassische Anzeigen weiter nach Ad-Merkmalen.

## Was verifiziert ist, was nicht
- **Verifiziert:** Der Quellcode kann als ARM64-Mach-O-Objekt kompiliert werden (`clang -target arm64-apple-ios14.0`). In der früheren vom Nutzer hochgeladenen *VibeTok/TikTok 47.1.0* finden sich einige der verwendeten iOS-Klassen und Selektoren.
- **Nicht verifiziert:** Der GitHub-Actions-Build und das Linking zur iOS-`.dylib` wurden hier nicht ausgeführt; der Filter wurde nicht auf TikTok oder ASJTok ausgeführt.
- **Besonders wichtig:** ASJTok 1.3 basiert auf TikTok 47.0.0. Ob alle Methoden dort vorhanden sind und der Tweak konfliktfrei mit ASJTok arbeitet, wurde **nicht** bestätigt. Shop-Anzeigen können andere serverseitige Strukturen verwenden.

## Bauen ohne Mac (GitHub Actions)
1. ZIP entpacken und die enthaltenen Dateien mit **korrekter Verzeichnisstruktur** in ein **privates** GitHub-Repository hochladen.
2. Auf `github.com` **Actions** öffnen, den Workflow `Build FYShopGuard for ESign` wählen, dann **Run workflow**.
3. Wenn der Build erfolgreich durchläuft, aus **Artifacts** `FYShopGuard-arm64-dylib` herunterladen.
4. ZIP der GitHub-Aktion entpacken: **FYShopGuard.dylib** ist die injizierbare Bibliothek. Die `.o`-Datei wäre dagegen **nicht** injizierbar.

## Bauen auf Mac (Theos)
Voraussetzung: Xcode samt iPhoneOS SDK und installierter Theos-Toolchain.

```sh
export THEOS="$HOME/theos"
make clean
make
find .theos -name FYShopGuard.dylib
```

## Installation (ESign, kein Jailbreak)
1. **Backup** deiner funktionierenden ASJTok-IPA anlegen.
2. In ESign die `.dylib` **in eine Kopie** der ASJTok-IPA injizieren. Bei ASJTok 1.3 ist ein weiterer Tweak bereits eingebaut; es kann Hook-Konflikte geben.
3. Mit deinem eigenen Developer-Zertifikat neu signieren und installieren; prüfen, ob Start und Feed funktionieren.
4. Bei Absturz oder Fehlverhalten die unveränderte ASJTok-IPA wieder verwenden.

**Kein Jailbreak / keine `.deb` erforderlich.** Nicht den ganzen TikTok-Shop bzw. Zahlungsverkehr abfangen oder Accountdaten verändern: Es ist ein rein lokaler Anzeige-/Feed-Filter. Keine Zertifikate oder IPAs für den GitHub-Build hochladen.

## Einschränkungen
Es können Shop-Videos übersehen oder normale Videos mit Produktlinks entfernt werden. Der Filter hängt von internen, nicht dokumentierten TikTok-APIs ab. Für zuverlässig funktionierende Adblocker ist ein Test und ggf. Nachbesserung anhand der tatsächlich angezeigten Shop-Videos nötig.
