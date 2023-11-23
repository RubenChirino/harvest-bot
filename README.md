# 🌱 Harvest Bot 🪴

This repository contains code for an Automated Watering System Bot, designed to facilitate remote monitoring and control of plant watering via a Telegram Bot interface. The system utilizes an ESP32 microcontroller, interfaced with soil moisture and light level sensors, and controls a mini water pump through a relay. The bot provides a range of commands for real-time data acquisition from sensors and the setting of watering schedules and reminders, enhancing the ease and effectiveness of plant care.

## Components 

- ESP32 (Microcontroller)
- Capacitive Soil Moisture Sensor
- LDR Photoresistor
- Mini Submersible Motor Water Pump (120l/h 3-6v)
- Relay (1 Channel 5v 10a)
- Battery 9V and Adapter.
- Breadboard
- Resistor
- Cables for Connections

### Connections

![image](https://github.com/RubenChirino/harvest-bot/assets/52714843/630fa9b2-3a95-4419-9262-07611ba960a7)

![setup](https://github.com/RubenChirino/harvest-bot/assets/52714843/6ec81763-2d2b-4af6-b7d2-f6ec871ea18d)

## Getting Started 

To get the system up and running, follow these steps:

1. [Install Arduino IDE](https://www.arduino.cc/en/software)
2. Connect the Microcontroller (Connect your ESP32 to your computer using a USB cable)
3. Configure the Arduino IDE (Open the Arduino IDE and select the correct board (ESP32) from the Tools menu)
4. Install Required Libraries (You can do this via the Library Manager in the Arduino IDE or find the library's ZIP and add/install it manually):
   * WiFi
   * WiFiClientSecure
   * NTPClient
   * UniversalTelegramBot
   * ArduinoJson
5. Set up WiFi Connection: Replace the `ssid` and `password` variables in the code with your WiFi network's credentials.
6. Configure Telegram Bot Token (Replace the `BOTtoken` variable in the code with the **PRIVATE KEY** of your Telegram Bot to connect with it.
7. Add Your User ID to Whitelist: Ensure you are authorized to send commands to your bot by adding your Telegram User ID to the `authorizedChatIDs` array in the ESP32 code. Replace the placeholder IDs with your own User ID, which you can obtain by messaging the IDBot on Telegram.

If you don't have a *Telegram Bot* yet you can follow these steps:

1. Start a Chat with BotFather: BotFather is the official bot by Telegram that allows you to create and manage bots. Open Telegram and search for "[@BotFather](https://t.me/BotFather)".
2. Create a New Bot: Send the `/newbot` command to BotFather. Follow the prompts to set a name and a username for your bot.
3. Save the API Token: After the bot is created, BotFather will provide you with an API token. This is essential for your bot's integration with the ESP32 code. Save this token securely.
4. Configure the Bot in Code: Replace the BOTtoken variable in your ESP32 code with the new API token you received from BotFather.

If you don't know your *Telegram User ID* you can follow these steps:

1. Message the IDBot: Go to Telegram and search for the "[@userinfobot](https://t.me/myidbot)".
2. Get Your User ID: Send `/getid` to IDBot. It will respond with your User ID.
3. Add Your ID to the Whitelist: In the ESP32 code, replace the existing values in the `authorizedChatIDs` array with your User ID. For example, if your User ID is `12345678`, change `authorizedChatIDs` to `{ 12345678 }`.

## Commands Table

| Command                               | Description                                                        | Parameter                                |
| ------------------------------------- | ------------------------------------------------------------------ | ---------------------------------------- |
| `/start`                              | Welcome message and command list.                                  | N/A                                      |
| `/set_recurring_watering_interval`    | Sets automatic watering interval (e.g., 5 for every 5 hours).      | `hours` (int)                            |
| `/stop_recurring_watering_interval`   | Stops the automatic watering system.                               | N/A                                      |
| `/set_watering_schedule`              | Sets a specific watering schedule (e.g., Monday,Wednesday,Friday 18:00). | `Days` (string), `Time` (HH:MM)          |
| `/stop_watering_schedule`             | Cancels the current watering schedule.                             | N/A                                      |
| `/set_reminder_schedule`              | Sets a specific reminder schedule (e.g., Monday,Thursday 08:00 Water the orchids). | `Days` (string), `Time` (HH:MM), `Text` (string) |
| `/stop_reminder_schedule`             | Cancels the current reminder schedule.                             | N/A                                      |
| `/schedules_state`                    | Gets the active schedules and their remaining times.               | N/A                                      |
| `/soil_moisture`                      | Retrieves the current soil moisture level.                         | N/A                                      |
| `/light_level`                        | Reports the current light level detected by the LDR sensor.       | N/A                                      |
| `/led_on`                             | Turns the LED (or another GPIO) ON.                                | N/A                                      |
| `/led_off`                            | Turns the LED (or another GPIO) OFF.                               | N/A                                      |
| `/led_state`                          | Requests the current state of the LED (or another GPIO).          | N/A                                      |

## Block Diagram about the use case flows

![image](https://github.com/RubenChirino/harvest-bot/assets/52714843/35983238-a706-44cd-b3d7-1c7bcf16457f)

![image](https://github.com/RubenChirino/harvest-bot/assets/52714843/30701fe0-0bed-42a4-a4f4-819e28b182c1)


To see it in action you can go to the [use-cases.md](https://github.com/RubenChirino/harvest-bot/blob/main/use-cases.md) file.

