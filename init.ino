#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// -------- SENSORS ------
#define SENSOR_HUMIDITY_PIN 36

// -------- Wi-Fi Details ----------
const char* ssid = "XXX";
const char* password = "XXX";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// ------- Telegram Bot Token -----
#define BOTtoken "XXX"

// Authorized Chats ID
const long authorizedChatIDs[] = { 1234567891, 1234567892 };
const int authorizedChatsCount = sizeof(authorizedChatIDs) / sizeof(authorizedChatIDs[0]);

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

// ------- Globals Variables -------
// Checks for new messages every 1 second
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

// Harvest
unsigned long lastWatering = 0;
unsigned long wateringInterval = 0; // Miliseconds
bool isRecurringWateringEnabled = false;

struct ScheduleStructure {
  bool days[7]; // Sunday = 0, Monday = 1, Tuesday = 2, ..., Saturday = 6
  int hour; // 24 Hours Format
  int minute;
  String text;
  bool alreadyExecuted = false;
  int lastDayChecked = -1;
};

ScheduleStructure wateringSchedule;
ScheduleStructure reminderSchedule;

const char* daysNames[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

bool isAwaitingConfirmation = false;
String pendingScheduleType = "";
String pendingScheduleCommand = "";
String pendingReminderText = "";

// ESP32 Led
const int ledPin = 2;
bool ledState = LOW;

void setup() {
  Serial.begin(9600);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);

  connectToWifi();

  // Init NTP Client
  timeClient.begin();
  // Setting the time zone to UTC-3 for Buenos Aires, ARG
  timeClient.setTimeOffset(-10800);
}

void loop() {
  checkNewMessages();

  // Watering Triggers
  if (checkWateringRecurring()) {
    handleWatering();
    lastWatering = millis();
  }
  
  if (checkSchedule(wateringSchedule)) {
    handleWatering();
  }
  // ---------------

  if (checkSchedule(reminderSchedule)) {
    sendReminder(reminderSchedule.text);
  }
}

//  ----- Functions -----

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("");
  Serial.println("WiFi connected ✅");
  Serial.print("Local IP Address: ");
  Serial.print(WiFi.localIP());
  Serial.println("");
}

void handleNewMessages(int numNewMessages) {
  Serial.println("");
  Serial.println("LOG: New Message 📲💬");
  Serial.println("Quantity: " + String(numNewMessages));

  String message = "";

  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    if (!isAuthorizedUser(bot.messages[i].chat_id.toInt())) {
      message = "🚫 Oops! It looks like you don't have permission to interact with me. If you believe this is a mistake, please contact the administrator. 🌼";
      bot.sendMessage(chat_id, message, "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println("Message: " + text);

    String userName = bot.messages[i].from_name;

    // ---- Confirmations -----
    if (isAwaitingConfirmation && pendingScheduleType != "" && pendingScheduleCommand != "") {
        if (text.equalsIgnoreCase("yes")) {
            
            if (pendingScheduleType == "watering") {
                cleanSchedule(wateringSchedule);
                message = setSchedule(pendingScheduleCommand, wateringSchedule);
            } else if (pendingScheduleType == "reminder") {
                cleanSchedule(reminderSchedule);
                message = setSchedule(pendingScheduleCommand, reminderSchedule);
                if (!isErrorMessage(message)) {
                  reminderSchedule.text = pendingReminderText;
                }
            }
            isAwaitingConfirmation = false;
        } else {
            message = "Schedule update canceled.";
            isAwaitingConfirmation = false;
        }
    }

    // ---- Commands 
    String startCommand = "/start";
    String setRecurringWateringCommand = "/set_recurring_watering_interval ";
    String stopRecurringWateringCommand = "/stop_recurring_watering_interval";
    String setWateringScheduleCommand = "/set_watering_schedule ";
    String setReminderScheduleCommand = "/set_reminder_schedule ";
    String getSoilMoistureCommand = "/soil_moisture";
    String setLedOnCommand = "/led_on";
    String setLedOffCommand = "/led_off";
    String ledStateCommand = "/led_state";
    String schedulesStateCommand = "/schedules_state";
    String stopWateringScheduleCommand = "/stop_watering_schedule";
    String stopReminderScheduleCommand = "/stop_reminder_schedule";

    // ---- Welcome -----
    if (text == startCommand && message == "") {
        String welcome = "🌱🤖 Welcome to Harvest Guardian, " + userName + "! 🌿🌞\n\n";
        welcome += "I'm here to help you automate and monitor your *plant watering and care system*. Let's make your gardening journey easier and more fun! 🌼💧🌻\n\n";
        welcome += "Here are some commands you can use to control your outputs and watering system:\n\n";
        // Commands
        welcome += "*Actions*\n";
        welcome += setRecurringWateringCommand + "[hours] - Sets the automatic watering interval. For example, 5 will start watering every five hours. ⏱️🌊\n";
        welcome += stopRecurringWateringCommand + " - Stops the automatic watering system. 🛑💧\n";
        welcome += setWateringScheduleCommand + "[Days] [Time] - Sets a specific watering schedule. For example, Monday,Wednesday,Friday 18:00 will set the watering at 6:00 PM on these days. 📅⏰\n";
        welcome += stopWateringScheduleCommand + " - Cancels the current watering schedule. 🚫💧\n";
        welcome += setReminderScheduleCommand + "[Days] [Time] [Text] - Sets a specific reminder schedule. For example, Monday,Thursday 08:00 Water the orchids will remind you to water the orchids at 8:00 AM on Mondays and Thursdays. 📆🌸🕗\n";
        welcome += stopReminderScheduleCommand + " - Cancels the current reminder schedule. 🚫🔔\n\n";
        welcome += "*Records*\n";
        welcome += schedulesStateCommand + " - Get the active schedules and the remaining times to execute them.\n";
        welcome += getSoilMoistureCommand + " - Get the current soil moisture level.\n\n";
        // Test Commands
        /* welcome += setLedOnCommand + " - Turns the LED ON. 💡🔛\n";
        welcome += setLedOffCommand + " - Turns the LED OFF. 💡🔚\n";
        welcome += ledStateCommand + " - Requests the current state of the LED. 🔍💡\n"; */
        welcome += "Feel free to reach out with these commands anytime you need. Let's keep your plants thriving together! 🌺🌵🌼\n";
        message = welcome;
    }

    // ---- Actions -----
    if (text.startsWith(setRecurringWateringCommand) && message == "") {
        String intervalStr = text.substring(setRecurringWateringCommand.length());
        Serial.println("Hours: " + intervalStr);
        int intervalHours = intervalStr.toInt();
        if (intervalHours < 12) {
            message = "🌼 Oops! The minimum watering interval is 12 hours to ensure your plants aren't overwatered. Let's give them just the right amount of water! 💧";
        } else {
            wateringInterval = intervalHours * 3600000; // Hours to Milliseconds
            message = "🌱 You've set the watering interval to every " + intervalStr + " hour(s).\nI'll make sure your plants stay hydrated just right! 🕒";
            isRecurringWateringEnabled = true;
            lastWatering = millis(); // Reset the counter
        }

        if (isScheduleSet(wateringSchedule)) {
          cleanSchedule(wateringSchedule);
          message += "\n🌼 Just a heads-up: I've turned off the scheduled watering to focus on this new interval. Let's keep those plants happy!";
        }
    }

    if (text == stopRecurringWateringCommand && message == "") {
        if (isRecurringWateringEnabled) {
            isRecurringWateringEnabled = false;
            message = "🌿 The automatic watering interval has been stopped.\nYour plants will now rely on your personal touch for their hydration needs! 💦";
        } else {
            message = "🤔 It seems like there's no active watering interval to stop.\nTo set one up, use '/set_recurring_watering_interval [hours]'. Let's keep those plants happy! 🌱";
        }
    }

    if (text == getSoilMoistureCommand && message == "") {
        float soilMoisture = getSoilMoisture();
        message = "🌱 The current soil moisture level is at " + String(soilMoisture) + "%.\n\nKeep an eye on it to ensure your plants are getting just the right amount of water! 💧🌼";
    }

    if (text.startsWith(setWateringScheduleCommand) && message == "") {
      int commandLength = setWateringScheduleCommand.length();
      if (isScheduleSet(wateringSchedule)) {
          isAwaitingConfirmation = true;
          pendingScheduleType = "watering";
          pendingScheduleCommand = text.substring(commandLength);
          message = "You already have a watering schedule set. Do you want to replace it? Send 'yes' to confirm.";
      } else {
          cleanSchedule(wateringSchedule);
          message = setSchedule(text.substring(commandLength), wateringSchedule);
      }
    } 

    if (text == stopWateringScheduleCommand && message == "") {
      if (isScheduleSet(wateringSchedule)) {
          cleanSchedule(wateringSchedule);
          message = "🌿 Watering schedule has been successfully stopped. Your plants will now rely on your manual care. 💧";
      } else {
          message = "🤔 It looks like there's no watering schedule to stop.";
      }
    }
    
    if (text.startsWith(setReminderScheduleCommand) && message == "") {
        int commandLength = setReminderScheduleCommand.length();
        int firstSpaceIndex = text.indexOf(' ', commandLength);
        int secondSpaceIndex = text.indexOf(' ', firstSpaceIndex + 1);
        String scheduleCommandPart = text.substring(commandLength, secondSpaceIndex);
        if (isScheduleSet(reminderSchedule)) {
            isAwaitingConfirmation = true;
            pendingScheduleType = "reminder";
            pendingScheduleCommand = scheduleCommandPart;
            pendingReminderText = text.substring(secondSpaceIndex + 1);
            message = "You already have a reminder schedule set. Do you want to replace it? Send 'yes' to confirm.";
        } else {
            cleanSchedule(reminderSchedule);
            message = setSchedule(scheduleCommandPart, reminderSchedule);
            if (!isErrorMessage(message)) {
                reminderSchedule.text = text.substring(secondSpaceIndex + 1);
            }
        }
    }

    if (text == stopReminderScheduleCommand && message == "") {
      if (isScheduleSet(reminderSchedule)) {
          cleanSchedule(reminderSchedule);
          message = "🌿 Reminder schedule has been successfully stopped. Don't forget to check on your plants! 🌱";
      } else {
          message = "🤔 It looks like there's no reminder schedule to stop.";
      }
    }

    if (text == schedulesStateCommand && message == "") {
        message = "🌿 Here are your current schedules:\n\n";

        // LOG line
        printCurrentDateTime();

        if (isScheduleSet(wateringSchedule)) {
            message = "🌿 *Watering Schedule:*\n\n";
            message += "Days: " + getScheduleDays(wateringSchedule) + "\n";
            message += "Time: " + getScheduleFormattedTime(wateringSchedule) + "\n";
            message += "Remaining: " + getRemainingTimeMessage(wateringSchedule) + "\n\n";
        } else {
            message += "No watering schedule set.\n\n";
        }

        if (isScheduleSet(reminderSchedule)) {
            message += "🌿 *Reminder Schedule:*\n";
            message += "Days: " + getScheduleDays(reminderSchedule) + "\n";
            message += "Time: " + getScheduleFormattedTime(reminderSchedule) + "\n";
            message += "Remaining: " + getRemainingTimeMessage(reminderSchedule) + "\n\n";
        } else {
            message += "No reminder schedule set.\n\n";
        }

        message += "Keep an eye on your schedules to stay on top of your gardening! 🌱💦";
    }

    // ---- Test Commands ----
    if (text == setLedOnCommand && message == "") {
      message = "LED state set to ON";
      ledState = HIGH;
      digitalWrite(ledPin, ledState);
    }

    if (text == setLedOffCommand && message == "") {
      message = "LED state set to OFF";
      ledState = LOW;
      digitalWrite(ledPin, ledState);
    }

    if (text == ledStateCommand && message == "") {
      if (digitalRead(ledPin)){
        message = "LED is ON";
      } else {
        message = "LED is OFF";
      }
    }
    // --------

    if (message == "") {
      bot.sendMessage(chat_id, "Oops! It looks like that command doesn't exist.\n\n🤔 If you need help, type '" + startCommand + "' to see all available commands. I'm here to assist you with your garden! 🌼🌿", "");
      continue;
    } 

    bool isMessageSent = bot.sendMessage(chat_id, message, "");

    if (isMessageSent) {
      Serial.println("Result: Message sent successfully ✅");
    } else {
      Serial.println("Result: Failed to send message ❌");
    }
  }
  
  Serial.println("");
}

void handleWatering() {
    Serial.println("Doing watering...");
    // digitalWrite(9, HIGH);
    Serial.println("");
    delay(10000); // Watering Time
    // digitalWrite(9, LOW);
    // isRecurringWateringEnabled = false;
}

void checkNewMessages() {
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}

bool checkWateringRecurring() {
  return isRecurringWateringEnabled && millis() - lastWatering >= wateringInterval;
}

bool checkSchedule(ScheduleStructure& schedule) {
    timeClient.update();
    int currentDay = timeClient.getDay();
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();

    // Check if it's a new day and reset the flags
    if (!schedule.lastDayChecked || schedule.lastDayChecked != currentDay) {
        schedule.alreadyExecuted = false;
        schedule.lastDayChecked = currentDay;
    }

    // Check if the schedule is active for the current day
    if (schedule.days[currentDay] && !schedule.alreadyExecuted) {
        // If the current time is after the set schedule and the schedule has not yet run today
        if (currentHour > schedule.hour || (currentHour == schedule.hour && currentMinute >= schedule.minute)) {
            schedule.alreadyExecuted = true;
            return true;
        }
    }

    return false;
}

// ------ Actions -----

void sendReminder(String text) {
  // Send the reminder to all the Authorized users
  for (int i = 0; i < authorizedChatsCount; i++) {
    bot.sendMessage(String(authorizedChatIDs[i]), text, "");
  }
}

void cleanSchedule(ScheduleStructure& schedule) {
  memset(schedule.days, 0, sizeof(schedule.days));
  schedule.hour = 0;
  schedule.minute = 0;
  schedule.text = "";
  schedule.alreadyExecuted = false;
  schedule.lastDayChecked = -1;
  pendingScheduleType = "";
  pendingScheduleCommand = "";
}

// ----- Getters & Setters -----

float getSoilMoisture() {
  int sensorValue = analogRead(SENSOR_HUMIDITY_PIN); 
  float humidity = (sensorValue / 4095.0) * 100.0;
  return humidity;
}

String getScheduleDays(const ScheduleStructure& schedule) {
    String daysStr = "";
    for (int i = 0; i < 7; i++) {
        if (schedule.days[i]) {
            if (daysStr != "") daysStr += ", ";
            daysStr += daysNames[i];
        }
    }
    return daysStr == "" ? "No days set" : daysStr;
}

String getScheduleFormattedTime(const ScheduleStructure& schedule) {
    String formattedHour = (schedule.hour < 10) ? "0" + String(schedule.hour) : String(schedule.hour);
    String formattedMinute = (schedule.minute < 10) ? "0" + String(schedule.minute) : String(schedule.minute);
    return formattedHour + ":" + formattedMinute;
}

int getRemainingTimeForNearestDay(const ScheduleStructure& schedule) {
    timeClient.update();
    int currentDay = timeClient.getDay();
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();

    // LOG INFO
    printScheduleInfo(schedule);

    // If the schedule for the current day has already run, finds the next scheduled day
    if(schedule.alreadyExecuted && schedule.lastDayChecked == currentDay) {
        for(int i = 1; i <= 7; i++) {
            int nextDayIndex = (currentDay + i) % 7;
            if(schedule.days[nextDayIndex]) {
                int daysUntilNext = i;
                int hoursUntilNext = 24 * daysUntilNext + schedule.hour - currentHour;
                int minutesUntilNext = schedule.minute - currentMinute;
                if (minutesUntilNext < 0) {
                    hoursUntilNext--;
                    minutesUntilNext += 60;
                }
                return hoursUntilNext * 60 + minutesUntilNext;
            }
        }
    }
    // If the schedule for the current day has not been executed yet
    else {
        int hoursDiff = schedule.hour - currentHour;
        int minutesDiff = schedule.minute - currentMinute;
        if (minutesDiff < 0) {
            hoursDiff--;
            minutesDiff += 60;
        }
        return hoursDiff * 60 + minutesDiff;
    }

    return -1; // Error calculating the remaining time
}

String getRemainingTimeMessage(const ScheduleStructure& schedule) {
    int totalMinutes = getRemainingTimeForNearestDay(schedule);

    if(totalMinutes < 0) {
        return "Error calculating remaining time";
    }

    int hours = totalMinutes / 60;
    int minutes = totalMinutes % 60;

    String remainingTime = "";
    if (hours > 0) remainingTime += String(hours) + " hour(s) ";
    if (minutes > 0) remainingTime += String(minutes) + " minute(s)";
    return remainingTime == "" ? "Less than a minute" : remainingTime;
}

String setSchedule(String scheduleCommand, ScheduleStructure& schedule) {
  int spaceIndex = scheduleCommand.indexOf(' ');
  String daysPart = scheduleCommand.substring(0, spaceIndex);
  String timePart = scheduleCommand.substring(spaceIndex + 1);

  // LOG
  Serial.println("----- Schedule Command Info -----");
  Serial.print("spaceIndex: ");
  Serial.print(spaceIndex);

  Serial.println("");
  Serial.print("daysPart: ");
  Serial.print(daysPart);

  Serial.println("");
  Serial.print("timePart: ");
  Serial.print(timePart);

  Serial.println("");
  Serial.print("scheduleCommand: ");
  Serial.print(scheduleCommand);
  Serial.println("");

  bool invalidDayFound = false;
  String invalidDays = "";

  // Config the days
  int startIndex = 0;
  int endIndex = daysPart.indexOf(',', startIndex);
  while (endIndex != -1) {
    String day = daysPart.substring(startIndex, endIndex);
    day.trim();
    int dayIndex = dayOfWeekToIndex(day);
    if (dayIndex == -1) {
      invalidDayFound = true;
      invalidDays += day + ", ";
    } else {
      schedule.days[dayIndex] = true;
    }
    startIndex = endIndex + 1;
    endIndex = daysPart.indexOf(',', startIndex);
  }

  // Last day or unique day
  String lastDay = daysPart.substring(startIndex);
  lastDay.trim();
  int lastDayIndex = dayOfWeekToIndex(lastDay);
  if (lastDayIndex == -1) {
    invalidDayFound = true;
    invalidDays += lastDay;
  } else {
    schedule.days[lastDayIndex] = true;
  }

  if (invalidDayFound) {
    return "Oops! The following day(s) are not valid: " + invalidDays + ".\nPlease use correct day names. 📆";
  }

  // Config the hour and minutes
  int colonIndex = timePart.indexOf(':');
  int hour, minute;

  if (colonIndex == -1) {
      hour = timePart.toInt();
      minute = 0;
  } else {
      hour = timePart.substring(0, colonIndex).toInt();
      minute = timePart.substring(colonIndex + 1).toInt();
  }

  // Validate hour and minutes
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    return "Oops! It looks like you've entered an invalid time. Remember, the hour should be between 0 and 23, and minutes between 0 and 59.\nPlease try setting the schedule again.";
  }

  schedule.hour = hour;
  schedule.minute = minute;
  schedule.lastDayChecked = -1;

  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int currentDay = timeClient.getDay();

  // If the current time is later than the scheduled time and the scheduled day is the same current day
  if (schedule.days[currentDay] && 
           (schedule.hour < currentHour || (schedule.hour == currentHour && schedule.minute <= currentMinute)) ) {
     schedule.alreadyExecuted = true;
     schedule.lastDayChecked = currentDay;
  } 
  // If the set time is for a future time on the same day
  // make sure it is not marked as already executed
  /* else if(schedule.days[currentDay] && 
     (schedule.hour > currentHour || (schedule.hour == currentHour && schedule.minute > currentMinute))) {
      schedule.alreadyExecuted = false;
  } */
  // For all other cases (future schedules on future days)
  /* else {
      schedule.alreadyExecuted = false;
  } */

  // LOG line
  printScheduleInfo(schedule);

  return "🌿✨ Schedule successfully set to: " + getScheduleDays(schedule) + " at " + getScheduleFormattedTime(schedule) + ". I'll make sure to remind you on time! ⏰🌷";
}

int dayOfWeekToIndex(String day) {
  day.toLowerCase();
  if (day == "sunday") return 0;
  if (day == "monday") return 1;
  if (day == "tuesday") return 2;
  if (day == "wednesday") return 3;
  if (day == "thursday") return 4;
  if (day == "friday") return 5;
  if (day == "saturday") return 6;
  return -1; // Invalid day
}

bool isAuthorizedUser(long chatID) {
  for (int i = 0; i < authorizedChatsCount; i++) {
    if (authorizedChatIDs[i] == chatID) {
      return true;
    }
  }
  return false;
}

bool isScheduleSet(const ScheduleStructure& schedule) {
    for (int i = 0; i < 7; i++) {
        if (schedule.days[i]) return true;
    }
    return false; // No days set means no schedule is set
}

bool isErrorMessage(String msg) {
  return msg.indexOf("Oops") != -1;
}

// ----- Debug & Loggers Methods -----

void printScheduleInfo(const ScheduleStructure& schedule) {
    Serial.println("----- Schedule Info -----");
    Serial.print("Days: ");
    for (int i = 0; i < 7; i++) {
        if (schedule.days[i]) {
            Serial.print(daysNames[i]);
            Serial.print(" ");
        }
    }

    Serial.println("");
    Serial.print("Hour: ");
    Serial.print(schedule.hour);

    Serial.println("");
    Serial.print("Minute: ");
    Serial.print(schedule.minute);

    Serial.println("");
    Serial.print("Text: ");
    Serial.print(schedule.text);

    Serial.println("");
    Serial.print("Already Executed: ");
    Serial.print(schedule.alreadyExecuted);

    Serial.println("");
    Serial.print("Last Day Checked: ");
    Serial.print(schedule.lastDayChecked);

    Serial.println("");
    Serial.println("----------");
}

void printCurrentDateTime() {
    Serial.println("-- Current Date and Time Info --");
    timeClient.update();
    int dayOfWeek = timeClient.getDay();
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    int currentSecond = timeClient.getSeconds();

    const char* daysOfWeek[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

    Serial.print("Day of week: ");
    Serial.println(daysOfWeek[dayOfWeek]);

    Serial.print("Current time: ");
    if (currentHour < 10) Serial.print("0");
    Serial.print(currentHour);
    Serial.print(":");
    if (currentMinute < 10) Serial.print("0");
    Serial.print(currentMinute);
    Serial.print(":");
    if (currentSecond < 10) Serial.print("0");
    Serial.println(currentSecond);
    Serial.println("----------");
}

void printScheduleDays(const ScheduleStructure& schedule) {
    Serial.println("Schedule days:");
    for (int i = 0; i < 7; i++) {
        Serial.print(daysNames[i]);
        Serial.print(": ");
        Serial.println(schedule.days[i]);
    }
}