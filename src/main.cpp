#include <Arduino.h>
#include <IRremote.hpp>
#include <EEPROM.h>

#define receiver_pin 2
#define sender_pin 3
#define btn_1 12
#define btn_2 9
#define btn_3 6

#define debug 1

//  Lumien: 
#define send_address 0x4040
#define delay_addr 0
uint16_t send_delay;

#define BTN_POWER 0xA
#define BTN_OK 0xD
#define BTN_LEFT 0x10
#define BTN_VOLUME 0xF
#define BTN_HOME 0x1A
#define BTN_BACK 0x40
#define BTN_PC 0x33

const uint16_t TURN_OFF[] = {BTN_POWER, BTN_LEFT, BTN_OK};

struct Buttons {
  bool _1;
  bool _2;
  bool _3;
  bool _1_proc;
  bool _2_proc;
  bool _3_proc;
};


void check_receiver() {
  if (IrReceiver.decode()) {
    // Игнорируем флаги повтора зажатой кнопки
    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
        IRData data = IrReceiver.decodedIRData;
        
        Serial.print(F("Command receiver: protocol -- "));
        Serial.print(IrReceiver.getProtocolString());
        Serial.print(F(" | address -- 0x"));
        Serial.print(data.address, HEX);
        Serial.print(F(" | command -- 0x"));
        Serial.println(data.command, HEX);
      }
    }
    IrReceiver.resume();
}


void check_serial() {
  if (Serial.available() > 0) {
    // Ждем не больше 50 мс, чтобы loop() не зависал надолго,
    // если терминал телефона не шлет символ перевода строки.
    Serial.setTimeout(20);

    // Считываем всё до '\n' или пока не истечет таймаут
    String input = Serial.readStringUntil('\n');

    // Очищаем от пробелов, \r, лишних переводов строк по краям
    input.trim();

    // Если ничего осмысленного не пришло — выходим
    if (input.length() == 0) {
      return;
    }

    // Ищем разделитель ':'
    int colon_idx = input.indexOf(':');

    if (colon_idx != -1) {
      String cmd = input.substring(0, colon_idx);
      String val_str = input.substring(colon_idx + 1);

      cmd.trim();
      val_str.trim();

      // === Обработка команд ===

      if (cmd.equalsIgnoreCase("set_delay")) {
        // strtoul сам поймет 50 или 0x32
        char *end_ptr;
        unsigned long val = strtoul(val_str.c_str(), &end_ptr, 0);

        if (end_ptr != val_str.c_str() && val <= 65535) {
          send_delay = (uint16_t)val;
          EEPROM.put(delay_addr, send_delay);

          Serial.print(F("Delay "));
          Serial.print(send_delay);
          Serial.println(F(" has successfully saved."));
        } else {
          Serial.println(F("Error: invalid delay value (0-65535)."));
        }
      } 
      else if (cmd.equalsIgnoreCase("set_byte")) {
        // Пример для байта
      }
      else {
        Serial.print(F("Wrong command: '"));
        Serial.print(cmd);
        Serial.println(F("'"));
      }

    } else {
      Serial.println(F("Error: missing ':' in command."));
    }
  }
}


void check_buttons(Buttons& bts) {
  if (!digitalRead(btn_1)) {
    bts._1 = true;
  } else {
    bts._1 = false;
    bts._1_proc = false;
  }

  if (!digitalRead(btn_2)) {
    bts._2 = true;
  } else {
    bts._2 = false;
    bts._2_proc = false;
  }

  if (!digitalRead(btn_3)) {
    bts._3 = true;
  } else {
    bts._3 = false;
    bts._3_proc = false;
  }
}


void send_command(const uint16_t& command) {
  IrReceiver.stop();
  IrSender.sendNEC(send_address, command, 0);
  delay(5);
  IrReceiver.start();
}


void setup() {
  init(); //надо, чтобы ардуинка завелась, встроенная функция
  Serial.begin(115200); //инициализия com порта
  IrReceiver.begin(receiver_pin, ENABLE_LED_FEEDBACK);
  IrSender.begin(sender_pin);

  pinMode(btn_1, INPUT_PULLUP);
  pinMode(btn_1 - 2, OUTPUT);
  digitalWrite(btn_1 - 2, LOW);

  pinMode(btn_2, INPUT_PULLUP);
  pinMode(btn_2 - 2, OUTPUT);
  digitalWrite(btn_2 - 2, LOW);

  pinMode(btn_3, INPUT_PULLUP);
  pinMode(btn_3 - 2, OUTPUT);
  digitalWrite(btn_3 - 2, LOW);

  EEPROM.get(delay_addr, send_delay);
  Serial.print(F("Saved in memory delay: ")); Serial.println(send_delay);
}

int main() {
  setup();
  Buttons buttons;
  uint32_t last_bt_time = millis();
  uint32_t last_ser_time = millis();
  uint32_t last_res_time = millis();

  while(true) {
    if(millis() - last_bt_time > 30) {
    check_buttons(buttons);

    if(buttons._1 && !buttons._1_proc) {
      uint8_t count = sizeof(TURN_OFF) / sizeof(TURN_OFF[0]);
      for(uint8_t i = 0; i < count; i++) {
        send_command(TURN_OFF[i]);
        if(count - i > 1) delay(send_delay);
      }
      if(debug) Serial.println(F("Turn_off command has sent"));
      buttons._1_proc = true;
      IrReceiver.resume();
    }
    
    if(buttons._2 && !buttons._2_proc) {
      send_command(BTN_HOME);
      if(debug) Serial.println(F("Home command has sent"));
      buttons._2_proc = true;
      IrReceiver.resume();
    }

    if(buttons._3 && !buttons._3_proc) {
      send_command(BTN_VOLUME);
      if(debug) Serial.println(F("Volume command has sent"));
      buttons._3_proc = true;
      IrReceiver.resume();
    }

    last_bt_time = millis();
  }

    if(millis() - last_ser_time > 100) {
      check_serial();

      last_ser_time = millis();
    }

    if(millis() - last_res_time > 25) {
      check_receiver();

      last_res_time = millis();
    }
  }
}