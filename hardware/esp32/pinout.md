# ESP32 Pinout (ESP32-WROOM-32)

Pin assignments used by this project. All GPIOs are configurable in
`include/config.h`.

## Signal connections

- GPIO 18 → Fan PWM (LEDC channel, 2000 Hz, 10-bit)
- GPIO 19 → DHT22 data pin (bit-banged, requires 4.7 kΩ pull-up)

## Power

- 3.3 V → ESP32 output — do NOT power the fan from this pin
- 5 V → External supply for the fan
- GND → Common ground with fan supply

## Notes

- DHT22 needs a **4.7 kΩ pull-up** on the data line to 3.3 V (may be built
  into the module).
- Fan is powered by external 5 V supply — GPIO18 drives a MOSFET gate, not
  the fan directly.
- See wiring.md for the full connection diagram.
