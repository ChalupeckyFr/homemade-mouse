# Arduino kód

V repozitáři zatím není hlavní firmware myši. Existuje pouze oddělený měřicí Arduino sketch BMI160: [experiment, zapojení, nahrání a CSV analýza](../experiments/bmi160-motion/README.md). Není to firmware myši ani důkaz fyzického testu.

## Zamýšlené části

- Inicializace BLE HID a bezpečný stav po připojení/odpojení.
- Čtení mechanických tlačítek s pull-upem, debounce a mapováním LMB/RMB/Back/Forward.
- Čtení BMI160 přes I²C, ověření chip ID, kalibrace biasu a relativní Air pohyb.
- Vzorkování ADC baterie z děliče na bateriovém uzlu a přepočet po ověření kalibrace.
- Později obsluha OLED, režimů, motoru a případného optického senzoru.

Při návrhu oddělit neblokující čtení senzorů/HID od obnovy displeje. Při chybě senzoru nebo BLE nesmí zůstat HID tlačítko zaseknuté. Konkrétní knihovny, vzorkovací frekvence, citlivost, filtry a pinout zatím nejsou zvoleny.

Měřicí experiment v `experiments/bmi160-motion/` má vlastní malý register driver a integraci pouze pro fixní orientaci. Nepoužívej jej jako hotové Air/Normal ovládání, HID vrstvu ani důkaz přesnosti.

Přehled elektrických pinů je pouze v [centrální tabulce](../design/circuits/CIRCUITS.md). Po implementaci zaznamenat prostředí, verzi knihoven a výsledky podle [testovacího plánu](../guide/TESTING.md). Celkový postup je v [roadmapě](../guide/ROADMAP.md).
