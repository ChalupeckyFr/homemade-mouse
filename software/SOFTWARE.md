# Software a firmware

Zatím neexistuje implementace firmware. Tato stránka je návrhový přehled, ne popis funkčního zařízení.

## Zamýšlená architektura

1. Vstupní vrstva čte tlačítka, BMI160 a později displej/optický senzor.
2. Zpracování pohybu kalibruje bias, filtruje šum a v Air režimu převádí úhlovou rychlost na relativní pohyb.
3. HID vrstva odesílá pohyb a tlačítka přes BLE. Musí také bezpečně uvolnit tlačítka při chybě nebo odpojení.
4. Uživatelské rozhraní spravuje OLED, režim, citlivost a případné menu, aniž by blokovalo vstupní smyčku.
5. Diagnostika sleduje I²C chyby, BLE spojení a odhad napětí baterie; číselné výsledky se doplní až po měření.

ESP32-C3 má BLE, nikoli Bluetooth Classic. Jeho nativní USB Serial/JTAG není univerzální USB HID rozhraní. Kabelový HID režim by vyžadoval samostatné rozhodnutí o architektuře. Normal režim s pohybem po podložce nelze realizovat pouhou kalibrací BMI160; potřebuje další návrh snímače.

Rozhraní periferií, centrální GPIO tabulku a omezení napájení popisuje [elektrický návrh](../design/circuits/CIRCUITS.md). Implementaci a testy plánuje [roadmapa](../guide/ROADMAP.md) a [testovací plán](../guide/TESTING.md); průběžný Arduino sketch by patřil do [arduino-code](../arduino-code/ARDUINO-CODE.md).
