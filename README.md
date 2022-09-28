# ESP Spa Controller
An Arduino based DIY controller for controlling a hot tub system with temperature reporting, pump and heater control, and Home Assistant MQTT based integration

# Requirements

## Hardware
This particular implementation uses a Wemos D1 Mini clone with built in ESP8266 module for WiFi connectivity

![Wemos D1 Mini Pinout](https://www.wemos.cc/en/latest/_static/boards/d1_mini_v4.0.0_5_16x9.png)

## Arduino Libraries
- [ESP8266WiFi](https://github.com/esp8266/Arduino)
- [PubSubClient](https://pubsubclient.knolleary.net)
- [OneWire](https://www.pjrc.com/teensy/td_libs_OneWire.html)
- [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library)
- [Arduino_JSON](https://github.com/arduino-libraries/Arduino_JSON)

## Home Assistant Configuration
```
mqtt:
  sensor:
  - name: "Spa Heater"
    state_topic: "homeassistant/spa/state"
    value_template: "{{ value_json.heater }}"
    
  switch:
  - name: "Spa Pump"
    state_topic: "homeassistant/spa/state"
    value_template: "{{ value_json.pump }}"
    state_on: true
    state_off: false
    command_topic: "homeassistant/spa/set"
    payload_on: '{"pump":true}'
    payload_off: '{"pump":false}'
    
  number:
  - name: "Spa Temperature"
    device_class: "temperature"
    command_topic: "homeassistant/spa/set"
    command_template: '{"tempSP": {{value}} }'
    min: 32
    max: 105
    state_topic: "homeassistant/spa/state"
    value_template: "{{ value_json.temperature }}"
    unit_of_measurement: "°F"
```