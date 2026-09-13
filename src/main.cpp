#include <Arduino.h>
#include <IRremote.hpp>
#include <EEPROM.h>

#define receiver_pin 2
#define sender_pin 3
#define btn_0 12
#define btn_1 9
#define btn_2 6

#define debug 1

#define delay_addr 0
#define mode_addr 16

// (0) Lumien: 
#define L_ADRESS 0x4040

#define L_POWER 0x0A
#define L_OK 0x0D
#define L_LEFT 0x10
#define L_VOLUME 0x0F
#define L_HOME 0x1A
#define L_BACK 0x40
#define L_PC 0x33
#define L_MENU 0x42
#define L_VOL_MINUS 0x1C
#define L_VOL_PLUS 0x15

// (1) MosTech:
#define M_ADRESS 0x08

#define M_POWER 0xD7
#define M_OK 0x9B
#define M_LEFT 0x97
#define M_VOLUME 0xAE
#define M_HOME 0xAD
#define M_BACK 0xDC
#define M_MENU 0xD4

// (2) Irbis:
#define I_ADRESS 0x01

#define I_POWER 0x18
#define I_OK 0x55
#define I_LEFT 0x47
#define I_HOME 0x04
#define I_FAST_COOL 0x12
#define I_PC 0x0D
#define I_MENU 0x40
#define I_VOL_MINUS 0x10
#define I_VOL_PLUS 0x15



uint16_t send_delay;
uint8_t mode;

uint16_t ADRESS;
uint8_t COMMAND_0[3];
uint8_t COMMAND_1[1];
uint8_t COMMAND_2[1];

struct Buttons {
  bool _0;
  bool _1;
  bool _2;
  bool _0_proc;
  bool _1_proc;
  bool _2_proc;
};

void refresh_commands() {
  switch(mode) {
    case 0:
      ADRESS = L_ADRESS;

      COMMAND_0[0] = L_POWER;
      COMMAND_0[1] = L_LEFT;
      COMMAND_0[2] = L_OK;

      COMMAND_1[0] = L_HOME;
      COMMAND_2[0] = L_VOLUME;
      break;
    case 1:
      ADRESS = M_ADRESS;
      
      COMMAND_0[0] = M_POWER;
      COMMAND_0[1] = M_LEFT;
      COMMAND_0[2] = M_OK;

      COMMAND_1[0] = M_HOME;
      COMMAND_2[0] = M_VOLUME;
      break;
    case 2:
      ADRESS = I_ADRESS;
      
      COMMAND_0[0] = I_POWER;
      COMMAND_0[1] = I_LEFT;
      COMMAND_0[2] = I_OK;

      COMMAND_1[0] = I_HOME;
      COMMAND_2[0] = I_FAST_COOL;
      break;
  }
}

void check_receiver() {
  if (IrReceiver.decode()) {
    // Игнорируем флаги повтора зажатой кнопки
    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
      
      // 1. Протокол
      Serial.print(F("\nProtocol: "));
      Serial.println(IrReceiver.getProtocolString());

      // 2. Адрес
      Serial.print(F("Address: 0x"));
      if ((uint8_t)IrReceiver.decodedIRData.address < 0x10) {
        Serial.print('0');
      }
      Serial.println(IrReceiver.decodedIRData.address, HEX);

      // 3. Команда и её размер
      Serial.print(F("Command: 0x"));
      if ((uint8_t)IrReceiver.decodedIRData.command < 0x10) {
        Serial.print('0');
      }
      Serial.println(IrReceiver.decodedIRData.command, HEX);

      // 4. Сырые данные (декодированное шестнадцатеричное число)
      Serial.print(F("Raw Data: 0x"));
      Serial.print(IrReceiver.decodedIRData.decodedRawData, HEX);
      Serial.print(F(" (len: "));
      Serial.print(IrReceiver.decodedIRData.numberOfBits); // Размер пакета протокола
      Serial.println(F("bit)"));

      Serial.println(F("\n"));
    }
    IrReceiver.resume();
  }
}


void check_serial() {
  if (Serial.available() > 0) {
    // Ждем не больше 20 мс
    Serial.setTimeout(20);

    // Считываем всё до '\n'
    String input = Serial.readStringUntil('\n');

    // Очищаем от мусора по краям
    input.trim();

    // Если ничего не пришло — выходим
    if (input.length() == 0) {
      return;
    }

    String cmd = "";
    String arg = "";

    // Ищем первый пробел, чтобы отделить команду от аргумента (если он есть)
    uint8_t space_idx = input.indexOf(' ');

    if (space_idx != -1) {
      cmd = input.substring(0, space_idx);
      arg = input.substring(space_idx + 1);
      
      cmd.trim();
      arg.trim();
    } else {
      // Если пробелов нет, вся строка — это команда
      cmd = input;
    }

    // === Обработка команд ===

    if (cmd.equalsIgnoreCase("help")) {
      Serial.println(F("--- Available commands ---"));
      Serial.println(F("help            : Show this list"));
      Serial.println(F("delay           : Show current delay value"));
      Serial.println(F("mode            : Show current mode"));
      Serial.println(F("set_delay <num> : Set new delay (0-65535)"));
      Serial.println(F("mode_lumien     : Select Lumien preset"));
      Serial.println(F("mode_mostech    : Select MosTech preset"));
      Serial.println(F("mode_irbis      : Select Irbis preset"));
      Serial.println(F("--------------------------"));
    } 
    
    else if (cmd.equalsIgnoreCase("delay")) {
      Serial.print(F("Current delay: "));
      Serial.print(F("Current delay: "));
      Serial.print(send_delay); 
      Serial.println(F((" ms")));
    } 

    else if (cmd.equalsIgnoreCase("mode")) {
      Serial.print(F("Current mode: "));
      switch(mode) {
        case 0: Serial.println(F("Lumien")); break;
        case 1: Serial.println(F("MosTech")); break;
        case 2: Serial.println(F("Irbis")); break;
      }
    }
    
    else if (cmd.equalsIgnoreCase("set_delay")) {
      if (arg.length() > 0) {
        char *end_ptr;
        uint32_t val = strtoul(arg.c_str(), &end_ptr, 0);

        if (end_ptr != arg.c_str() && val <= 65535) {
          send_delay = (uint16_t)val;
          EEPROM.put(delay_addr, send_delay);

          Serial.print(F("New delay: "));
          Serial.print(send_delay);
          Serial.println(F(" has successfully saved"));
        } else {
          Serial.println(F("Error: invalid delay value (0-65535)"));
        }
      } else {
        Serial.println(F("Error: missing value. Use: set_delay <number>"));
      }
    } 
    
    else if (cmd.equalsIgnoreCase("mode_lumien")) {
      Serial.println(F("Mode changed to: Lumien"));
      EEPROM.put(mode_addr, 0);
      mode = 0;
      refresh_commands();
    } 
    
    else if (cmd.equalsIgnoreCase("mode_mostech")) {
      Serial.println(F("Mode changed to: MosTech"));
      EEPROM.put(mode_addr, 1);
      mode = 1;
      refresh_commands();
    } 
    
    else if (cmd.equalsIgnoreCase("mode_irbis")) {
      Serial.println(F("Mode changed to: Irbis"));
      EEPROM.put(mode_addr, 2);
      mode = 2;
      refresh_commands();
    } 
    
    else {
      // Неизвестная команда или опечатка
      Serial.print(F("Unknown command: '"));
      Serial.print(cmd);
      Serial.println(F("'. \nType 'help' for the list of available commands."));
    }
  }
}


void check_buttons(Buttons& bts) {
    if (!digitalRead(btn_0)) {
    bts._0 = true;
  } else {
    bts._0 = false;
    bts._0_proc = false;
  }

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
}


void send_command(const uint8_t& command) {
  IrReceiver.stop();
  IrSender.sendNEC(ADRESS, command, 0);
  delay(10);
  IrReceiver.start();
  IrReceiver.resume();
}


void _setup() {
  init(); //надо, чтобы ардуинка завелась, встроенная функция
  Serial.begin(115200); //инициализия com порта
  IrReceiver.begin(receiver_pin, ENABLE_LED_FEEDBACK);
  IrSender.begin(sender_pin);

  pinMode(btn_0, INPUT_PULLUP);
  pinMode(btn_0 - 2, OUTPUT);
  digitalWrite(btn_0 - 2, LOW);

  pinMode(btn_1, INPUT_PULLUP);
  pinMode(btn_1 - 2, OUTPUT);
  digitalWrite(btn_1 - 2, LOW);

  pinMode(btn_2, INPUT_PULLUP);
  pinMode(btn_2 - 2, OUTPUT);
  digitalWrite(btn_2 - 2, LOW);

  EEPROM.get(delay_addr, send_delay);
  EEPROM.get(mode_addr, mode);
  refresh_commands();
}

int main() {
  _setup();
  Buttons buttons = {false, false, false, false, false, false};
  uint32_t last_bt_time = millis();
  uint32_t last_ser_time = millis();
  uint32_t last_res_time = millis();

  while(true) {
    if(millis() - last_bt_time > 30) {
    check_buttons(buttons);

    if(buttons._0 && !buttons._0_proc) {
      uint8_t count = sizeof(COMMAND_0) / sizeof(COMMAND_0[0]);
      for(uint8_t i = 0; i < count; i++) {
        send_command(COMMAND_0[i]);
        if(count - i > 1) delay(send_delay);
      }
      if(debug) Serial.println(F("command 0 has sent"));
      buttons._0_proc = true;
    }
    
    if(buttons._1 && !buttons._1_proc) {
      uint8_t count = sizeof(COMMAND_1) / sizeof(COMMAND_1[0]);
      for(uint8_t i = 0; i < count; i++) {
        send_command(COMMAND_1[i]);
        if(count - i > 1) delay(send_delay);
      }
      if(debug) Serial.println(F("command 1 has sent"));
      buttons._1_proc = true;
    }

    if(buttons._2 && !buttons._2_proc) {
      uint8_t count = sizeof(COMMAND_2) / sizeof(COMMAND_2[0]);
      for(uint8_t i = 0; i < count; i++) {
        send_command(COMMAND_2[i]);
        if(count - i > 1) delay(send_delay);
      }
      if(debug) Serial.println(F("command 2 has sent"));
      buttons._2_proc = true;
    }

    last_bt_time += 30;
    }

    if(millis() - last_ser_time > 50) {
      check_serial();

      last_ser_time += 50;
    }

    if(millis() - last_res_time > 25) {
      check_receiver();

      last_res_time += 25;
    }
  }
}