# Vlastní počítačová myš

Osobní projekt, ve kterém se učím navrhovat elektroniku a postupně připravit vlastní počítačovou myš. Dokumentuji rozhodnutí, otevřené otázky i chyby, na které narazím. Zatím jde o návrh: repozitář neobsahuje hotovou desku, firmware ani ověřený funkční prototyp.

> Při přípravě textů mi pomohla také umělá inteligence. Návrhy jsem musel ověřovat a tato poznámka neznamená, že zařízení bylo fyzicky otestováno.

## Stav a záměr

Záměrem je myš s mikrokontrolérem ESP32-C3, pohybovým senzorem BMI160 pro ovládání ve vzduchu, mechanickými tlačítky, bateriovým napájením, vibrační odezvou a displejem OLED 128×64. Uvažují se dva režimy: relativní ovládání ve vzduchu a běžné používání na podložce. Způsob spolehlivého snímání pohybu na podložce není vyřešen; samotný BMI160 není optický snímač myši.

Zapojení níže zachycuje záměr a dostupné upřesnění, nikoli ověřený KiCad netlist. Jednotlivé údaje rozlišují potvrzené zapojení, odvození ze screenshotu, návrh a dosud otevřenou otázku.

## Dokumentace

| Téma | Stránka |
|---|---|
| Úvod, principy, bezpečnostní opravy a hlavní rozhodnutí | [Průvodce projektem](guide/GUIDE.md) |
| Schéma, napájení, BMI160, tlačítka a jediná centrální tabulka pinů | [Elektrický návrh](design/circuits/CIRCUITS.md) |
| Součástky, jejich stav a neověřené specifikace | [Seznam součástek](components/COMPONENTS.md) |
| Etapy a otevřené úkoly | [Roadmapa](guide/ROADMAP.md) |
| Plán budoucích kontrol a zaznamenávání výsledků | [Testovací plán](guide/TESTING.md) |
| Firmware – plán, nikoli existující kód | [Software](software/SOFTWARE.md) a [Arduino kód](arduino-code/ARDUINO-CODE.md) |
| Mechanická montáž – zatím bez výrobních podkladů | [Montáž](assembly/ASSEMBLY.md) |
| Budoucí kryt | [3D návrh](design/3d-design/3D-DESIGN.md) |
| Inspirace a rozhodnutí o ovládání | [Inspirace](inspiration/INSPIRATION.md) |
| Fotografie postupu | [Fotografie](progress-photos/PROGRESS-PHOTOS.md) |

## Co je potřeba vyřešit

- Přesný model a rozhraní OLED, jeho napájení a I²C adresu.
- Bezpečný návrh nabíjení a napájecí cesty při připojené zátěži; ověřit konkrétní baterii, modul nabíječky a SuperMini.
- Zda a jak doplnit optický snímač pro režim na podložce.
- Revizi GPIO konkrétní desky, zejména boot/strapping pinů a rezervy pro všechny zamýšlené funkce.
- Způsob scrollování, enkodér/menu a přepínač režimů.
- Přesné modely součástek, footprinty, schéma, firmware, kryt a následné testy.

## Licence a stav projektu

Licence [CC0 1.0](LICENSE) zůstává zachována. Chybějící soubory nebo nevyřešené body nejsou důkazem, že byl hardware objednán, sestaven nebo otestován. Pokud přispěješ opravou, uveď prosím zdroj technického tvrzení nebo zaznamenej, že jde o nové rozhodnutí.
