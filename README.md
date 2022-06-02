# RAK11200_LoRa_RX_TX

A simple LoRa RX/TX example for the [RAK11200 ESP32 Core](https://store.rakwireless.com/products/wiscore-esp32-module-rak11200) with a Wisblock [RAK13300 LoRa SX1262 IO module](https://store.rakwireless.com/products/rak13300-wisblock-lpwan).

![Board](Board.jpg)
![Screenshot](Screenshot1.png)
![Screenshot](Screenshot2.png)
![SDRPP](SDRPP.jpg)

## UPDATE 2022/06/02

I added RNG via the LoRa sx1262 chip. The packets sent back fill up the remainder of the 64-byte packet with random bytes – just for kicks.

![Random](Random.png)
