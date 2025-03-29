## ESP-LG-Control: ESP Home based Modbus LG Heatpump controller 
Control your LG Therma Heatpump through Home Assistant using an ESP32 with modbus connection.
Copyright (C) 2025 altrnate32

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.

## Hardware: Recommend to use this board:
[Github Link](https://github.com/johanneswilkens/Heat-Pump-Controller-PCB)

## State of main branch: 
Stable for over a year
Functions without issues in my setup for over a year. But remains experimental (so use at your own risk)

## Enable/Disable external thermostat/relays:
If you are not using an external thermostat/pump/backup_heater choose the correct packages in esp-lg-control.yaml (lines 9-19).  
Comment the config that you don't use and uncomment the used file. Always make sure 1 file is commented and one is uncommented per set op files.  
Example without external thermostat and without external relays:  

```yaml
  #Choose correct version of below two files. Always enable (by uncommenting) only one of the two
  #*****
  #with_external_thermostat: !include lg-heatpump-control/sensors/lg-with-external-thermostat.yaml
  without_external_thermostat: !include lg-heatpump-control/sensors/lg-without-external-thermostat.yaml
  #*****
  
  #Again Choose correct version of below two files. Always enable (by uncommenting) only one of the two
  #*****
  #with_external_relays: !include lg-heatpump-control/sensors/lg-with-external-relays.yaml 
  without_external_relays: !include lg-heatpump-control/sensors/lg-without-external-relays.yaml 
  #*****
```
Also select to use external 'feelslike' temperature or not

## esp-lg-control -> Hobby project!
ESP32 based controller for LG Therma V Monoblock Unit.
Will work on any ESPHome compatible chip, but recommend dual core ESP32
Connects the LG to HomeAssistant and tries to optimise output modulation.
Tested with 3-phase 14kW U34 version. So propably works at least with 12-14-16kW U34 3-phase models. May also work with 1-phase and U44 models (depending on modbus registers). But not tested.

This is a hobby project. I share this to allow other people to pick up ideas and contribute. All assistance is welcome. I will not provide support, so use at your own risk. And make sure you know what you are doing, by using this script it is possible that your unit stops functioning or breaks and you could electrocute yourself.

## Whats new:
* Update 13-01-2025: Added new feature to load external (HA) 'feelslike' temperature sensor. This temperature is then used (with factor 0.4) to offset stooklijn calculation. This automatically increases stooklijn value in case of high wind etc.
I use the OpenWeatherMap API
* Update 10-01-2024: Added new feature to manually or automatically disable the heatpump and run the backup heater. Usefull if you have a hybrid setup with gas/oil heater. This also enables you to create a (HA/Node-red) automation to 'manually' disable the heatpump and run the backup heater based on electricity/gas price and COP.
* Update: New control algoritm
* Update: Changed back to monitoring supply temp, after much tweaking found out this works better as modulation is more agressive
* Update: Rewrote code to simple finite state machine. Gives more stability and is easier to debug
* Update: Changed ant-pendel algoritm to monitor return temp. After much experimenting I have the feeling that the LG controller modulates on the return temp, not the supply temp.
* Update: Ditched the PID controller. Switched back to an (improved) basic algorithm.

~~Experimental (simple) PID controller. It works, but I am not sure if it really addes value compared to the simple algoritm. It fixes some issues (with HP not starting, because target is set too low). But there are other ways to fix that.
But it is a lot of fun to experiment with.~~

## Known limitations/ToDo:
* Currently none. Stable on several different LG models.

## Hardware
Works with any ESP chip/board supported by ESPHome.

**Recommend to use the board designed by johanneswilkens:** [Github Link](https://github.com/johanneswilkens/Heat-Pump-Controller-PCB)

In this specific config four relays are connected to GPIO Pins. These relays actuate the external thermostat connections on the LG. This config is optional.

A Nest external thermostat is connectend to a GPIO Pin (with pulldown resistor). The Nest Ketel Module connects this pin to a 3.3V supply to create a digital 'high' when the thermostat requests heat. This config is optional. It can be replaced by a template switch. The switch is nesscesary as this start the control algoritm.

Modbus communication is done through a rs485 to TTL converter based on the max485 chip. I use a board with automatic flow control. You can use other boards if you like. 

## Installation
* Clone repo
* Copy config/config-example.yaml to config/config.yaml
* Update config.yaml
* Update secrets.yaml
* Adapt to your own setup als needed
* Build

## Disclaimer
This is an experimental script and will remain in beta stage (even though releases may not include beta designator). This is in no way finished software and should only be used by professionals. You are using this at your own risk. Do not use if you don't have any soldering/programming experience. Pull request are welcome!
