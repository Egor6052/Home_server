Home Server Project
---
This home server is designed for remote access, monitoring, and management. The core hardware setup consists of a Raspberry Pi 5, an STM32F401RCT6 microcontroller, a 5-port network switch, a Hikvision camera with a 12V power connector, and a 400W power supply.

---

### Paspberry Module:
- Camera streaming RTSP to https;
- Systemd services;
- UART to STM32 conect;
- Dashboard for controlling the server;


### STM32 Module:
- Controller;
- WATCHDOG system can reboot Raspberry Pi if it doesn't have a response from it;
- Physical reboot button;
- Temperature inside the server;
- Cooler controlling;
- Connect to the street controller to get the humidity and temperature data;




