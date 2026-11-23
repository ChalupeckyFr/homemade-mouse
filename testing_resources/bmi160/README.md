# BMI160: verzované experimenty

Jde o samostatné pokusy, nikoli hotový firmware myši. Soubory jsou
rekonstruované podle kódu z chatu; nezaručujeme, že jsou totožné s lokální
kopií nahranou na desce. Stávající experiment v `experiments/bmi160-motion`
zůstává zachovaný a má jiné ovládání.

| Verze | Sketch | Funkce |
|---|---|---|
| v1.0.0-beta | [v1_0_0_beta.ino](v1_0_0_beta/v1_0_0_beta.ino) | Detekce klidu, `s`, `k`, `d`, výpis verze `v`. |
| v1.1.0-beta | [v1_1_0_beta.ino](v1_1_0_beta/v1_1_0_beta.ino) | Navíc experimentální zpětná korekce příkazem `r`. |

Otevři přímo vybraný `.ino` soubor v Arduino IDE; jeho složka se jmenuje
stejně jako soubor. Deska je ESP32-C3, nastavení `ESP32C3 Dev Module`,
`USB CDC On Boot: Enabled`, port vyber podle počítače. Sériový monitor má
rychlost **115200 baud**. Zapojení podle již fungující sestavy: GPIO20 SDA,
GPIO10 SCL, senzor na adrese `0x69`. Zapojení měň pouze s odpojeným USB.

V monitoru odešli `v` pro verzi, `s` pro kalibraci a začátek, `k` pro konec,
`d` pro CSV a ve v1.1.0-beta `r` pro korekci. Nové `s`, reset a nahrání
programu smažou záznam v RAM; předtím si jej ulož. Měření se automaticky
zastaví nejpozději po 12 sekundách.

Korekce předpokládá skutečný klid před pohybem i po něm a rovnoměrný
nárůst chyby rychlosti. Nezná skutečnou vzdálenost, není zárukou přesnosti
a nemění uložené původní hodnoty. Falešné označení klidu, zejména při
rovnoměrném pohybu, může způsobit chybnou korekci. Úseky bez potvrzeného
klidu nejsou do opraveného výsledku zahrnuté.
