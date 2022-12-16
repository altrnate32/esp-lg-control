## ******* State of main:  *******
Current state of main: tested and ok  
Tested for a couple of weeks seems pretty stable  
Remains experimental (so use at your own risk) but functions good in my setup  
## *******************************
## tip:
If you are not using an external thermostat/pump/backup_heater activate (uncomment) the dummy switches in the code.  
And comment out originals.  
Then click "Thermostat On" to start the script
switch:  
  - platform: template  
    name: "Dummy heat relay"  
    id: "relay_heat"  
    optimistic: true  
  - platform: template  
    name: "Dummy external pump relay"  
    id: "relay_heat"  
    optimistic: true  
  - platform: template  
    name: "Dummy backup_heat relay"  
    id: "relay_backup_heat"  
    optimistic: true  
  - platform: template  
    name: "Termostat On"  
    id: thermostat_signal  
    optimistic: true  

## esp-lg-control -> Hobby project!
ESP8266 based controller for LG Therma V Monoblock Unit.
Connects the LG to HomeAssistant and tries to optimise output modulation.
Tested with 3-phase 14kW U34 version. So propably works at least with 12-14-16kW U34 3-phase models. May also work with 1-phase and U44 models (depending on modbus registers). But not tested.

This is a hobby project. I share this to allow other people to pick up ideas and contribute. All assistance is welcome. I will not provide support, so use at your own risk. And make sure you know what you are doing, by using this script it is possible that your unit stops functioning or breaks and you could electrocute yourself.

## Whats new:
Update: Changed back to monitoring supply temp, after much tweaking found out this works better as modulation is more agressive
Update: Rewrote code to simple finite state machine. Gives more stability and is easier to debug
Update: Changed ant-pendel algoritm to monitor return temp. After much experimenting I have the feeling that the LG controller modulates on the return temp, not the supply temp.
Update: Ditched the PID controller. Switched back to an (improved) basic algorithm.

~~Experimental (simple) PID controller. It works, but I am not sure if it really addes value compared to the simple algoritm. It fixes some issues (with HP not starting, because target is set too low). But there are other ways to fix that.
But it is a lot of fun to experiment with.~~

## Known limitations/ToDo:
* Further testing on different models needed

## Hardware
Works with any ESP chip/board supported by ESPHome.

In this specific config two relays are connected to GPIO Pins. These relays actuate the external thermostat connections on the LG. This config is optional, but the actuation of the relays is used to determine operation mode.

An Eneco Toon is connectend to a GPIO Pin (with pulldown resistor). The Toon Ketel Module connects this pin to a 3.3V supply to create a digital 'high' when the thermostat requests heat. This config is optional.

Modbus communication is done through a rs485 to TTL converter based on the max485 chip. I use a board with automatic flow control. You can use other boards if you like. 

## Installation
* Clone repo
* Copy config/config-example.yaml to config/config.yaml
* Update config.yaml
* Update secrets.yaml
* Adapt to your own setup als needed
* Build

## Disclaimer
This is an experimental script and in early alpha stage. This is in no way finished software and should only be used by professionals. You are using this on your own riks. Do not use if you don't have any soldering/programming experience. Pull request are welcome! 
