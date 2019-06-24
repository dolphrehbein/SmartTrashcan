# SmartTrashcan
I made a prototype of a smart trashcan to improve municipal trash collection.

[youtube video](https://m.youtube.com/watch?v=YbdOFqoOZbs)

__Features:__
- Solar panel + LiPo battery + Particle Power shield for power
- Ultra sonic sensor and IR sensor to measure the amount of trash
- Accelerometer for auto wake when opening the lid
- GPS to publish location of trashcan
- iOS app to find the location of the trash can

**Here are the components used and connections:**

[Particle Photon ](https://store.particle.io/products/photon)

[Particle Power Shield](https://store.particle.io/products/power-shield-with-headers)
D0,D1 used for battery stats

[LiPo battery](https://store.particle.io/products/li-po-battery)
2 pin JST connector to the power shield

[5 v Solar Panel](https://www.amazon.com/gp/product/B00CBT8A14/ref=ppx_yo_dt_b_asin_title_o02_s00?ie=UTF8&psc=1)


[HC-SR04 Ultra sonic distance sensor](https://www.sparkfun.com/products/13959)
5V Vcc, Gnd, Trig A0, Echo A1 via a 1k/2.2k voltage divider to drop to voltage form 5V to 3.3V

[Sharp IR GP2Y0A41SK0F](https://www.pololu.com/product/2464)
Vcc 5V, Gnd, Analog data is A6 aka DAC

[ADXL362 Accelerometer](https://www.sparkfun.com/products/11446)
Vcc 3.3V, Gnd, A2 CS,  A3 SCL, A4 SDO, A5 SDA)

[Adafruit ultimate GPS breakout](https://www.adafruit.com/product/746)
Vcc 5V (Vin) if you want the GPS to be always on. Vcc 3V3 if you want the GPS to power off when going to sleep.
Gnd, Rx->Tx and Tx -> Rx.


![Pictures](https://github.com/dolphrehbein/SmartTrashcan/blob/master/trashcan1.png)
![Pictures](https://github.com/dolphrehbein/SmartTrashcan/blob/master/Circuit.png)
![Pictures](https://github.com/dolphrehbein/SmartTrashcan/blob/master/App%20screenshot.png)
