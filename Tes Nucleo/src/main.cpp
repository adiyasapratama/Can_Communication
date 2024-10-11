#include "STM32_CAN.h"
#include <stdint.h>
#define Lampu LED_BUILTIN

static CAN_message_t CAN_outMsg_1;
static CAN_message_t CAN_outMsg_2;
static CAN_message_t CAN_outMsg_3;
static CAN_message_t CAN_outMsg_4;
static CAN_message_t CAN_inMsg;

STM32_CAN Can( CAN1, ALT, RX_SIZE_64, TX_SIZE_16 );

uint8_t Counter;

union FloatToBytes {
  float velocity;
  uint8_t byteArray[4];
};

// Data float
FloatToBytes motor_1;
FloatToBytes motor_2;
FloatToBytes motor_3;
FloatToBytes motor_4;

void SendData() {
  if (Counter >= 255) {
    Counter = 0;
  }

  motor_1.velocity = 0.0;
  motor_2.velocity = 0.0;
  motor_3.velocity = 0.0;
  motor_4.velocity = 0.0;
  
  // Isi buffer message
  for (int i = 0; i < 4; i++) {
    CAN_outMsg_1.buf[i] = motor_1.byteArray[i]; //Motor 1
    CAN_outMsg_1.buf[i + 4] = motor_2.byteArray[i]; // Motor 2 
  }

  // Mengisi buffer CAN message dengan data kecepatan motor 3 dan 4
  for (int i = 0; i < 4; i++) {
    CAN_outMsg_2.buf[i] = motor_3.byteArray[i]; // Motor 3
    CAN_outMsg_2.buf[i + 4] = motor_4.byteArray[i]; // Motor 4
  }

  Can.write(CAN_outMsg_1); // Motor 1, 2
  Can.write(CAN_outMsg_2); // Motor 3, 4

  Serial.print("Sent: ");
  Serial.println(Counter, HEX);
  Counter++;
}

void readCanMessage() // Read data from CAN bus and print out the messages to serial bus. Note that only message ID's that pass filters are read.
{
  if (Can.read(CAN_inMsg)) {
    // Check ID
    if (CAN_inMsg.id == 0x1A5) {
      // Konversi byteArray --> float
      for (int i = 0; i < 4; i++) {
        motor_1.byteArray[i] = CAN_inMsg.buf[i]; // Motor 1
        motor_2.byteArray[i] = CAN_inMsg.buf[i + 4]; // Motor 2
      } 
      Serial.print("Motor 1 Speed: ");
      Serial.println(motor_1.velocity);
      Serial.print("Motor 2 Speed: ");
      Serial.println(motor_2.velocity);
    } 
    else if (CAN_inMsg.id == 0x7E8) {
      // Konversi byteArray --> float
      for (int i = 0; i < 4; i++) {
        motor_3.byteArray[i] = CAN_inMsg.buf[i]; // Motor 3
        motor_4.byteArray[i] = CAN_inMsg.buf[i + 4]; // Motor 4
      }
      
      Serial.print("Motor 3 Speed: ");
      Serial.println(motor_3.velocity);
      Serial.print("Motor 4 Speed: ");
      Serial.println(motor_4.velocity);
    }
  }
}

void setup() {
  pinMode(Lampu, OUTPUT);
  Counter = 0;
  Serial.begin(115200);
  
  Can.begin();
  Can.setBaudRate(500000);
  Can.setMBFilterProcessing( MB0, 0x153, 0x1FFFFFFF );
  Can.setMBFilterProcessing( MB1, 0x613, 0x1FFFFFFF );
  Can.setMBFilterProcessing( MB2, 0x615, 0x1FFFFFFF, STD );
  Can.setMBFilterProcessing( MB3, 0x1F0, 0x1FFFFFFF, EXT );

  // Set ID and length of the message
  CAN_outMsg_1.id = (0x1A5);
  CAN_outMsg_1.len = 8; // 4 bytes for motor 1, 4 bytes for motor 2
  CAN_outMsg_1.buf[0] =  0x03;
  CAN_outMsg_1.buf[1] =  0x41;
  CAN_outMsg_1.buf[2] =  0x11;
  CAN_outMsg_1.buf[3] =  0x00;
  CAN_outMsg_1.buf[4] =  0x00;
  CAN_outMsg_1.buf[5] =  0x00;
  CAN_outMsg_1.buf[6] =  0x00;
  CAN_outMsg_1.buf[7] =  0x00;

  CAN_outMsg_2.id = (0x7E8);
  CAN_outMsg_2.len = 8; // 4 bytes for motor 3, 4 bytes for motor 4
  CAN_outMsg_2.buf[0] =  0x03;
  CAN_outMsg_2.buf[1] =  0x41;
  CAN_outMsg_2.buf[3] =  0x21;
  CAN_outMsg_2.buf[4] =  0x00;
  CAN_outMsg_2.buf[5] =  0x00;
  CAN_outMsg_2.buf[6] =  0x00;
  CAN_outMsg_2.buf[7] =  0xFF;
  
  // setup hardware timer to send data in 50Hz pace
  #if defined(TIM1)
  TIM_TypeDef *Instance = TIM1;
  #else
  TIM_TypeDef *Instance = TIM2;
  #endif
  HardwareTimer *SendTimer = new HardwareTimer(Instance);
  SendTimer->setOverflow(50, HERTZ_FORMAT); // 50 Hz
  #if ( STM32_CORE_VERSION_MAJOR < 2 )
  SendTimer->attachInterrupt(1, SendData);
  SendTimer->setMode(1, TIMER_OUTPUT_COMPARE);
  #else
  SendTimer->attachInterrupt(SendData);
  #endif
  SendTimer->resume();
}


void loop() {
  // We only read data from CAN bus if there is frames received, so that main code can do it's thing efficiently.
  while (Can.read(CAN_inMsg)) {
    readCanMessage();
  }
  digitalWrite(Lampu, HIGH);
  delay(1000);
  digitalWrite(Lampu, LOW);
  delay(1000);
}

