# EnvMonitor

STM32 Nucleo-F401RE environmental monitor.

Reads a BMP280 (temperature, pressure, humidity) over I2C, shows values on an SSD1306 OLED, and prints them over UART. Also uses ADC, PWM, a button/LED, and a 74HC595 shift register.

Open `EnvMonitor.ioc` or the `.project` file in STM32CubeIDE to build and flash.
