<h1 align="center">Chameya OS</h1>
<p align="center">
  <img src="https://github.com/user-attachments/assets/133a8d94-5653-4d3e-856b-0d0bb2295cbe" alt="f">
</p>

# 1-Project description:
Chameya OS is an opreating system for the esp 8266 , managed by freeRTOS the user can interact with the hardware with my api implementation of a TFT touch display that uses software emulated SPI protocol (bit-banged) . 

the data sheet of the display's chip driver [ILI9488](https://www.hpinfotech.ro/ILI9488.pdf) and the touch controller [XPT2046](https://datasheet4u.com/datasheet/XPTEK/XPT2046-746665) .
# 2-Development environment 
I used the [esp 8266 freertos SDK](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html) with eclipse IDE to develop this project .
# 3-Hardware used
- Esp 8266 lolin v3 
- Touch Screen (ILI9488 , XPT2046) (480 * 320)  [the module i used](https://cothings.net/collections/ecran/products/ecran-daffichage-lcd-tft-avec-panneau-tactile-3-5-pouces-480x320?variant=40046657634495)
# 4-Wiring 
![Sans titre](https://github.com/user-attachments/assets/f8ef7120-aea0-4c8d-8f13-3486b035ae43)
# 5-code 
[doxygen document](https://yassine-hattay.github.io/chameya_os/index.html)
# 6- Apps
Generally in this OS every app page is it's own task that runs and self terminates when the adquate input is provided , i made 4 apps with varying numbers of pages each .

https://github.com/user-attachments/assets/ae4bb940-3f1c-4833-97c0-8f78ce4296a6

## a - note book app :
This app let's you use the esp's spiffs system for file management and creation , you can ceate new text files , view their content , modify or delete them .

https://github.com/user-attachments/assets/edb71519-3710-4950-803d-82d5a1066354


## b - pong :
This is the classic pong retro game there are 3 modes (AI vs AI , player vs AI , player vs player ) with 3 difficulty options ( easy , medium , chameya mabloula ! ) .

https://github.com/user-attachments/assets/6a872e89-2a5a-4ca8-a53e-88e021967720


## c - Goblin Slayer game :
This is my original little game where you place maximum of 10 goblins and they fight each other to the death , each goblin is a freertos task managed by the OS .

https://github.com/user-attachments/assets/2bcdbda0-124b-4dc4-b862-25a1d56341e8


## d - SPI , UART , I2C software emulated communication :
This app let's you send data with software emulated (UART , SPI , I2C) to another device (this was only tested with an esp 32) .


https://github.com/user-attachments/assets/422d69cc-9549-490c-8bc3-89e7467a9d7d



