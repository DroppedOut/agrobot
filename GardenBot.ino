#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <TroykaLight.h>
#include <dht.h>
#include "TrackingCamDxlUart.h"

#define X_MIN 0
#define X_MAX 55.5
#define Y_MIN 0
#define Y_MAX 32.0
#define Z_MIN 0
#define Z_MAX 9.0

#define DROP_POS_X 31.3
#define DROP_POS_Y 47

#define STEP_ENABLE 8
#define STEP_PIN_X 2 
#define DIR_PIN_X 5
#define STEP_PIN_Y 3 
#define DIR_PIN_Y 6
#define STEP_PIN_Z 4 
#define DIR_PIN_Z 7
#define STEP_PIN_TOOL 12 
#define DIR_PIN_TOOL 13

#define X_ENDSTOP 9
#define Y_ENDSTOP 15
#define Z_ENDSTOP 11

#define HUMIDITY_PIN A15
#define LIGHT_PIN A14
#define MOISTURE_PIN A13

#define WATER_PIN A12
#define LED_PIN A11

struct toolpos
{
  float x = 0;
  float y = 0;
  float z = 0;
};

toolpos pos;

toolpos drop_pos;

dht DHT;

char msg[8];
LiquidCrystal_I2C lcd(0x27,20,4);  

TroykaLight sensorLight(LIGHT_PIN);
  
TrackingCamDxlUart trackingCam;

void erase_plants(int rows=5,int columns=3)
{
  trackingCam.init(51, 1, 115200, 30);
  delay(5000);
  
   move_z(Z_MAX/2);
  for(int i=0;i<columns;i++)
  {
    if(i%2==0)
      for(int j=0;j<rows;j++)
      {
        move_xy(STEP_PIN_X,DIR_PIN_X,X_MAX/rows);
        uint8_t n = trackingCam.readBlobs(5);
        if(n>0)
        {
          move_z(Z_MAX/2);
          use_drill();
          move_z(-Z_MAX/2);
        }
      }
    else
      for(int j=0;j<rows;j++)
      {
        move_xy(STEP_PIN_X,DIR_PIN_X,-X_MAX/rows);
        move_z(Z_MAX/2);
        uint8_t n = trackingCam.readBlobs(5);
        if(n>0)
        {
          move_z(Z_MAX/2);
          use_drill();
          move_z(-Z_MAX/2);
        } 
      }
    move_xy(STEP_PIN_Y,DIR_PIN_Y,Y_MAX/columns);
  }
}

void use_drill() //вращение насадкой (бур, культиватор)
{
  digitalWrite(DIR_PIN_TOOL,LOW);
  for(int i = 0; i < 1000; i++) 
  {
    digitalWrite(STEP_PIN_TOOL,HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(STEP_PIN_TOOL,LOW); 
    delayMicroseconds(1000); 
  }
}

void open_arm() //открыть захват
{
  digitalWrite(DIR_PIN_TOOL,LOW);
  for(int i = 0; i < 50; i++) 
  {
    digitalWrite(STEP_PIN_TOOL,HIGH); 
    digitalWrite(STEP_PIN_TOOL,LOW); 
    delay(10);
  } 
}

void close_arm() //закрыть захват
{
  digitalWrite(DIR_PIN_TOOL,HIGH);
  for(int i = 0; i < 50; i++) 
  {
    digitalWrite(STEP_PIN_TOOL,HIGH); 
    digitalWrite(STEP_PIN_TOOL,LOW); 
    delay(10);
  } 
}

void move_z(float distance)//движение по OZ, есть проверка расстояния на валидность, отрицательное расстояние - движемся вверх, положительное - вниз
{
  distance > 0? digitalWrite(DIR_PIN_Z,LOW) : digitalWrite(DIR_PIN_Z,HIGH);
  
  if (pos.z + distance > Z_MAX)
  {
    distance = Z_MAX - pos.z; 
    pos.z = Z_MAX;
  } 
  else
  {
  if (pos.z + distance < Z_MIN)
  {
    distance = -pos.z; 
    pos.z = Z_MIN;
  } 
  else
  {
    pos.z += distance; 
  }
  }
    for(int i = 0; i < 500*fabs(distance); i++) 
  {
    digitalWrite(STEP_PIN_Z,HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(STEP_PIN_Z,LOW); 
    delayMicroseconds(1000); 
  }
  update_display();
}

void move_xy(int step_pin,int dir_pin,float distance)//движение по осям ОХ и ОУ, есть проверка расстояния на валидность
{
  distance > 0? digitalWrite(dir_pin,HIGH) : digitalWrite(dir_pin,LOW);

  if (dir_pin == DIR_PIN_X)
  {
    if (pos.x + distance > X_MAX)
    {
      distance = X_MAX - pos.x; 
      pos.x = X_MAX;
    } 
    else
    {
      if (pos.x + distance < X_MIN)
      {
        distance = -pos.x; 
        pos.x = X_MIN;
      } 
      else
      {
        pos.x += distance; 
      }
    }
  }
    if (dir_pin == DIR_PIN_Y)
  {
    if (pos.y + distance > Y_MAX)
    {
      distance = Y_MAX - pos.y; 
      pos.y = Y_MAX;
    } 
    else
    { 
      if (pos.y + distance < Y_MIN)
      {
        distance = -pos.y; 
        pos.y = Y_MIN;
      } 
      else
      {
        pos.y += distance; 
      }
    }
  }
  
  for(int i = 0; i < 50 * fabs(distance); i++) 
  {
    digitalWrite(step_pin,HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(step_pin,LOW); 
    delayMicroseconds(1000); 
  }
  update_display();
}

void move_xy_coord(float x,float y)//движение к заданной точке с координатами x y
{
  x > X_MAX? x=X_MAX:x=x;
  x < X_MIN? x=X_MIN:x=x; 
  y > Y_MAX? y=Y_MAX:y=y;
  y < Y_MIN? y=Y_MIN:y=y;  

  move_xy(STEP_PIN_X,DIR_PIN_X,x-pos.x);
  move_xy(STEP_PIN_Y,DIR_PIN_Y,y-pos.y);
}

void home_x()//движение в 0 по x
{
  digitalWrite(DIR_PIN_X,LOW);
  while(digitalRead(X_ENDSTOP) != true)
  {
    digitalWrite(STEP_PIN_X,HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(STEP_PIN_X,LOW); 
    delayMicroseconds(1000); 
  }
  pos.x = 0;
}

void home_y()//движение в 0 по y
{
  digitalWrite(DIR_PIN_Y,LOW);
  while(digitalRead(Y_ENDSTOP) != true)
  {
    digitalWrite(STEP_PIN_Y,HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(STEP_PIN_Y,LOW); 
    delayMicroseconds(1000); 
  }
  pos.y = 0;
  update_display();
}

void home_z()//движение в 0 по z
{
  digitalWrite(DIR_PIN_Z,HIGH);
  while(digitalRead(Z_ENDSTOP) != true)
  {
    digitalWrite(STEP_PIN_Z,HIGH); 
    delayMicroseconds(1000); 
    digitalWrite(STEP_PIN_Z,LOW); 
    delayMicroseconds(1000); 
  }
  pos.z=0;
  update_display();
}

void move_home()//движение в 0 по всем осям
{
  home_x();
  home_y();
  home_z();
}

void dig(int rows=5,int columns=3)// движемся змейкой, копаем ямы буром, аргументы функции - количество рядов и строк с ямами
{
  move_z(Z_MAX/2);
  for(int i=0;i<columns;i++)
  {
    if(i%2==0)
    for(int j=0;j<rows;j++)
    {
      move_xy(STEP_PIN_X,DIR_PIN_X,X_MAX/rows);
      move_z(Z_MAX/2);
      use_drill();
      move_z(-Z_MAX/2);
    }
    else
    for(int j=0;j<rows;j++)
    {
      move_xy(STEP_PIN_X,DIR_PIN_X,-X_MAX/rows); 
      move_z(Z_MAX/2);
      use_drill();
      move_z(-Z_MAX/2);
    }
    move_xy(STEP_PIN_Y,DIR_PIN_Y,Y_MAX/columns);
  }
  move_home();
}

void harvest(int rows=5,int columns=3)// Движемся змейкой, берем урожай захватом, везем его в корзину, сбрасываем, возвращаемся к предыдущей точке, дальше едем змейкой.
{
  move_z(Z_MAX/2);
  for(int i=0;i<columns;i++)
  {
    if(i%2==0)
    for(int j=0;j<rows;j++)
    {
      move_xy(STEP_PIN_X,DIR_PIN_X,X_MAX/rows);
      open_arm();
      move_z(Z_MAX/2);
      close_arm();
      move_z(-Z_MAX/2);

      toolpos temp_pos;
      temp_pos.x=pos.x;
      temp_pos.y=pos.y;
      
      move_xy_coord(drop_pos.x,drop_pos.y);
      move_z(1);
      open_arm();
      close_arm();
      move_z(-1);
      move_xy_coord(temp_pos.x,temp_pos.y);
      drop_pos.x+=4;
    }
    else
    for(int j=0;j<rows;j++)
    {
      move_xy(STEP_PIN_X,DIR_PIN_X,-X_MAX/rows); 
      open_arm();
      move_z(Z_MAX/2);
      close_arm();
      move_z(-Z_MAX/2);

      toolpos temp_pos;
      temp_pos.x=pos.x;
      temp_pos.y=pos.y;
      
      move_xy_coord(drop_pos.x,drop_pos.y);
      move_z(1);
      open_arm();
      close_arm();
      move_z(-1);
      move_xy_coord(temp_pos.x,temp_pos.y);
      drop_pos.x+=4;
    }
    move_xy(STEP_PIN_Y,DIR_PIN_Y,Y_MAX/columns);
  }
}

void water_on()
{
  digitalWrite(WATER_PIN,125);
}

void water_off()
{
  digitalWrite(WATER_PIN,LOW);
}

void led_on()
{
  digitalWrite(LED_PIN,HIGH);
}

void led_off()
{
  digitalWrite(LED_PIN,LOW);
}

void water(int rows=5,int columns=3)//движемся змейкой без остаковок с включенным поливом
{
  move_z(Z_MAX/2);
  for(int i=0;i<columns;i++)
  {
    if(i%2==0)
      for(int j=0;j<rows;j++)
      {
        move_xy(STEP_PIN_X,DIR_PIN_X,X_MAX/rows);
        move_z(Z_MAX/2);
        water_on();
        delay(3000);
        water_off();
        move_z(-Z_MAX/2);
      }
    else
      for(int j=0;j<rows;j++)
      {
        move_xy(STEP_PIN_X,DIR_PIN_X,-X_MAX/rows);
        move_z(Z_MAX/2);
        water_on();
        delay(3000);
        water_off();
        move_z(-Z_MAX/2); 
      }
    move_xy(STEP_PIN_Y,DIR_PIN_Y,Y_MAX/columns);
  }
}
float read_sensor(int pin)
{
  int sensorValue=0;
  for (int i = 0; i <= 100; i++) 
  { 
   sensorValue = sensorValue + analogRead(pin); 
   delay(1); 
  } 
  sensorValue = sensorValue/100.0;
  return sensorValue;
}

void update_display()
{
   static float old_value_temp=20, old_value_hum=5;
   float sensorValue = read_sensor(MOISTURE_PIN);
   dtostrf(sensorValue, 6, 2, msg);
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("Moisture: ");
   lcd.setCursor(9,0);
   lcd.print(msg);
   
   sensorLight.read();
   dtostrf(sensorLight.getLightLux(), 6, 2, msg);
   lcd.setCursor(0,1);
   lcd.print("Light: ");
   lcd.setCursor(9,1);
   lcd.print(msg);

   DHT.read11(HUMIDITY_PIN);
   sensorValue = DHT.temperature;
   if(sensorValue<-100)
    sensorValue=old_value_temp;
   else
    old_value_temp=sensorValue;
   dtostrf(sensorValue, 6, 2, msg);
   lcd.setCursor(0,2);
   lcd.print("Temperature: ");
   lcd.setCursor(13,2);
   lcd.print(msg);
   
   sensorValue =  DHT.humidity;
   if(sensorValue<-100)
    sensorValue=old_value_hum;
   else
    old_value_hum=sensorValue;
   dtostrf(sensorValue, 6, 2, msg);
   lcd.setCursor(0,3);
   lcd.print("Humidity: ");
   lcd.setCursor(13,3);
   lcd.print(msg);
}

void setup() 
{
  drop_pos.x=DROP_POS_X;
  drop_pos.y=DROP_POS_Y;

  pinMode(STEP_ENABLE,OUTPUT);
  pinMode(WATER_PIN,OUTPUT);
  pinMode(LED_PIN,OUTPUT);
  pinMode(STEP_PIN_X,OUTPUT); 
  pinMode(DIR_PIN_X,OUTPUT);
  pinMode(STEP_PIN_Y,OUTPUT); 
  pinMode(DIR_PIN_Y,OUTPUT);
  pinMode(STEP_PIN_Z,OUTPUT); 
  pinMode(DIR_PIN_Z,OUTPUT);
  pinMode(STEP_PIN_TOOL,OUTPUT); 
  pinMode(DIR_PIN_TOOL,OUTPUT);

  pinMode(X_ENDSTOP,INPUT); 
  pinMode(Y_ENDSTOP,INPUT); 
  pinMode(Z_ENDSTOP,INPUT); 
  water_off();
  led_on();
     
  digitalWrite(STEP_ENABLE,LOW);
  Serial.begin(9600);
  lcd.init();                  
  lcd.backlight();
  lcd.setCursor(1,0);
  update_display();
  delay(1000);
  move_home();
  //здесь могут быть ваши команды
  move_home();
}

void loop() 
{
update_display();
delay(3000);
}
