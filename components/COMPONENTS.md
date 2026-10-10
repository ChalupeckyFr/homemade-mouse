# Součástky a kusovník

Seznam zachycuje zamýšlené bloky, nikoli potvrzené objednávky nebo inventář. Přesný produkt uvádím jen tehdy, když je doložen. Starší prodejní odkazy nebyly spolehlivé, proto je zde neopakuji jako platné nákupní odkazy.

## Známé záměry a otevřené volby

| Počet | Blok / součástka | Stav a co ověřit |
|---:|---|---|
| 1 | ESP32-C3 SuperMini | Základní zamýšlený mikrokontrolér. Přesná revize/deska, pinout, interní zapojení LED/BOOT, napájecí vstupy a USB/boost chování: doplnit. |
| 1 | BMI160 nebo GY-BMI160 modul | Záměr pro Air režim. Není doloženo, zda půjde o holý IC nebo konkrétní modul; ověřit napájení, integrované kondenzátory a I²C pull-upy. |
| 4 | Mechanický normálně rozpojený kontakt | Po jednom pro levé, pravé, Forward a Back. `SW_Push` v KiCadu je symbol, nikoli vybraný fyzický díl. Model, footprint a mapování padů doplnit. |
| 1 | OLED 128×64 | Uživatel potvrdil, že displej má; přesný model/objednávka zde není doložena. Řadič, I²C/SPI, napájení, konektor a adresa TBD. |
| 1 | 1S Li-Pol článek | Typ článku ani kapacita nejsou ověřeny; dřívější zmínka 500 mAh není potvrzená specifikace. Ověřit ochranu a povolený nabíjecí proud v datasheetu. |
| 1 | Nabíječka a power-path | Dříve se uvažoval modul TP4056, jeho přesná varianta/schéma nejsou známy. Bez ověřeného power-path nepovažovat nabíjení při zapnuté zátěži za vyřešené. |
| 1 | Zvyšující měnič | Dříve uvažovaný MT3608; konkrétní modul, limit, účinnost, trimr a bezpečné nastavení TBD. |
| 1 | Vibrační motor | Požadavek/plán, nikoli potvrzený díl. Model, napětí a proud TBD. |
| 1 | Spínací tranzistor, rezistor a flyback dioda | Zvažován PN2222A, bázový rezistor a 1N4007; proud, pinout tranzistoru, dimenzování a orientaci ověřit podle konkrétních dílů a obvodu. |
| 2 | Pull-upy pro I²C | Hodnota a osazení závisí na délce/kapacitě sběrnice a již přítomných rezistorech modulů. 4,7 kΩ je pouze výchozí návrhový příklad. |
| TBD | Blokovací a lokální kondenzátory | Hodnoty a umístění podle datasheetů konkrétních IC/modulů. Holý BMI160 není ekvivalent modulu s kondenzátory. |
| TBD | Dělič napětí baterie | Dříve uvažovány 100 kΩ + 100 kΩ, ale dostupný screenshot vyžaduje kontrolu napojení; ověřit ADC rozsah a GPIO. |
| TBD | Optický snímač pro podložku | Otevřená architektonická volba. PMW3360 byl jen nápad, nikoli vybraný nebo objednaný díl. |
| TBD | Scroll/menu ovladač, režimový přepínač, stavová LED | Výběr a přiřazení pinů zatím nejsou dokončené. TTP223 sám není plnohodnotný dotykový slider. |

Přesný model nelze z těchto údajů objednat bez dalšího ověření: **odkaz / přesný model k doplnění**. Nejde o tvrzení, že kterýkoli díl byl koupen nebo je fyzicky k dispozici, s výjimkou OLED, u kterého je potvrzena pouze existence uživatelem.

## Materiál k breadboardu

Původní seznam zmiňoval kontaktní pole, propojovací vodiče, rezistory, kondenzátory, LED, 1N4007 a PN2222A jako obsah startovací sady. Není potvrzené, které položky uživatel vlastní ani jejich přesné hodnoty a pouzdra. Před stavbou ověřit označení součástek a pinout; breadboardové zapojení bude samostatnou etapou po schématu, nikoli náhradou návrhu finálního obvodu.

## Zdroje výběru

- [BMI160 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi160-ds000.pdf)
- [ESP32-C3 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [TP4056 datasheet – kopie](https://www.alldatasheet.com/datasheet-pdf/pdf/1132405/ASIC/TP4056.html)
- [KiCad knihovny a dokumentace](https://docs.kicad.org/)

Přehled pinů a elektrických omezení je v [elektrickém návrhu](../design/circuits/CIRCUITS.md). Otevřené výběry jsou součástí [roadmapy](../guide/ROADMAP.md).
