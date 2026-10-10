# Průvodce projektem

Projekt je ve fázi návrhu a učení. Cílem není předstírat hotovou myš: zatím nejsou doložené úplné schéma, PCB, firmware, kusovník ověřených objednávek ani fyzické zkoušky.

## Záměr a princip

ESP32-C3 má číst tlačítka a BMI160, případně OLED, snímat bateriové napětí a později odesílat relativní pohyb a stisky přes BLE HID. Další zvažované bloky jsou vibrační motor a optický snímač pro použití na podložce. Každá z těchto funkcí potřebuje samostatný elektrický a softwarový návrh.

```text
Tlačítka ────────────────┐
BMI160 (I²C) ────────────┤
OLED 128×64 (rozhraní ?) ├──> ESP32-C3 ───> BLE HID ───> počítač
Baterie + monitorování ──┤
Motor (spínací stupeň) ──┘
Optický pohybový snímač: volitelný blok, výběr TBD
```

## Stav důkazů

- **Potvrzené záměry zapojení:** BMI160 SDA na GPIO20, CSB na 3,3 V, SDO na GND; čtyři tlačítka mají mapování popsané v centrální tabulce.
- **Odvozeno ze screenshotu:** BMI160 SCL je na GPIO10. Nutno porovnat se skutečným KiCad souborem, který zatím chybí.
- **Plán:** OLED rozlišení 128×64; vibrační odezva; Normal/Air režimy; bateriové napájení.
- **TBD:** přesný model/rozhraní displeje, baterie a moduly, bezpečná nabíjecí cesta, Normal tracking, finální pin budget, scroll/menu a footprinty.
- **Neověřeno fyzicky:** veškeré zapojení, chování, spotřeba, latence, výdrž i nabíjení.

Jedinou tabulku pinů udržuje [elektrický návrh](../design/circuits/CIRCUITS.md). Ostatní stránky na ni odkazují, aby se přidělení pinů nerozcházelo.

## Vysvětlení ovládacího vstupu

Tlačítko není zdrojem napájení pro vstup. Interní pull-up v ESP32-C3 (řádově desítky kΩ, přibližně 45 kΩ typicky podle [datasheetu ESP32-C3](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)) slabě připojuje GPIO k 3,3 V. Rozpojené tlačítko ponechá vstup HIGH. Po stisku kontakt spojí GPIO se zemí, takže vstup čte LOW. Malý proud protéká přes pull-up a tlačítko; samotný vstup snímá napětí. Bez pull-upu by rozpojený GPIO mohl plavat. Externí pull-up je také platné řešení, pokud je jeho použití zdůvodněné. Do GPIO uzlu se nesmí připojit 3,3 V přímo bez omezení proudu.

Schéma kontaktů a mapování názvů tlačítek jsou v [elektrickém návrhu](../design/circuits/CIRCUITS.md). Mechanický kontakt může zakmitat; při čtení je potřeba debounce. Případné ukázky Arduino kódu budou jen ilustrativní, ne existující firmware.

## Pohybová data

BMI160 měří gyroskopickou úhlovou rychlost a akcelerometrickou specifickou sílu včetně gravitace. V Air režimu se dá navrhnout relativní mapování: vzorkování → kalibrace biasu v klidu → filtrování/mrtvá zóna → použití časového kroku `dt` → relativní `dx/dy` → BLE HID. Frekvence, citlivost a latence jsou hodnoty k návrhu a následnému změření, nikoli dosavadní výsledky. Hostitelský operační systém určuje výslednou odezvu kurzoru; pohyb nelze slíbit jako přesný počet obrazových pixelů.

Akcelerometr může pomoct odhadovat náklon roll/pitch, ale neposkytuje referenci yaw. Dvojí integrace akcelerace rychle kumuluje chybu. Zrychlení osy Z ani klidová kalibrace neurčují výšku nad stolem. BMI160 samotný nezajišťuje optické XY sledování po podložce. Požadovaný Normal režim je proto otevřený technický problém; optický snímač a detekci zdvihu je třeba vybrat a ověřit. Podrobnosti a zdroje jsou v [elektrickém návrhu](../design/circuits/CIRCUITS.md).

## OLED, režimy, scroll a menu

Displej je uživatelem potvrzen pouze jako OLED 128×64. Přesný produkt, řadič, I²C/SPI rozhraní, napájecí varianta a adresa nejsou doloženy. Čtyřpinový I²C modul s 3,3V logikou by mohl sdílet existující SDA/SCL, ale až po kontrole jeho pinoutu, pull-upů a adresy; adresu 0x3C nepředpokládat. Na displeji lze plánovat baterii, BLE stav, režim, citlivost (ne „DPI“, pokud není optický snímač), diagnostiku a případně monochromatickou bitmapu. Časové údaje vyžadují zdroj času, synchronizaci nebo RTC.

Přepínač Normal/Air zatím nemá GPIO. Mechanický SPST/SPDT kontakt lze číst jako vstup s pull-upem podle skutečného přepínače; fyzický prostřední kontakt není automaticky COM. Přepnutí režimu je nutné navrhnout tak, aby nezpůsobovalo nechtěné kliknutí nebo skok kurzoru.

„Touch-scroll“ ještě není zvolen. TTP223 je jednobodový dotykový spínač, ne vícedotykový touchpad ani spojitý slider. Možnosti k rozhodnutí: dvojice dotykových kontaktů pro krokový scroll, více elektrod s vhodným řadičem pro slider, nebo mechanický enkodér. Enkodér může posouvat i menu, ale jeho A/B, push kontakt, debounce a ergonomické umístění musí být navržené. Zda bude společný pro scroll a menu, nebo oddělený, zatím není rozhodnuto. Obnova displeje nesmí blokovat snímání a odesílání vstupů.

## Napájení a monitoring

Uvažuje se 1S Li-Pol, nabíjecí modul a boost na napájení SuperMini. Přesný článek, povolený proud nabíjení, ochrany a skutečné schéma modulů jsou TBD. TP4056 nemá systémový power-path; běžný modul s OUT+/OUT− není automaticky USB 5V zdroj ani řešení pro bezpečné nabíjení se zapnutou zátěží. Dokud nebude zvolená a ověřená cesta, nabíjet jen s odpojenou zátěží. Detailní podmínky, kontrola polarity a měření baterie jsou v [elektrickém návrhu](../design/circuits/CIRCUITS.md).

Záměr bateriového monitoru je dělič z bateriového uzlu před boostem na ověřený ADC. Hodnota GPIO0 je zatím jen dřívější návrh. ADC z napětí odhaduje stav nabití, nikoli stav nabíjení; převod na procenta bude orientační. Žádný 5V signál nesmí jít přímo na ESP32 GPIO.

## Opravy původních doporučení

- Neověřené odkazy na prodejce, parametry, stav objednávky ani vlastnictví součástek nepovažovat za fakta; seznam nyní uvádí TBD, pokud přesný model není znám.
- TP4056 sám nemá load sharing/power-path a jeho OUT není automaticky 5V USB výstup. Nabíjení při napájené zátěži zůstává otevřeným bezpečnostním bodem.
- I²C vyžaduje vhodné pull-upy na 3,3 V, nikoli pull-downy. 4,7 kΩ je možný výchozí bod k výpočtu, nikoli vždy nutná hodnota.
- Kondenzátory nejsou automaticky zbytečné na breadboardu ani zaručeně dostačující jen proto, že jsou na modulu. Potřebnost a umístění se řídí konkrétním IC/modulem a datasheetem.
- Tlačítko GPIO–kontakt–GND s pull-upem je platné zapojení. Není pravda, že externí pull-up se nikdy nesmí použít; 3,3 V se ale nesmí připojit přímo na GPIO uzel bez rezistoru.
- BMI160 není výškoměr ani optická myš. Jeho akcelerometrická osa Z neposkytuje vzdálenost a IMU integrace nedává spolehlivé podložkové XY sledování.
- GPIO8/GPIO9 jsou boot/strapping piny a vyžadují kontrolu resetu; netřeba však bez revize prohlásit aktuální mapování za bezpečné nebo nebezpečné.
- `SW_Push` je obecný symbol spínače, nikoli konkrétní Omron ani zaručený footprint. Ověřit model a mapování vývodů.
- ESP32-C3 používá BLE, nikoli Bluetooth Classic; nativní USB Serial/JTAG není obecný USB HID.

## Autoritativní zdroje

- [Bosch BMI160 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi160-ds000.pdf) a [BMI160 SensorAPI](https://github.com/boschsensortec/BMI160_SensorAPI) (repozitář API není tímto projektem prohlášen za aktivně udržovaný).
- [ESP32-C3 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf), [GPIO dokumentace](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/gpio.html) a [hardware checklist](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32c3/schematic-checklist.html).
- [TP4056 datasheet výrobce TPower](http://toppwr.com/uploadfile/file/20240130/65b892bb04a3c.pdf) ([kopie u AllDatasheet](https://www.alldatasheet.com/datasheet-pdf/pdf/1132405/ASIC/TP4056.html)), [TI power-path vysvětlení](https://www.ti.com/lit/an/slyt842/slyt842.pdf) a [Microchip AN1149](https://ww1.microchip.com/downloads/en/AppNotes/01149a.pdf).
- [NXP AN3397: IMU dead reckoning](https://www.nxp.com/docs/en/application-note/AN3397.pdf) a [článek o omezeních odhadu polohy IMU](https://arxiv.org/abs/1311.4572).
- [KiCad dokumentace](https://docs.kicad.org/) pro symboly, footprinty, ERC a knihovny; [Windows mouse input](https://learn.microsoft.com/en-us/windows/win32/learnwin32/mouse-input).

## Další kroky

Pořadí etap a nevyřešené závislosti jsou v [roadmapě](ROADMAP.md). Budoucí kontroly včetně bezpečnostní revize napájení jsou popsané v [testovacím plánu](TESTING.md). Výsledky lze zapsat až po skutečném provedení testů.
