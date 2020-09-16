//---------------Библиотеки---------------------
#include <math.h>
#include <ros.h>
#include <ros/time.h>
#include <std_msgs/Int16.h>
#include <std_msgs/Float32.h>
#include <std_msgs/Bool.h>
//----------------------------------------------

//Пины, к которым подключены энкодеры
#define pin_A1 3 // Энкодер
#define pin_B1 2 // Энкодер

//Параметры робота, для вычисления одометрии
#define WHR 0.072 //Радиус колеса, в метрах
#define WHEEL_SEP 0.3 //Расстояние между коселвами, в метрах

#define CPR 20
#define RGHT_LINE_SENSOR 33
#define LEFT_LINE_SENSOR 35

volatile long Position_enc1 = 0; //переменная,для хранения тиков энкодера с первого мотора
volatile long Position_enc2 = 0; //переменная,для хранения тиков энкодера со второго мотора
bool dir1 = false; //переменная, для хранения направления вращения левого колеса, вперед - true, назад - false
bool dir2 = false; //переменная, для хранения направления вращения правого колеса, вперед - true, назад - false

//----------------------------------------------

//Пины, к которым подключен драйвер моторов, каждым мотором управляет по 3 пина, 2 управляют направлением, 1 - скоростью вращения вала
byte ena = 9;
byte in1 = 7;
byte in2 = 8;
byte ena1 = 6;
byte in3 = 4;
byte in4 = 5;

//функция инициализации пинов, задает всем пинам режим работы, настраивает прерывание.
void init_motors()
{
pinMode( ena, OUTPUT );
pinMode( in1, OUTPUT );
pinMode( in2, OUTPUT );
pinMode( ena1, OUTPUT );
pinMode( in3, OUTPUT );
pinMode( in4, OUTPUT );
pinMode(pin_A1, INPUT);
pinMode(pin_B1, INPUT);
attachInterrupt(digitalPinToInterrupt(pin_A1), Encoder1_Rotate, FALLING);//настройка прерывания на пин, к которому подключен энкодер
attachInterrupt(digitalPinToInterrupt(pin_B1), Encoder2_Rotate, FALLING);//настройка прерывания на пин, к которому подключен энкодер
}

//Создание ROS nodehandler
ros::NodeHandle nh;

//Обработчик первого энкодера
void Encoder1_Rotate() {
if (dir1==true) {//Если колесо вращается вперед
Position_enc1++;//увеличиваем счетчик тиков энкодера
} else {//Если колесо вращается назад
Position_enc1--;//уменьшаем счетчик тиков энкодера
}
}

//Обработчик второго энкодера
void Encoder2_Rotate() {
if (dir2==true) {//Если колесо вращается вперед
Position_enc2++;//увеличиваем счетчик тиков энкодера
} else {//Если колесо вращается назад
Position_enc2--;//уменьшаем счетчик тиков энкодера
}
}

//функция чтения данных с пина (используется для датчиков линии)
bool read_sensor(int pin)
{
return digitalRead(pin);
}

//Эта функция (subscriber callback function) срабатывает, когда в топик ROS /rmotor_cmd приходит сообщение,
//она задает скорость вращения вала правого мотора
void set_rspd( const std_msgs::Float32& msg)
{ if(msg.data > 0)//если скорость положительная
{
dir2=true;//направление вперед
analogWrite( ena, abs(msg.data) );//задаем скорость вращения мотора из сообщения ROS
digitalWrite( in1, LOW ); //валы моторов должны
digitalWrite( in2, HIGH ); //вращаться вперед
}
//если скорость <0
else{
dir2=false;//направление назад
analogWrite( ena, abs(msg.data) );//задаем скорость вращения мотора из сообщения ROS
digitalWrite( in1, HIGH ); //валы моторов должны
digitalWrite( in2, LOW );//вращаться назад
}

}

//Эта функция (subscriber callback function) срабатывает, когда в топик ROS /lmotor_cmd приходит сообщение,
//она задает скорость вращения вала левого мотора
void set_lspd( const std_msgs::Float32& msg)
{
if(msg.data > 0)//если скорость положительная
{ dir1=true;//направление вперед
analogWrite( ena1, abs(msg.data) );//задаем скорость вращения мотора из сообщения ROS
digitalWrite( in3, HIGH );//валы моторов должны
digitalWrite( in4, LOW );//вращаться вперед

}
//если скорость <0
else{
dir1=false;//направление назад
analogWrite( ena1, abs(msg.data) );//задаем скорость вращения мотора из сообщения ROS
digitalWrite( in3, LOW );//валы моторов должны
digitalWrite( in4, HIGH );//вращаться назад
}
}

//Создание сообщения ROS, данные из которого будут отправляться в топик
//В этом сообщении только одно поле - data, оно имеет тип данных int16.
//тип данных сообщения должен соответствовать типу данных топика
std_msgs::Int16 msg;
std_msgs::Bool line_msg;
//Объявление паблишеров ROS
ros::Publisher lwpub("lwheel", &msg);//паблишер для энкодера левого колеса
ros::Publisher rwpub("rwheel", &msg);//паблишер для энкодера правого колеса
ros::Publisher line_pub_right("line_sensor_right", &line_msg);//паблишер для правого датчика линии
ros::Publisher line_pub_left("line_sensor_left", &line_msg);// паблишер для левого датчика линии
//Объявление сабскрайберов ROS
ros::Subscriber<std_msgs::Float32> rmotor("rmotor_cmd", set_rspd );//сабскрибер на топик, в который приходит требуемая скорость правого колеса
ros::Subscriber<std_msgs::Float32> lmotor("lmotor_cmd", set_lspd );//сабскрибер на топик, в который приходит требуемая скорость левого колеса

void setup()
{
init_motors();//первоначальная настройка пинов
nh.initNode();//инициализация ноды ROS
//Сообщение ядру ROS о создании паблишеров в топик
nh.advertise(lwpub);
nh.advertise(rwpub);
nh.advertise(line_pub_right);
nh.advertise(line_pub_left);
//Сообщение ядру ROS о создании сабскрайберов на топик
nh.subscribe(rmotor);
nh.subscribe(lmotor);

}

void loop()
{
//в сообщение ROS пишем количество тиков энкодера правого колеса
msg.data = Position_enc2;
//отправляем сообщение в топик
rwpub.publish( &msg );
//в сообщение ROS пишем количество тиков энкодера левого колеса
msg.data = Position_enc1;
//отправляем сообщение в топик
lwpub.publish( &msg );
//отправляем данные с датчиков линии
line_msg.data = read_sensor(RGHT_LINE_SENSOR);
line_pub_right.publish( &line_msg );
 
line_msg.data = read_sensor(LEFT_LINE_SENSOR);
line_pub_left.publish( &line_msg );
nh.spinOnce();

delay(10);

}
