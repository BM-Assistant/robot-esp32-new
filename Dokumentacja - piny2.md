### ESP32
- `VIN` - `OUT+` [[Dokumentacja - piny#LM2596|LM2596]]
- `GND` - `OUT-` [[Dokumentacja - piny#LM2596|LM2596]]
- `GND2` - `IN_GND` [[Dokumentacja - piny#PAM8403|PAM8403]]
- `D14` - `SCL` [[Dokumentacja - piny#Ekranik|Ekranik]]
- `D27` - `SDA` [[Dokumentacja - piny#Ekranik|Ekranik]]
- `D13` - `ENA` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]
- `D23` - `IN1` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]
- `D04` - `IN2` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]
- `D15` - `ENB` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]
- `D33` - `IN3` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]
- `D32` - `IN4` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]
- `D25` - `IN_L` __/__ `IN_R` [[Dokumentacja - piny#PAM8403|PAM8403]] *(Przez filtr RC)*

### Silniki (autko)
##### Silnik przedni (skręcanie)
- `PIN1` - `OUT3` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]
- `PIN2` - `OUT4` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]
##### Silnik tylny (napęd)
- `PIN1` - `OUT1` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]] 
- `PIN2` - `OUT2` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]

### Głośnik
- `+ (Plus)` - `OUT_L+` __/__ `OUT_R+` [[Dokumentacja - piny#PAM8403|PAM8403]]
- `- (Minus)` - `OUT_L-` __/__ `OUT_R-` [[Dokumentacja - piny#PAM8403|PAM8403]]

### Mostek L298N
- `12V` - `VCC` [[Dokumentacja - piny#Akumulator|Akumulator]]
- `5V` - `OUT+` [[Dokumentacja - piny#LM2596|LM2596]]
- `GND` - `IN-` [[Dokumentacja - piny#LM2596|LM2596]] __&__ `GND` [[Dokumentacja - piny#Akumulator|Akumulator]]
- `OUT1` - `PIN1` [[Dokumentacja - piny#Silnik tylny (napęd)|Silnik tylny (napęd)]]
- `OUT2` - `PIN2` [[Dokumentacja - piny#Silnik tylny (napęd)|Silnik tylny (napęd)]]
- `OUT3` - `PIN1` [[Dokumentacja - piny#Silnik przedni (skręcanie)|Silnik przedni (skręcanie)]]
- `OUT4` - `PIN2` [[Dokumentacja - piny#Silnik przedni (skręcanie)|Silnik przedni (skręcanie)]]
- `ENA` - `D13` [[Dokumentacja - piny#ESP32|ESP32]]
- `IN1` - `D23` [[Dokumentacja - piny#ESP32|ESP32]]
- `IN2` - `D04` [[Dokumentacja - piny#ESP32|ESP32]]
- `ENB` - `D15` [[Dokumentacja - piny#ESP32|ESP32]]
- `IN3` - `D33` [[Dokumentacja - piny#ESP32|ESP32]]
- `IN4` - `D32` [[Dokumentacja - piny#ESP32|ESP32]]

### Akumulator
- `VCC` - `IN+` [[Dokumentacja - piny#LM2596|LM2596]] __&__ `12V` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]] *(Przez przełącznik ON/OFF)*
- `GND` - `IN-` [[Dokumentacja - piny#LM2596|LM2596]] __&__ `GND` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]

### Ekranik
- `VCC` - `OUT+` [[Dokumentacja - piny#LM2596|LM2596]]
- `GND` - `OUT-` [[Dokumentacja - piny#LM2596|LM2596]]
- `SCL` - `D14` [[Dokumentacja - piny#ESP32|ESP32]]
- `SDA` - `D27` [[Dokumentacja - piny#ESP32|ESP32]]

### PAM8403
- `5V` - `OUT+` [[Dokumentacja - piny#LM2596|LM2596]]
- `GND` - `OUT-` [[Dokumentacja - piny#LM2596|LM2596]]
- `IN_L` __/__ `IN_R` - `D25` [[Dokumentacja - piny#ESP32|ESP32]] *(Przez filtr RC)*
- `IN_GND` - `GND2` [[Dokumentacja - piny#ESP32|ESP32]]
- `OUT_L+` __/__ `OUT_R+` - `+ (Plus)` [[Dokumentacja - piny#Głośnik|Głośnik]]
- `OUT_L-` __/__ `OUT_R-` - `- (Minus)` [[Dokumentacja - piny#Głośnik|Głośnik]]

### LM2596
- `IN+` - `VCC` [[Dokumentacja - piny#Akumulator|Akumulator]]
- `IN-` - `GND` [[Dokumentacja - piny#Akumulator|Akumulator]]
- `OUT+` - `VIN` [[Dokumentacja - piny#ESP32|ESP32]] __&__ `VCC` [[Dokumentacja - piny#Ekranik|Ekranik]] __&__ `5V` [[Dokumentacja - piny#PAM8403|PAM8403]] __&__ `5V` [[Dokumentacja - piny#Mostek L298N|Mostek L298N]]
- `OUT-` - `GND` [[Dokumentacja - piny#ESP32|ESP32]] __&__ `GND` [[Dokumentacja - piny#Ekranik|Ekranik]] __&__ `GND` [[Dokumentacja - piny#PAM8403|PAM8403]]

### Dodatkowe informacje
- Każdy silnik będzie miał wpięty bezpośrednio do jego + i - kondensator ceramiczny 104 *(100nF)*
- **Filtr Audio RC:** Między pinem `D25` a wejściem `IN_L` wzmacniacza wlutowany będzie rezystor 1kΩ. Bezpośrednio za rezystorem dołożony będzie kondensator 100nF, z którego druga nóżka powędruje do czystej masy `IN_GND`.
- **Kondensator buforowy:** Między pinami `OUT+` a `OUT-` przetwornicy LM2596 będzie wpięty równolegle kondensator elektrolityczny 25V 1000uF (dłuższa nóżka do +, krótsza do -).
- W środku czerwonego kabla wychodzącego od `VCC` [[Dokumentacja - piny#Akumulator|Akumulator]] będzie się znajdować przełącznik ON/OFF dla natychmiastowego otwarcia całego obwodu.
- **UWAGA KRYTYCZNA:** Należy fizycznie ZDJĄĆ zworkę (jumper) "5V EN" znajdującą się na mostku L298N!