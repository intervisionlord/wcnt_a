#include <SoftwareSerial.h>
SoftwareSerial esps(8, 9); // RX, TX


char Version[7] = "0.74.2";
char echo;
String echostr = "";

int led = 13;
int hot = 7; // Вход со счетчика горячей воды
int cold = 2; // Вход со счетчика холодной воды

unsigned long getCold, getHot, newGetCold, newGetHot, coldState, hotState;
int timer; // системный таймер

String ssid     = "----";  // SSID to connect to
String password = "----"; // Our virtual wifi has no password (so dont do your banking stuff on this network)
String host     = "----"; // Open Weather Map API
const int httpPort   = 80;
//String uri     = "/arduino.cfm";

// the setup routine runs once when you press reset:
void setup() {
  
  analogWrite(A5, 200);
  timer = 0;

  pinMode(led, OUTPUT);
  
  pinMode(cold, INPUT_PULLUP); // Вход для счетчика холодной воды
  pinMode(hot, INPUT_PULLUP); // Вход для счетчика горячей воды
  
  // Вешаем прерывания на события со счетчиков
  // При поступлении сигнала со счетчика вызываем функцию обработки сигнала
  
  attachInterrupt(digitalPinToInterrupt(cold), ColdChange, RISING);
  attachInterrupt(digitalPinToInterrupt(hot), HotChange, RISING);
  
  pinMode(A0, INPUT); // Снятие показаний с источника питания (+5В) для анализа заряда батареи.
  
  pinMode(A1, OUTPUT); // Питание светодиода разряда батареи.
  // (При использовании дисплея срабатывает на 4.33 В и ниже)
  // Без дисплея и светодиодов верхний лимит может быть уменьшен.
  
  delay(500);
  
// EEPROM //
// Читаем данные из EEPROM после запуска
// Если предыдущая работа была завершена из-за низкого напряжения питания (разряд батарей),
// то данные заносятся в EEPROM и обработка показаний счетчика завершается.

// Не будет использовано до внедрения FRAM

//coldState = EEPROM.get(1, coldState);
//hotState = EEPROM.get(100, hotState);

  // SoftwareSerial + WIFI //
  // Проверяем ESP модуль
  
  uint32_t baud = 19200; // Скорость работы ESP
  
  Serial.begin(baud);
  esps.begin(baud);
  Serial.println("PWR ON\r\n BOOT SEQ");
  Serial.print("VER.: ");
  Serial.println(Version);
  Serial.print("CHK ESP RDY ... ");
  esps.println("AT");
  if(esps.find("OK")){
    Serial.println("OK");
  }
  delay(100);
  Serial.println("CHK AT VER");
  esps.println("AT+GMR");
  delay(30);
  while(esps.available()) {
    echo = esps.read();
    echostr.concat(echo);
  }
  Serial.println(echostr);
  Serial.println("STARTING...");
  delay(15000);
  
// END WIFI //  
}
// END SETUP //


void loop() {
// Основное тело кода
  timer++;
  
  // Слушаем порт. Если поступила команда SVC,
  // переводим устройство в сервисный ресжим (для задания начальных показаний счетчиков)
  while(Serial.available())
  {
   String sread = Serial.readString();
    if (sread == "svc")
    {
      Service();
    }
  }
  
  int bat = analogRead(A0);
  bat = map(bat, 784, 1023, 0, 100);
  
  // Отладка показаний, считываемых с шины питания
  Serial.println("============");
  Serial.print("Analog: ");
  Serial.println(analogRead(A0));
  
  Serial.print("Digital: ");
  Serial.println(bat);
  // ------------ //

  // Проверяем вольтаж на батарее (на линии питания)

  if (bat <= 15)
  {
   digitalWrite(A1, 1);
    delay(200);
    digitalWrite(A1, 0);
    delay(200);
  }
  else
  {
   digitalWrite(A1, 0);
  }
  
  //if (bat < 0)
    // Состояние заряда, сопоставимое с нулем подразумевает границу нормальной работы.
    // Отрицательным значение становится тогда, когда напряжения питания достаточно
    // для работы контроллера, но не гарантируется его стабильность (теоретически, меньше 5В),
    // на практике планируется проверить работу при 4.3В

    // В случае, когда напряжение стало критически низким записываем текущие показания в EEPROM и завершаем работу
    // (функция PowerOFF)
  //{
  //  EEPROM.put(1, coldState);
  //  EEPROM.put(100, hotState);
  //  
  // PowerOFF();
  //}
   
  // Индикация работы устройства.
  // Включаем и выключаем светодиод на контроллере
  
  if (timer == 5)
  {
    digitalWrite(led, 1);
    delay(200);
    digitalWrite(led, 0);
    timer = 0;
  }
delay(500);
  // END //
}

// Обработка сигнала со счетчика холодной воды

void ColdChange()
  {
    coldState = coldState + 1;
  }   
  
// Обработка сигнала со счетчика горячей воды

void HotChange()  
  {
    hotState = hotState + 1;
  }

// Сервисный режим

void Service()
{
  Serial.println("==========");
  Serial.println("ENT SVC MD");
  Serial.println("==========");
  
svc:  String svcCmd = Serial.readString();
  if (svcCmd == "ext")
  {
  Serial.println("==========");
  Serial.println("EXT SVC MD");
  Serial.println("==========");
    delay(1000);
  return;
  }
  
  if (svcCmd == "h")
  {
    Serial.println("h - this help");
    Serial.println("sc - set cold status");
    Serial.println("sh - set hot status");
    Serial.println("ext - exit service mode");
    Serial.println("stat - show counters status");
    Serial.println("v - show version info");
    Serial.println("stor - save data to EEPROM");
  }
  
  if (svcCmd == "stat")
  {
    Serial.println("==========");
    Serial.print("COLD: ");
    Serial.println(coldState);
    Serial.print("HOT: ");
    Serial.println(hotState);
  }
  
  if (svcCmd == "sc")
  {
    Serial.println("==========");
    Serial.println("New COLD state: ");
    String newSc = Serial.readString();
    delay(3000);
    coldState = newSc.toInt();
    Serial.print("Cold State = ");
    Serial.println(coldState);
  }
  
  if (svcCmd == "sh")
  {
    Serial.println("==========");
    Serial.println("New HOT state: ");
    String newSh = Serial.readString();
    delay(3000);
    hotState = newSh.toInt();
    Serial.print("Hot State = ");
    Serial.println(hotState);
  }
  
  if (svcCmd == "v")
  {
    Serial.println("==========");
    Serial.print("Version Info: ");
    Serial.print(Version);
  }
  
  //if (svcCmd == "stor")
  //{
  //  Serial.println("==========");
  //  Serial.println("RCVD STOR");
  //  Serial.print("STOR COLD... ");
  //  EEPROM.put(1, coldState);
  //  delay(5);
  //  Serial.println("DONE");
  //  Serial.print("STOR HOT... ");
  //  EEPROM.put(100, hotState);
  //  Serial.println("DONE");
  //}
  
  goto svc;
}

// Прекращение работы при низком разряде.
// После записи показаний в EEPROM, прекращается обработка данных со счетчиков
// и включается индикация низкого разряда батареи.
// Это сделано для того, чтобы исключить многократную перезапись EEPROM в цикле
// в силу его ограничений на кол-во итераций записи.

void PowerOFF()
{
pwr:
  digitalWrite(A1, 1);
  delay(200);
  digitalWrite(A1, 0);
  delay(200);
  goto pwr;
}
