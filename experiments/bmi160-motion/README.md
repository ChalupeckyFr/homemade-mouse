# Měřicí experiment BMI160

Samostatný začátečnický experiment pro ESP32-C3 SuperMini a modul BMI160. Jeho cílem je zaznamenat skutečná data z akcelerometru a gyroskopu a zkusit, jak se chová dvojí integrace zrychlení. Nejde o firmware myši, důkaz přesného 3D sledování ani příslib, že BMI160 dokáže spolehlivě měřit polohu.

**Výsledky zatím nejsou naměřené.** Sketch a hostitelský kód lze testovat bez senzoru; fyzické výsledky doplní až uživatel po sestavení a zaznamenání konkrétního pokusu.

## Bezpečné zapojení na breadboardu

Použij pouze ESP32-C3 SuperMini napájené přes USB a hotový, pro logiku 3,3 V vhodný breakout BMI160. **Holý LGA čip BMI160 není součástka určená k zasunutí do breadboardu.** Neodvozuj piny modulu z pouzdra čipu, symbolu schématu nebo fotografie jiného modulu.

| ESP32-C3 SuperMini | Modul BMI160 | Poznámka |
|---|---|---|
| 3V3 | ověřený napájecí vstup modulu pro 3,3 V | Ověř označení i napájecí rozsah konkrétního modulu; nepřipojuj neznámý pin. |
| GND | GND | Společná zem. |
| GPIO20 | SDA | Arduino sketch nastavuje I²C explicitně přes `Wire.begin(20, 10)`. |
| GPIO10 | SCL | Zapojení převzaté z projektového schématu; ověř konkrétní desku. |
| 3V3 | CSB, pouze pokud je pin na modulu vyveden | Volí I²C. |
| GND | SDO, pouze pokud je pin na modulu vyveden | Volí adresu `0x68`. |

U modulu se čtyřmi piny SDA/SCL/3V3/GND **nepřidávej ani nehledej neexistující CSB/SDO**; ověř, že modul už má piny rozhraní a adresu připravené podle dokumentace výrobce. Pokud pinout nebo napájení konkrétního kusu není jasné, před připojením pošli fotografii obou stran modulu a jeho označení.

SDA i SCL potřebují pull-up na 3,3 V. Nejdřív zkontroluj, zda je modul už obsahuje; 4,7 kΩ je jen výchozí návrhová hodnota, nikoli pokyn přidat další dva rezistory paralelně. Použij krátké vodiče. Napájecí kolejnice breadboardu mohou být uprostřed přerušené — ověř jejich kontinuitu.

Připoj pouze USB do ESP32. **Nepřipojuj baterii, TP4056, MT3608 ani jiný boost zdroj současně s USB.** Nepřipojuj OLED, tlačítka (ani GPIO6/7/8/9), motor nebo BLE příslušenství. Připevni ESP32, modul i breadboard k lehké pevné podložce a pohybuj celou podložkou; netahej za vodiče ani nenechávej breadboard volně viset.

GPIO20 může být na konkrétním C3 použité také pro UART. V Arduino IDE zvol desku odpovídající ESP32-C3 SuperMini (obvykle **ESP32C3 Dev Module**), správný port a u podporovaného core nastav **Tools → USB CDC On Boot → Enabled**. Sketch posílá log přes USB Serial/JTAG/CDC, nikoli přes myší HID. USB rozhraní ESP32-C3 není samo o sobě USB HID myš. Zaznamenej verzi Arduino-ESP32 core a skutečnou volbu desky; názvy voleb se mohou lišit podle instalované verze.

## Co sketch měří a jak počítá

- Při startu prohledá adresy `0x68` a `0x69`, očekává chip ID `0xD1`, ověří zápis konfigurace a PMU status. Při selhání I²C nebo kontroly senzor zastaví integraci.
- I²C je nastaveno explicitně na SDA GPIO20, SCL GPIO10 a 100 kHz. Akcelerometr i gyroskop běží na 100 Hz; rozsahy jsou ±2 g a ±250 °/s.
- Čte podepsané 16bitové vzorky pouze tehdy, když status hlásí nová data obou senzorů. `dt_s` vychází ze skutečných časových značek; log obsahuje také kumulativní `missed_samples` a `serial_dropped_rows`. Výpis řádku se přeskočí, pokud není dost místa ve výstupním bufferu, místo aby blokoval získávání vzorků.
- CSV obsahuje čas `timestamp_ms` relativní od začátku experimentu, `dt_s`, surové počty LSB, akceleraci v m/s², úhlovou rychlost v rad/s, korigované zrychlení, odhad rychlosti a posunu, platnost kalibrace, stavové příznaky a ID označeného segmentu. Komentáře začínající `#` nesou konfiguraci a události; analyzátor je přeskočí.
- Výchozí režim je `RAW_INTEGRATION`: bez mrtvé zóny, skrytého clampu nebo automatického vynulování rychlosti. Zrychlení integruje lichoběžníkovou metodou.
- Gyro baseline se ukládá zvlášť. Akcelerační baseline je průměr celého startovního vektoru, který obsahuje gravitaci i bias. Jeho odečítání platí jen pro **FIXED_ORIENTATION_ONLY**. Při náklonu se změní složky gravitace a vznikne zdánlivé zrychlení; sketch nedělá orientační fúzi ani obecnou 3D kompenzaci gravitace.
- Po resetu senzoru a jeho ustálení se nejprve sbírají nejméně 2 sekundy vzorků. Kontrola rozptylu, úhlové rychlosti a normy akcelerace přibližně `g` při nevyhovujícím okně kalibraci opakuje. Je to pouze heuristika: malé pohyby mohou projít a nulové translační zrychlení neznamená, že zařízení stojí — může se pohybovat konstantní rychlostí.
- Volitelný režim ZUPT lze přepnout příkazem `m`. Pokud zrychlení po dobu alespoň 0,5 s vypadá blízko baseline a gyro blízko biasu, sketch vynuluje odhad rychlosti a stav označí `ZUPT_APPLIED`. To může potlačit drift v odhadnutém klidu, ale neobnoví skutečnou polohu a ze samotné IMU nelze spolehlivě odlišit klid od pohybu konstantní rychlostí. Porovnávej ho s raw režimem, nevybírej výsledek jen podle „hezčího“ grafu.

Šum je náhodné kolísání měření; bias je trvalejší posun nulové hodnoty; drift je postupná změna odhadu v čase. Dvojí integrace zesiluje chybu: konstantní chyba zrychlení `b` způsobí chybu polohy přibližně `0,5 × b × t²`. **16bitové rozlišení není totéž co přesnost.** Nižší šum jiného ani dražšího akcelerometru sám o sobě nezaručuje přesnou 3D polohu. Tento PR vybírá ani nenakupuje jiný senzor a nemění BOM.

## Nahrání, záznam a analýza

1. Nainstaluj Arduino IDE s podporou ESP32, otevři `bmi160-motion.ino`, zvol desku/port a nahraj sketch. Zaznamenej verzi IDE, ESP32 core a přesný název desky.
2. Pro zachycení celého logu použij monitor, který umí výstup ukládat do souboru. Arduino IDE Serial Monitor nemá ve všech verzích přímé ukládání do souboru. S Arduino CLI lze po nahrání použít například:

   ```sh
   arduino-cli monitor -p /dev/ttyACM0 --config baudrate=921600 | tee bmi160-run.csv
   ```

   Na Windows nahraď port například `COM5` a použij `Tee-Object -FilePath bmi160-run.csv`; případně použij terminál s funkcí logování. Otevři monitor a resetuj ESP32, aby se zaznamenaly i inicializace a kalibrace. Ukonči záznam `Ctrl+C`. Počet zahozených řádků musí být v logu nula nebo musí být výslovně uveden v reportu.
3. Spusť analýzu standardní knihovnou Pythonu:

   ```sh
   python3 experiments/bmi160-motion/analyze_log.py bmi160-run.csv
   ```

   Při nezávisle změřené koncové souřadnici v metrech lze připojit například `--reference-end-m 0.10,0,0`. Report porovná `odhad − skutečnost`; nenahrazuje referenční měření ani správně označené segmenty.
4. Hostitelské testy, bez připojeného ESP32:

   ```sh
   python3 -m unittest discover -s experiments/bmi160-motion/tests -v
   g++ -std=c++11 -Wall -Wextra -Werror -pedantic \
     experiments/bmi160-motion/tests/test_motion_math.cpp \
     -o /tmp/bmi160-motion-math-test
   /tmp/bmi160-motion-math-test
   ```

   Tyto testy ověřují matematiku a analýzu, **nikoli kompilaci Arduino sketche ani hardware**.

Příkazy do sériového terminálu:

| Příkaz | Funkce |
|---|---|
| `c` | Opakovat kalibraci po dobu nejméně 2 s a vynulovat rychlost/posun. |
| `C` | Opakovat kalibraci po dobu nejméně 10 s a vynulovat rychlost/posun. |
| `z` | Vynulovat rychlost a posun bez nové kalibrace. |
| `m` | Přepnout mezi `RAW_INTEGRATION` a volitelným ZUPT. |
| číslice `0` až `9` | Změnit `segment_id` pro označení úseku; změna je také uvedena v komentáři logu. |

## Navržený fyzický postup

Každý log opatři datem, modulem/sestavou, deskou, verzemi IDE/core, teplotními a mechanickými podmínkami, délkou a orientací vodičů, použitým režimem, ID segmentů a nezávislými referenčními rozměry. **Nedoplňuj výsledky, dokud nejsou skutečně naměřené.**

1. **Klid a drift:** po startu nech modul bez pohybu pro alespoň 2s kalibraci. Zaznamenej 60 s klidu. V CSV označ segmenty kolem 1, 2, 5, 10, 30 a 60 s; přečti odhady posunu a porovnej raw/ZUPT. Opakuj s příkazem `C` (10s kalibrace) a po zvoleném zahřátí; přesný čas předem i po zahřátí zapiš. Samotné „nepohnulo se“ není důkaz absolutního klidu ani nulové skutečné rychlosti.
2. **Vodorovný posun:** udržuj stejný náklon, vyznač rovnou dráhu 10 cm, posuň senzor, zastav na 2 s a vrať ho do výchozího místa. Opakuj 10× pomalu a 10× rychle. Změř skutečnou dobu, směr a konečnou polohu nezávisle; sleduj znaménko každé osy a posun po návratu (referenční konečný posun je nula).
3. **Náklon bez záměrného posunu:** nakloň sestavu přibližně kolem senzoru a sleduj změnu odhadu. Při otáčení kolem jiného bodu se může fyzicky posunout i samotný senzor, takže takový pohyb zaznamenej zvlášť.
4. **Zdvih:** až po předchozích zkouškách proveď svislý zdvih přibližně o 1 cm a potom 5 cm bez náklonu a vrať sestavu. Bez optiky či jiné nezávislé reference nelze garantovat centimetrový zdvih ani lift-off detekci.

Pro každý pokus uveď `chyba = odhad − skutečnost`, rozptyl/opakovatelnost mezi pokusy, drift v klidovém úseku, skutečnou vzorkovací frekvenci, počty vynechaných/zahozených vzorků a čas, po kterém odhad chyby polohy překročí 1 cm. Označení segmentu samo o sobě nedokazuje, že byl úsek skutečně nehybný.

## Zdroje

- [Bosch BMI160 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi160-ds000.pdf) — registry, rozsahy, časování a jednotky.
- [Kopie datasheetu BMI160 u Mouser](https://www.mouser.com/datasheet/2/783/BST-BMI160-DS000-1509569.pdf).
- [Bosch BMI160 SensorAPI](https://github.com/boschsensortec/BMI160_SensorAPI) — ověření názvů a kódování registrů; experiment nepřebírá její implementaci.
- [Arduino-ESP32: I²C](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/i2c.html) a [USB CDC](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/usb_cdc.html).
