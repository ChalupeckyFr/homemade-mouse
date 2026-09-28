# Components – Seznam součástek

## Hlavní komponenty

| Součástka | Popis | Odkaz |
|-----------|-------|-------|
| **ESP32-C3 SuperMini** | Hlavní mikrokontroler s WiFi a Bluetooth | [LaskaKit](https://www.laskakit.cz/laskakit-esp32-c3-super-mini-wifi-bluetooth-modul/) |
| **GY-BMI160** | 6-osý gyroskop a akcelerometr (I2C) | [LaskaKit](https://www.laskakit.cz/gy-bmi160-6-osy-gyroskop-a-akcelerometr-i2c-modul/) |
| **TP4056 modul** | Nabíječka Li-ion/Li-Pol článku s ochranou | [LaskaKit](https://www.laskakit.cz/laskakit-tp4056-nabijecka-li-ion-clanku/) |
| **Li-Pol baterie 3,7V 500mAh** | Napájení myši | [LaskaKit](https://www.laskakit.cz/laskakit-lipol-baterie-503035-500mah-3-7v-jst-ph-2-0/) |
| **MT3608 step-up modul** | Zvýšení napětí z 3,7V na 5V | [LaskaKit](https://www.laskakit.cz/step-up-boost-menic-s-mt3608/) |
| **Mini vibrační motor 10×3,4mm** | Haptická odezva tlačítek | [LaskaKit](https://www.laskakit.cz/mini-vibracni-motor-10x3-4mm-5v/) |
| **Omron D2FC-F-7N** | Mikrospínač pro hlavní tlačítka myši (2×) | [LaskaKit](https://www.laskakit.cz/omron-d2fc-f-7n-20m-mikrospinac-pro-mysi/) |
| **TTP223 modul** | Kapacitní dotykové tlačítko (2×, volitelné) | [LaskaKit](https://www.laskakit.cz/kapacitni-dotykove-tlacitko-ttp223/) |

## Elektro Starter Kit (LaskaKit)

Z kitu používáme:

| Součástka | Použití |
|-----------|---------|
| **Nepájivé kontaktní pole 400 pinů** | Prototypování |
| **Napájecí modul pro breadboard** | Testování napájení |
| **Breadboard vodiče** | Propojování |
| **Dupont kabely** | Propojování |
| **Rezistory** (220Ω, 330Ω, 1kΩ, 100kΩ) | LED, dělič napětí, pull-upy |
| **Kondenzátory** (100nF, 10µF) | Filtrace napájení |
| **Dioda 1N4007** | Ochrana tranzistoru před špičkou z motorku |
| **Tranzistor PN2222A** | Ovládání vibračního motorku |
| **LED** | Indikace stavu |
| **Tlačítka** | Testování vstupů |

## K dokoupení (volitelné)

| Součástka | Popis | Poznámka |
|-----------|-------|----------|
| **PMW3360** | Optický senzor pohybu | AliExpress – pro normální použití na stole |
| **BNO085 / BNO055** | 9-osý IMU s fúzí | Pro přesnější "air mouse" režim |
| **OLED displej 0,96" 128×64** | Zobrazení stavu baterie, DPI | SSD1306, I2C |
| **JST-PH 2.0 konektor** | Protikus pro baterii | Pro snadné připojení baterie |

## Poznámky

- **Pull-up rezistory pro I2C:** 4,7kΩ mezi SDA→3V3 a SCL→3V3 (pokud modul BMI160 nemá vestavěné)
- **Nastavení MT3608:** Před připojením ESP32 nastavit trimrem na 5,0V!
- **Nabíjecí proud TP4056:** Z výroby 1A, pro 500mAh baterii ideální 0,5A (volitelná úprava)
