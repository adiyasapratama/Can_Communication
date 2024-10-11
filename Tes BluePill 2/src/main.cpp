#include "STM32_CAN.h"
#include <stdint.h>

#define EXP_U PA0
#define EXP_L PA1
#define EXP_R PA2

static CAN_message_t CAN_inMsg;
static CAN_message_t CAN_outMsg_1;
static CAN_message_t CAN_outMsg_2;
static CAN_message_t CAN_outMsg_3;

STM32_CAN Can( CAN1, ALT, RX_SIZE_64, TX_SIZE_16 );

uint8_t motorControl[3];  // Store the expand data (on/off)
uint8_t Counter;

void sendData() {
  CAN_message_t outMsg;
  outMsg.id = 0x200; // set ID
  outMsg.len = 3;//message length (3 bytes)

  // motor status send
  outMsg.buf[0] = digitalRead(EXP_U);
  outMsg.buf[1] = digitalRead(EXP_L);
  outMsg.buf[2] = digitalRead(EXP_R);

  Can.write(outMsg);

  if (Counter >= 255){ Counter = 0;}
  
  // Only the counter value is updated to the 3 messages sent out.
  CAN_outMsg_1.buf[3] =  Counter; 
  Can.write(CAN_outMsg_1);

  CAN_outMsg_2.buf[5] =  Counter;
  Can.write(CAN_outMsg_2);

  CAN_outMsg_3.buf[6] =  Counter;
  Can.write(CAN_outMsg_3);

  Serial.print("Sent: ");
  Serial.println(Counter, HEX);
  Counter++;
}


void readCanMessage() // Read data from CAN bus and print out the messages to serial bus. Note that only message ID's that pass filters are read.
{
  if (Can.read(CAN_inMsg)) {
    //check ID message
    if (CAN_inMsg.id == 0x1A5) {
      motorControl[0] = CAN_inMsg.buf[0];
      // Motor 1 control
      if (motorControl[0] == 1) {
        digitalWrite(EXP_U, HIGH);
      } else {
        digitalWrite(EXP_U, LOW);
      }

      // Motor 2 control
      motorControl[1] = CAN_inMsg.buf[1];
      if (motorControl[1] == 1) {
        digitalWrite(EXP_L, HIGH);
      } else {
        digitalWrite(EXP_L, LOW);
      }

      // Motor 3 control
      motorControl[2] = CAN_inMsg.buf[2];
      if (motorControl[2] == 1) {
        digitalWrite(EXP_R, HIGH);
      } else {
        digitalWrite(EXP_R, LOW);
      }
    }
  }
}

void setup() {
  pinMode(EXP_U, OUTPUT);
  pinMode(EXP_L, OUTPUT);
  pinMode(EXP_R, OUTPUT);
  Serial.begin(115200);
  
  Can.begin();
  Can.setBaudRate(500000);
  Can.setMBFilterProcessing(MB0, 0x153, 0x1FFFFFFF);
  Can.setMBFilterProcessing(MB1, 0x613, 0x1FFFFFFF);
  Can.setMBFilterProcessing(MB2, 0x615, 0x1FFFFFFF, STD);
  Can.setMBFilterProcessing(MB3, 0x1F0, 0x1FFFFFFF, EXT);

  // We set the data that is static for the three different message structs once here.
  CAN_outMsg_1.id = (0x1B5);
  CAN_outMsg_1.len = 8;
  CAN_outMsg_1.buf[0] =  0x03;
  CAN_outMsg_1.buf[1] =  0x41;
  CAN_outMsg_1.buf[2] =  0x11;
  CAN_outMsg_1.buf[3] =  0x00;
  CAN_outMsg_1.buf[4] =  0x00;
  CAN_outMsg_1.buf[5] =  0x00;
  CAN_outMsg_1.buf[6] =  0x00;
  CAN_outMsg_1.buf[7] =  0x00;

  CAN_outMsg_2.id = (0x7E8);
  CAN_outMsg_2.len = 8;
  CAN_outMsg_2.buf[0] =  0x03;
  CAN_outMsg_2.buf[1] =  0x41;
  CAN_outMsg_2.buf[3] =  0x21;
  CAN_outMsg_2.buf[4] =  0x00;
  CAN_outMsg_2.buf[5] =  0x00;
  CAN_outMsg_2.buf[6] =  0x00;
  CAN_outMsg_2.buf[7] =  0xFF;

  CAN_outMsg_3.id = (0xA63);
  CAN_outMsg_3.len = 8;
  CAN_outMsg_3.buf[0] =  0x63;
  CAN_outMsg_3.buf[1] =  0x49;
  CAN_outMsg_3.buf[2] =  0x11;
  CAN_outMsg_3.buf[3] =  0x22;
  CAN_outMsg_3.buf[4] =  0x00;
  CAN_outMsg_3.buf[5] =  0x00;
  CAN_outMsg_3.buf[6] =  0x00;
  CAN_outMsg_3.buf[7] =  0x00;

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
#else //2.0 forward
  SendTimer->attachInterrupt(sendData);
#endif
  SendTimer->resume();
}

void loop() {
  // We only read data from CAN bus if there is frames received, so that main code can do it's thing efficiently.
  while (Can.read(CAN_inMsg)) {
    readCanMessage();
  }
}

