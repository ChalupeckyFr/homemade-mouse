# Budoucí testovací plán

Všechny body jsou plánované kontroly; repozitář neobsahuje výsledky těchto testů. Zkoušky provádět postupně a nepřipojovat baterii, dokud nejsou dokončeny bezpečnostní kontroly napájení. Testy nabíjení se zátěží až po návrhu a revizi skutečného power-path.

## Před napájením

- Zkontrolovat polaritu a orientaci baterie, konektorů, elektrolytických kondenzátorů, diody motoru, tranzistoru a dalších polarizovaných součástek podle jejich datasheetů.
- Multimetrem ověřit kontinuitu GND, absenci zkratu napájecích větví a uzavřenou cestu obou pólů baterie/modulů.
- Ověřit ochranu článku, napěťové a proudové limity všech modulů, nastavení nabíjecího proudu podle datasheetu konkrétního článku a tepelných podmínek.
- Měřit výstup zdroje a měniče před připojením ESP32. Ověřit USB proti boostu kvůli možnému zpětnému napájení.
- Zkontrolovat, že ADC dělič přichází z bateriového uzlu před boostem a že GPIO nikdy nedostává 5 V.
- V KiCadu provést ERC. Každé varování buď opravit, nebo vysvětlit; ERC nenahrazuje kontrolu schématu, datasheetů, footprintů ani elektrickou zkoušku.

Zaznamenat: datum, revizi zapojení, přístroje, napětí, proudový limit zdroje, naměřené hodnoty a odchylky. Nedomýšlet číselné výsledky.

## Funkce obvodů

1. **Napájení s omezením proudu:** nejprve bez článku, ověřit stabilní napájení a odběr. Limit nastavit podle ověřených požadavků komponent, nikoli odhadem.
2. **BMI160/I²C:** ověřit 3,3V logiku, SDA/SCL pull-upy a adresu skenerem; následně přečíst očekávané chip ID podle Bosch datasheetu. Pokud sken selže, zkontrolovat CSB, SDO, napájení a pull-upy.
3. **Tlačítka:** ověřit HIGH v klidu a LOW při stisku pro každý kontakt; sledovat debounce a správné názvy LMB/RMB/Back/Forward.
4. **Boot piny:** provést start/reset bez stisku a se stisky tlačítek na GPIO8/GPIO9; zaznamenat režim startu a ověřit konkrétní desku.
5. **ADC baterie:** porovnat napětí článku/zdroje a ADC výpočet při několika bodech v povoleném rozsahu, ověřit attenuation, kalibraci, ustálení a vliv zátěže.
6. **Motor:** až po kontrole jeho napětí/proudu, tranzistoru, bázového odporu a orientace flyback diody. Sledovat pokles napájení, rušení, restarty a I²C chyby při spínání.
7. **BLE HID:** otestovat pairing, připojení, odpojení a opětovné připojení; ověřit, že odpojení ani zakmitání nevyvolá zaseknuté tlačítko.
8. **Pohyb:** měřit drift v klidu, odezvu, latenci, opakovatelnost, spotřebu a chování filtrů v různých polohách. Zvlášť zaznamenat, že IMU test neprokazuje podložkové XY tracking ani výšku.
9. **OLED a UI:** po výběru modulu ověřit napájení, adresu, obnovu displeje a to, že vykreslování neblokuje snímání/HID.
10. **Nabíjení:** provést pouze po revizi konkrétní nabíječky, článku a power-path. Bez ověřeného power-path nabíjet s odpojenou zátěží. Zaznamenat napětí, proud, teplotu a chování ukončení nabíjení podle specifikace článku.

## Záznam výsledku

Pro každý test uveď stav `neprovedeno`, `vyhověl` nebo `nevyhověl`, datum, revizi hardwaru/firmwaru, podmínky, měřicí přístroje, skutečné hodnoty a případné chyby. Nezaměňovat návrhový parametr, simulaci, kontrolu ERC a fyzický výsledek.
