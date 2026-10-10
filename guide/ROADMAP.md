# Roadmapa

Pořadí níže popisuje plán, nikoli již provedené nebo otestované etapy. Výsledek každého kroku doplnit až po jeho dokončení.

| Etapa | Výstup | Závislosti / rozhodnutí |
|---|---|---|
| 1. Výběr funkčních bloků | Závazný seznam požadavků pro tlačítka, Air/Normal režim, napájení, displej a ovládání | Potvrdit, co má Normal režim na podložce skutečně umět |
| 2. Schéma napájení | Úplné schéma baterie, nabíječky, power-path, vypínače, boostu a zátěže | Vybrat konkrétní článek a moduly, dimenzovat proud a ochrany, řešit USB zpětné napájení |
| 3. BMI160 a I²C | Schéma senzoru, napájecí blokování, pull-upy a pinout | Ověřit holý IC versus modul, GPIO10 odvozené ze screenshotu a adresu 0x68 |
| 4. Mechanická tlačítka | Symboly, zvolené díly, footprinty, pad mapping a elektrické názvy | Ověřit model kontaktu/Omron a stavy boot pinů |
| 5. Plán OLED, režimů, scrollu a menu | Rozhraní a pin budget, návrh obrazovek a ovládání | Zjistit přesný OLED a jeho napájení/adresu; rozhodnout enkodér vs dotykové ovládání |
| 6. Elektrická revize | Kompletní schéma a zaznamenaný ERC, opravené nebo zdůvodněné výstrahy | Prověřit všechna napájení, GPIO, ADC, footprinty, konektory a nedostupné piny |
| 7. Breadboard a testy | Samostatné prototypové zapojení, naměřené výsledky podle [testovacího plánu](TESTING.md) | Nezačínat bateriovým napájením, dokud není jeho bezpečnostní revize hotová |
| 8. Firmware | Samostatně otestované vstupy, IMU, BLE HID, monitoring a UI | Definovat platformu/API; žádný firmware nyní není v repozitáři |
| 9. PCB a kryt | Deska a mechanický návrh po ověření obvodů | Vyřešit Normal tracking, pin budget, footprinty, umístění a ergonomii |
| 10. Integrace | Sestavené zařízení a zdokumentované integrační testy | Teprve po dokončení a ověření předchozích kroků |

## Otevřené rozhodnutí – před PCB

- Jaký je přesný OLED model, řadič, rozhraní (I²C/SPI), napájecí varianta a adresa?
- Jaké konkrétní jsou deska ESP32-C3, baterie, nabíjecí modul, boost měnič a vibrační motor?
- Jak je řešen bezpečný power-path a USB/boost zpětné napájení?
- Bude Normal režim obsahovat optický snímač? Jaké jsou lift-off a tracking požadavky?
- Lze bezpečně ponechat GPIO8/GPIO9 pro hlavní tlačítka při resetu a na konkrétní desce?
- Jaký je konečný rozpočet pinů pro pět tlačítek, I²C, ADC, motor, LED, režim, enkodér a případné SPI?
- Bude scroll ovládán enkodérem, dvěma dotykovými oblastmi nebo víceelektrodovým sliderem? Je ovladač menu společný?
- Bude potřeba další MCU nebo GPIO expander; pokud ano, jaké rozhraní a napájení?

## Jak zaznamenávat postup

U každé dokončené etapy přidat datum skutečné práce, odkaz na revizi schématu/zdrojový soubor, rozhodnutí a případně výsledek testu včetně podmínek měření. Oddělit „navrženo“, „sestaveno“ a „otestováno“. Obrázky exportovat ze skutečného návrhu a uložit do tematické podsložky, například `design/circuits/exports/`; screenshoty z konverzace nejsou součástí repozitáře.
