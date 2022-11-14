/**********************************************************************
  케이프로 QGC베터리 경고 외부로 알람. (2022/10/12)
  QGC(또는 mavlink_udp+libserial예제코드)로부터 시리얼 신호를 받으면 경고등(LED)를 켠다.
  '1':  QGC-> Arduino  :  Arduino recive message from QGC that Battery Warnig.Turn ON RED LED.
  '2':  QGC-> Arduino  :  Arduino recive message from QGC that RTL event.Turn ON GREEN LED.
  '3':  QGC-> Arduino  :  Arduino recive message from QGC that RC LOSS.Turn ON YELLOW LED.
  4채널(4~7핀) 릴레이 쉴드 사용하여 LED제어. (7번핀 릴레이1은 사용하지 않음) 
*****************************************************************************************/

//#define SERIAL_PRINT_DEBUG // 아두이노의 시리얼 모니터 디버깅시 주석 해제

#define PLAY_TIME 3000 // LED On Time
int incomingByte = 0;
const int BATTERY_LOW_LED = 6; // relay2: RED
const int RTL_ALARM_LED =   5; // relay3: GREEN
const int RC_LOSS_LED =     4; // realy4: YELLOW

int flgLowBattry = 0;
int flgRTL        = 0;
int flgRcLoss   =0;

unsigned int cntLowBattery = 0;
unsigned int cntRTL = 0;
unsigned int cntRcLoss = 0;

void alramLowBattery();
void alramRTL();
void alramRcLoss();

void setup() {
  Serial.begin(115200);
  pinMode(BATTERY_LOW_LED, OUTPUT);
  pinMode(RTL_ALARM_LED, OUTPUT);
  pinMode(RC_LOSS_LED, OUTPUT);
}

void loop() {
  delay(1);
  if (Serial.available() > 0) //[RX] Arduino recive message from QGC
  {
    incomingByte = Serial.read();
    if ( /*0x01*/'1' == incomingByte )  { // Battery Warnig
      flgLowBattry = 1;
    }
    if ( /*0x02*/'2' == incomingByte )  { // RTL ALARM
      flgRTL = 1;
    }
    if ( /*0x02*/'3' == incomingByte )  { // RC Loss ALARM
      flgRcLoss = 1;
    }
#ifdef SERIAL_PRINT_DEBUG
    if ('1' == incomingByte ) {
      Serial.print("Recive data: ");
      Serial.println( incomingByte );
      flgLowBattry = 1;
    }
    if ('2' == incomingByte ) {
      Serial.print("Recive data: ");
      Serial.println( incomingByte );
      flgRTL = 1;
    }
    if ('3' == incomingByte ) {
      Serial.print("Recive data: ");
      Serial.println( incomingByte );
      flgRcLoss = 1;
    }
    
#endif

  } // if (Serial.available() > 0)

  if (flgLowBattry)
  {
    alramLowBattery();
  }

  if (flgRTL)
  {
    alramRTL();
  }

  if (flgRcLoss)
  {
    alramRcLoss();
  }

}// void loop()


void alramLowBattery(){
  if (0 == cntLowBattery)// 처음 한번만 실행
  {
    digitalWrite(BATTERY_LOW_LED, HIGH);
#ifdef SERIAL_PRINT_DEBUG
    Serial.println("BATTERY_LOW_LED On" );
#endif
  }
  cntLowBattery++;

  if (PLAY_TIME < cntLowBattery)// 일정 시간 동작 후 정지.
  {
    digitalWrite(BATTERY_LOW_LED, LOW);
    //if nessesary Arduino send message to QGC that LED is Turned Off.
    //Serial.write(0x03); //[TX}

    // Clear variables
    cntLowBattery = 0;
    flgLowBattry = 0;

#ifdef SERIAL_PRINT_DEBUG
    Serial.println("Time out: LED Off");
#endif
  }
}

void alramRTL(){
  if (0 == cntRTL) // 처음 한번만 실행
  {
    digitalWrite(RTL_ALARM_LED, HIGH);
  }
  cntRTL++;

  if (PLAY_TIME < cntRTL)// 일정 시간 동작 후 정지.
  {
    digitalWrite(RTL_ALARM_LED, LOW);
    cntRTL = 0;
    flgRTL = 0;
  }
}

void alramRcLoss(){
  if (0 == cntRcLoss) // 처음 한번만 실행
  {
    digitalWrite(RC_LOSS_LED, HIGH);
  }
  cntRcLoss++;

  if (PLAY_TIME < cntRcLoss)// 일정 시간 동작 후 정지.
  {
    digitalWrite(RC_LOSS_LED, LOW);
    cntRcLoss = 0;
    flgRcLoss = 0;
  }
}
