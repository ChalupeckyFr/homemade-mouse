# Inspirace a rozhodnutí o ovládání

Tato stránka zachycuje požadavky a možné směry, nikoli výběr hotových součástek.

## Ovládání a režimy

- **Air:** ovládání ve vzduchu z relativních dat BMI160; potřebuje kalibraci, filtrování a zkoušky driftu.
- **Normal:** zamýšlené ovládání po podložce. BMI160 není optický senzor a jeho osa Z není měřič výšky; pro kvalitní XY tracking je třeba posoudit optický senzor s vhodnou optikou.
- **Režimový přepínač:** dosud není vybrán ani přidělen GPIO.
- **Scroll:** uživatel uvažuje o dotykovém ovládání podobném myším Microsoft, konkrétní produkt ale není doložen. TTP223 je jedno kontaktní dotykové tlačítko, ne souvislý touchpad. K rozhodnutí jsou krokové dotykové plochy, víc kanálový slider nebo mechanický enkodér.
- **Menu:** enkodér může vybírat položku otočením a potvrdit stiskem; způsob návratu a případné sdílení s rolováním je třeba rozhodnout.
- **OLED:** pouze rozlišení 128×64 je potvrzené. Na displeji se plánuje baterie, BLE, režim a citlivost; výraz DPI použít pouze tehdy, pokud přibude optický snímač.

## Ergonomie a integrace

Poloha hlavních/bočních tlačítek, scrollu, displeje a přepínače musí být ověřena na skutečném krytu. Neuvádět žádnou ergonomickou volbu jako otestovanou, dokud není zařízení sestavené a vyzkoušené. Rozhodnutí a jejich stav doplnit do [roadmapy](../guide/ROADMAP.md); elektronická omezení jsou v [elektrickém návrhu](../design/circuits/CIRCUITS.md).
