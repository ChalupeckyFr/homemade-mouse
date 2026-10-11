# Elektrický návrh

Tato stránka je autoritativním místem pro aktuální tabulku pinů a elektrické zapojení. Repozitář zatím neobsahuje KiCad projekt, proto nelze potvrdit úplnost sítí, provést ERC ani ověřit footprinty. Zapojení se musí před výrobou zkontrolovat podle konkrétní desky a datasheetů.

## Přehled bloků

```text
USB ──> nabíjecí modul ──> 1S Li-Pol ──> vypínač zátěže ──> step-up ──> ESP32-C3
              │                                              ├──> BMI160 (I²C)
              └── nabíjecí/power-path rozhodnutí TBD         ├──> tlačítka
                                                             ├──> OLED (plán)
                                                             └──> tranzistor ──> vibrační motor

Bateriový uzel ──> dělič napětí (návrh) ──> ADC (pin TBD)
```

Diagram je funkční záměr, nikoli hotové schéma. Napětí, proudové limity, ochrany a zpětné napájení je nutné ověřit pro vybrané moduly.

## Centrální tabulka pinů

`GPIOx` označuje číslo GPIO, ne fyzické číslo padu nebo konektoru. Mapování vychází z popsaného záměru a screenshotů, nikoli z kontrolovaného KiCad projektu. Označení „potvrzeno“ znamená potvrzený záměr zapojení, ne potvrzenou elektrickou bezpečnost modulu.

| Funkce / síť | ESP32-C3 GPIO | Stav a poznámka |
|---|---:|---|
| BMI160 SDx/SDA | GPIO20 | Potvrzené textové upřesnění uživatele; I²C SDA |
| BMI160 SCx/SCL | GPIO10 | Odvozeno z detailního screenshotu; ověřit v KiCad |
| Tlačítko dopředu `BTN_FORWARD` | GPIO6 | Potvrzené mapování; tlačítko mezi GPIO a GND |
| Tlačítko dozadu `BTN_BACK` | GPIO7 | Potvrzené mapování; tlačítko mezi GPIO a GND |
| Levé tlačítko `LMB` / MB1 | GPIO8 | Potvrzené mapování, ale GPIO8 je boot/strapping pin; nutná kontrola resetu |
| Pravé tlačítko `RMB` / MB2 | GPIO9 | Potvrzené mapování, ale GPIO9 je boot/strapping pin a na některých SuperMini souvisí s BOOT; nutná kontrola |
| MMB / MB3 | TBD | Zatím nepřiřazeno |
| Přepínač Normal/Air | TBD | Nepřiřazovat GPIO10; to je uvažovaná I²C SCL |
| ADC dělič baterie | TBD | GPIO0 je jen dřívější návrh a musí se ověřit vůči desce, rozsahu ADC a zapojení |
| Motor, LED, enkodér/scroll | TBD | Zatím bez finálního GPIO |
| Volitelný optický snímač | TBD | Rozhraní a pin budget závisí na výběru snímače |

GPIO8 a GPIO9 jsou u ESP32-C3 strapping piny; GPIO2 je také strapping pin. Stav externího obvodu při resetu může ovlivnit start. Ověřit konkrétní SuperMini, jeho LED/BOOT zapojení a chování se stisknutými tlačítky. GPIO20/21 se používají pro UART na některých deskách/nastaveních a GPIO18/19 pro nativní USB; ověřit také případný debug. Celkový pin budget pro I²C, ADC, motor, LED, pět tlačítek, režim, enkodér a případné SPI nelze považovat za vyřešený. Případný expander nebo jiný mikrokontrolér je otevřená možnost, ne hotové rozhodnutí.

## Tlačítka a vstupy

Každý mechanický spínač je zamýšlen mezi příslušným GPIO a společnou GND. Firmware má zapnout interní pull-up. Jde o běžný normálně rozpojený momentový kontakt, který lze v KiCadu zakreslit například symbolem `SW_Push`; symbol sám neurčuje fyzický typ spínače ani jeho footprint.

```text
               interní pull-up v ESP32
3,3 V ─────────────[ R ]────┬──── GPIO (vstup)
                            │
                         kontakt
                            │
GND ────────────────────────┘
```

Externí pull-up lze zapojit místo interního, nebo jej zvolit z odůvodněného důvodu. Jeho hodnotu je třeba určit podle konkrétního vstupu, rušení a spotřeby. Interní odpor je slabý a má širokou toleranci (řádově desítky kΩ; v datasheetu ESP32-C3 přibližně 45 kΩ typicky), proto není náhradou přesného odporu v citlivém návrhu. Bez pull-upu může být rozpojený vstup plovoucí (floating) a číst náhodné úrovně. GPIO měří logickou úroveň napětí, nikoli průtok proudu: při rozepnutém kontaktu pull-up drží uzel HIGH; při stisku kontakt stáhne uzel na GND a vstup čte LOW. Malý proud při stisku teče z 3,3 V přes pull-up a kontakt do GND. **Nepřipojovat 3,3 V přímo na uzel GPIO bez rezistoru.** Po mechanickém stisku může kontakt krátce zakmitat; firmware bude potřebovat debounce. Viz [datasheet ESP32-C3](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf).

| Stav kontaktu | Očekávaná úroveň |
|---|---|
| Uvolněno | HIGH |
| Stisknuto | LOW |

Konvenční názvy HID: `LMB`/MB1 levé, `RMB`/MB2 pravé, `MMB`/MB3 prostřední (zatím nenavržené), `BTN_BACK`/MB4/XBUTTON1 zpět a `BTN_FORWARD`/MB5/XBUTTON2 vpřed. Jde o konvenci; mapování musí odpovídat cílovému HID/API a může být uživatelsky nastavitelné. Viz [Windows mouse input](https://learn.microsoft.com/en-us/windows/win32/learnwin32/mouse-input) a [AutoHotkey seznam kláves](https://www.autohotkey.com/docs/v1/KeyList.htm).

U konkrétního Omronu ověřit přesný model, COM/NO/NC, pořadí a číslování padů i mechanický footprint. NC může zůstat nepoužitý pouze pokud to potvrzuje datasheet zvoleného modelu. Pokud má KiCad symbol dva piny COM+NO, doložit mapování na skutečné pady. Pinout se nesmí odhadovat podle toho, že COM bývá mechanicky uprostřed. Symbol i footprint kontrolovat samostatně v [KiCad dokumentaci](https://docs.kicad.org/).

## BMI160 a I²C

BMI160 je 6osá IMU: akcelerometr měří specifickou sílu (včetně vlivu gravitace) a gyroskop úhlovou rychlost. Není to optický snímač povrchu ani dálkoměr. Názvy a funkce níže vycházejí z [datasheetu Bosch BMI160](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi160-ds000.pdf); pinové doporučené zapojení a chování nepoužitých vývodů ověřit v příslušné revizi datasheetu.

| Pin / skupina | Funkce a záměr v tomto projektu |
|---|---|
| VDD | Napájení senzoru; uvažované 3,3 V dle konkrétní varianty a zapojení |
| VDDIO | Napájení logického rozhraní; uvažované 3,3 V |
| GND, GNDIO | Zem; společná zem systému podle doporučeného schématu |
| SDx/SDA | Víceúčelová datová linka primárního rozhraní; v I²C zapojení SDA na GPIO20 |
| SCx/SCL | Víceúčelová hodinová linka primárního rozhraní; v I²C zapojení SCL na GPIO10 (odvozeno ze screenshotu) |
| CSB | Volba rozhraní; v tomto I²C záměru pevně na VDDIO/3,3 V, ne na GPIO |
| SDO | V I²C volí adresový bit; záměr připojit na GND pro adresu 0x68. V SPI režimu má jinou funkci |
| INT1, INT2 | Programovatelné přerušovací výstupy; zatím bez přiřazení, lze zvolit polling |
| ASDx, ASCx | Piny sekundárního rozhraní; nepoužívat jako obecné výstupy magnetometru |
| OCSB, OSDO | Součást alternativního sekundárního/OIS rozhraní; zapojit či ponechat podle konkrétního režimu a doporučení datasheetu |

SDA a SCL potřebují pull-upy na **3,3 V**. Pull-down rezistory pro I²C nejsou náhradou pull-upů. Interní pull-up ESP32 nemusí stačit pro spolehlivou sběrnici. Například 4,7 kΩ může být výchozí návrhová hodnota, nikoli univerzální pravidlo: závisí na kapacitě a délce vedení, rychlosti a již osazených pull-upech na modulech. Více pull-upů je elektricky paralelně a jejich výsledný odpor se snižuje. Každá sběrnicová linka má vlastní odbočku přes pull-up na 3,3 V; rezistory nejsou zařazené sériově do SDA/SCL.

Ověřit, zda jde o holý BMI160 nebo modul GY-BMI160. Holý IC vyžaduje blokovací kondenzátory poblíž napájecích pinů podle typického zapojení Bosch; modul je může již obsahovat, ale ověřit jeho schéma. 100 nF poblíž příslušných napájecích pinů a lokální kapacita podle datasheetu a odběrových špiček jsou návrhové otázky. 10 µF je pouze uvažovaná hodnota, nikoli záruka pro každý obvod. Elektrolyt zapojit se správnou polaritou.

## Napájení, nabíjení a motor

Zvažovaná cesta je 1S Li-Pol s nominální hodnotou 3,7 V → vypínač zátěže → měnič MT3608 na 5 V → napájení SuperMini. U článku, vstupního/výstupního rozsahu desky, měniče, proudů a polarity konektorů je nutná kontrola podle skutečných komponent. Především ověřit, zda USB připojené k SuperMini nemůže zpětně napájet výstup boost měniče nebo bateriový systém.

TP4056 je nabíjecí obvod pro jeden Li-ion/Li-Pol článek, **nikoli 5V výstupní měnič ani systémový power-path**. Běžný modul s ochranným obvodem a vývody OUT+/OUT− automaticky power-path nedostává; OUT bývá chráněný bateriový uzel. Zátěž připojená přímo k BAT může ovlivnit ukončení nabíjení. Proto nabíjení při zapnuté zátěži není tímto návrhem bezpečně vyřešeno. Před použitím zvolit a zdokumentovat skutečný power-path obvod či vhodnou nabíječku, nebo dočasně nabíjet pouze s odpojenou zátěží. Nastavit nabíjecí proud podle datasheetu konkrétní baterie, konkrétního modulu a tepelných podmínek; kapacita 500 mAh sama o sobě neurčuje povolený proud. K TP4056: [datasheet výrobce TPower](http://toppwr.com/uploadfile/file/20240130/65b892bb04a3c.pdf) ([kopie u AllDatasheet](https://www.alldatasheet.com/datasheet-pdf/pdf/1132405/ASIC/TP4056.html)); princip power-path: [TI SLYT842](https://www.ti.com/lit/an/slyt842/slyt842.pdf) a [Microchip AN1149](https://ww1.microchip.com/downloads/en/AppNotes/01149a.pdf).

Před připojením článku zkontrolovat polaritu, spoje, zkraty multimetrem, ochranu článku a nastavený výstup měniče. Oba vodiče baterie a proudová cesta musí být uzavřené; proud se vrací společnou zemní cestou, „nezmizí v GND“. Nikdy nepřivádět 5V signál přímo na GPIO.

Motor je pouze plánovaný: uvažuje se o nízkostranném spínání tranzistorem PN2222A, bázovém rezistoru a ochranné diodě. Přesný motor, jeho jmenovité napětí a proud, pinout tranzistoru, dimenzování rezistoru i vhodnost diody zatím nejsou ověřeny. Nepředepisovat 5 V neznámému motoru ani nepředpokládat, že 1 kΩ vždy postačí. U běžně orientované flyback diody je katoda k napájecí kladné straně motoru a anoda ke spínanému nízkému pólu.

## Monitorování baterie

Záměr je měřit napětí na bateriovém/chráněném bateriovém uzlu před boost měničem přes dělič do ověřeného ADC vstupu. GPIO0 bylo pouze dřívější přidělení k prověření. Na dostupném screenshotu se dělič 100 kΩ/100 kΩ jeví napojený na horní 5V trasu; považovat to za bod ke kontrole, nikoli za hotový bateriový měřič. Měřit 5V výstup boostu neukazuje stav baterie.

Při děliči 100 kΩ / 100 kΩ platí ideálně `V_ADC = V_bat / 2`; při 4,2 V na článku je to přibližně 2,1 V. Před návrhem ověřit maximální napětí a útlum konkrétního ADC, měřicí rozsah, kalibraci, vstupní impedanci, čas ustálení a případnou potřebu kondenzátoru. Procenta odvozená jen z napětí jsou odhad závislý na zatížení, teplotě a chemii článku. Přesnější fuel gauge je možnost, ne nynější součást návrhu. Stav nabíjení nelze vyčíst pouze z napětí; případné CHRG/STDBY signály použít až po ověření skutečného modulu a logických úrovní.

## Pohybové režimy a omezení

Záměrem Air režimu je převádět **posun celé myši v X/Y/Z** na relativní pohyb kurzoru, nikoli ovládat kurzor pouhým otáčením zápěstí. Gyroskop může pomoci odhadovat orientaci a akcelerometr roll/pitch podle gravitace; orientační fúze je ale budoucí, neověřená etapa a samotná gravitace neodstraní drift yaw. Experiment s dvojí integrací v [měřicím pokusu BMI160](../../experiments/bmi160-motion/README.md) používá jen startovní baseline pro neměnnou orientaci, nikoli obecnou kompenzaci gravitace. Dvojí integrace rychle kumuluje chybu a není tímto projektem prohlášena za přesné prostorové sledování; viz [NXP AN3397](https://www.nxp.com/docs/en/application-note/AN3397.pdf) a [výzkum odhadu polohy IMU](https://arxiv.org/abs/1311.4572).

Režim Normal na podložce je požadavek, ne nynější schopnost BMI160. Klidová „kalibrace výšky“ nebo samotná osa Z neurčí výšku nad stolem: akcelerometr měří specifickou sílu, ne vzdálenost. BMI160 sám neposkytne spolehlivé XY sledování myši po povrchu. Kvalitní Normal režim vyžaduje posoudit skutečný optický snímač s vhodnou optikou a jeho detekci zdvihu; PMW3360 je zatím jen volitelný nápad. Jinou detekci kontaktu/vzdálenosti je třeba navrhnout a otestovat, neprezentovat ji jako řešení XY trackingu.

ESP32-C3 nabízí BLE, ale ne Bluetooth Classic. Jeho USB Serial/JTAG není univerzální USB HID řadič; kabelový režim myši není doložená schopnost tohoto návrhu. Pokud bude požadován, musí se samostatně rozhodnout architektura. Ověřovat [datasheet ESP32-C3](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf), [GPIO dokumentaci](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-reference/peripherals/gpio.html) a [hardware checklist](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32c3/schematic-checklist.html).

## Schéma a další revize

Před PCB doplnit úplný KiCad projekt a zkontrolovat:

1. přesný modul ESP32-C3, skutečné GPIO/pady, boot režim a sdílené periferie;
2. úplnou nabíjecí a napájecí cestu, ochrany, proudy, USB/boost zpětné napájení;
3. variantu BMI160, jeho napájecí blokování, pull-upy a stav všech nepoužitých pinů;
4. dělič baterie na chráněném bateriovém uzlu a ADC limity;
5. konkrétní spínače, mapování symbolu na pad, footprinty a orientaci motorové diody;
6. ERC; zaznamenat nevyřešené varování místo bezdůvodného potlačení;
7. rozmístění kondenzátorů u příslušných napájecích pinů a mechanickou servisovatelnost.

Exporty schémat ukládat až po vytvoření skutečného návrhu, například pod `design/circuits/exports/`, a v dokumentaci uvést datum/verzi a omezení výřezu. Aktuální obrázky z konverzace nejsou soubory v tomto repozitáři a nejsou ověřeným netlistem.
