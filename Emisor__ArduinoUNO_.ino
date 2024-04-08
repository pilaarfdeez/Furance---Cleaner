
/// Código biblioteca Button para Tinkercad
class Button
{
  public:
    Button(uint8_t pin);
    void begin();
    bool read();
    bool toggled();
    bool pressed();
    bool released();
    bool has_changed();

    const static bool PRESSED = LOW;
    const static bool RELEASED = HIGH;

  private:
    uint8_t  _pin;
    uint16_t _delay;
    bool     _state;
    bool     _has_changed;
    uint32_t _ignore_until;
};

Button::Button(uint8_t pin)
:  _pin(pin)
,  _delay(100)
,  _state(HIGH)
,  _ignore_until(0)
,  _has_changed(false)
{
}

void Button::begin()
{
  pinMode(_pin, INPUT_PULLUP);
}

//
// public methods
//

bool Button::read()
{
  // ignore pin changes until after this delay time
  if (_ignore_until > millis())
  {
    // ignore any changes during this period
  }

  // pin has changed
  else if (digitalRead(_pin) != _state)
  {
    _ignore_until = millis() + _delay;
    _state = !_state;
    _has_changed = true;
  }

  return _state;
}

// has the button been toggled from on -> off, or vice versa
bool Button::toggled()
{
  read();
  return has_changed();
}

// mostly internal, tells you if a button has changed after calling the read() function
bool Button::has_changed()
{
  if (_has_changed == true)
  {
    _has_changed = false;
    return true;
  }
  return false;
}

// has the button gone from off -> on
bool Button::pressed()
{
  if (read() == PRESSED && has_changed() == true)
    return true;
  else
    return false;
}

// has the button gone from on -> off
bool Button::released()
{
  if (read() == RELEASED && has_changed() == true)
    return true;
  else
    return false;
}

//----------------

//LIBRERÍAS
#include <Adafruit_Sensor.h>
#include <DHT.h> //libreria para el sensor
#include <EEPROM.h> //libreria para guardar en la eeprom


//librerías para la COMUNICACIÓN emisor y receptor:
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>

//comunicación IR
#include <IRremote.h>
#include <IRremoteInt.h>

//----------------
//SENSOR
#define DHTPIN 2 //donde se encuentra el sensor
#define DHTTYPE DHT11 //tipo de sensor

DHT dht(DHTPIN, DHTTYPE); // Inicializamos el sensor DHT11
//----------------
//MOTOR
int en = 5;
int in1 = 3;
int in2 = 4;
//----------------
//COMUNICACIÓN
#define CE_PIN 9 //pin Chip Enable
#define CSN_PIN 10 //pin  Chip Select

const uint64_t pipe = 0xE8E8F0F0E1LL;
RF24 radio(CE_PIN, CSN_PIN);
float data[3]; //vector donde se van a enviar los datos
//----------------
//IR
int receiver = 8; 
IRrecv irrecv(receiver);     // create instance of 'irrecv'
decode_results results;   
//----------------
//variables para el desarrollo del proyecto
const byte Tmax_direc = 0;
float Tmax;
float T, v;
int Tumbral = 19;
float estado;
bool encendido, inicializado, power;
unsigned long t1;
unsigned long duracion = 30000;
int numeroRepes = 0;

//----------------
//LED
#define led1 14

//----------------
//Buzzer
#define pinBuzzer 6
int tespera = 1000;

//----------------
//BOTON
#define BOTON 7
Button boton = Button(BOTON);

//----------------
//FUNCIONES

int leerTemperatura(){
  /*medirá la salida del sensor y la transformará en grados centígrados*/
  float temp;
  delay(5000); //cada 5seg mida el valor de la temperatura
  Tmax = EEPROM.read(Tmax_direc); //lee la temperatura almacenada en la EEPROM y si está el menor a la medida por el sensor, se sustituye por la mayor
  temp = dht.readTemperature();
  temp=(int)temp;
  if (temp > Tmax){
    Tmax = temp;
    EEPROM.update(Tmax_direc, Tmax);
    }
  Serial.print("T = ");
  Serial.println(temp);
  return temp;
  }

void inicializarTemperatura(){
  /*Escribe en la variable Tmax, en valor que hay en la EEPROM y cada vez que se ejecute el programa, la lee*/
  Tmax = dht.readTemperature();
  EEPROM.write(Tmax_direc, Tmax);
  }


int velocidadMotor(int temp){
  /*establece una velocidad según la tempetura que se haya medido*/
  int velocidad;
    int velMin = 50;
    int velMax = 255;
    velocidad = map(temp, 10, 30, 50, 255);
    //se satura la velocidad para que no alcance valores que no es capaz de llegar
    if (velocidad < velMin){
      velocidad = velMin;
      }
    if (velocidad > velMax){
      velocidad = velMax;
     }
    Serial.print("V = ");
    Serial.println(velocidad);
    return velocidad;
  }

void encenderMotor(int velocidad){
  /*Según una velocidad calculada, inicia el motor a esa*/
    analogWrite(en, velocidad); // Activamos Motor1
    digitalWrite(in1, HIGH);// Arrancamos
    digitalWrite(in2, LOW);
    Serial.println("motor encendido");
  }

void desactivarMotor(){
  /*Escribe en la variable enable un LOW para apagar el motor*/
    Serial.println("motor apagado");
    inicializado = false;
    digitalWrite(en, LOW); 
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

int movMotor(float temp){
  /*Indica el funcionamiento de nuestro sistema, si se ha superado una temperatura umbral, previamente establecida, se iniciará el motor a una velocidad.
  Si no se cumple la condición mantendrá apagado el motor.
  Una vez se ha superado esta condición, se establecerá la variable estado a 1, indicando que esta funcionando y, si la nueva medida de la temperatura es 
  menor que la umbral, se desactivará el motor y emitirá una señal sonora*/
  if (temp > Tumbral){
      v = velocidadMotor(T);
      Serial.println("temp>Tumbral");
      estado = 1;      
      //Serial.print("estado=");
      //Serial.println(estado); 
      encenderMotor(v);
      delay(2000);
    }
  else desactivarMotor();

  if (estado == 1 && temp < Tumbral){
    sonidoFin();
    estado = 0;
    //Serial.print("estado=");
    //Serial.println(estado);
    desactivarMotor();
    }
    return v, estado;
  }

void inicializarTiempo(float temp){
  /*función para conocer el instante en el que empieza el movimiento del motor*/
  if (inicializado == false && (temp > Tumbral)){
    t1 = millis();
    inicializado = true;
    //Serial.println(t1);
    }
  }

bool botonPulsado(){
  /*Detectar si el botón ha sido pulsado y cambiar el valor de la variable encendido. Además, si esto ocurre, se sumará 1 a la variable número de repeticiones*/
  if (boton.pressed()){
    encendido = true;
    Serial.println(encendido);
    numeroRepes += 1;
  }
  return encendido;
  }

bool detectarIR(){
  /*Detectar si el módulo IR ha recibido el valor de POWER ha sido pulsado y cambiar el valor de la variable power. Además, si esto ocurre, se sumará 
  1 a la variable número de repeticiones*/
if (irrecv.decode(&results)) // have we received an IR signal?
  {
    //translateIR(); 
    if (results.value == 0xFFA25D){ //0xFFA25D = POWER, botón rojo del mando
      power = 1;      
      numeroRepes += 1;
    }
    irrecv.resume(); // receive the next value
  }
  return power;
}  

void Info(float temperatura, float velocidad, float repeticiones){
  /*Se encarga de mandar la información al otro arduino, enviará la temperatura, el instante de tiempo en el que ha empezado y el número de veces que se
  ha iniciado el funcionamiento del sistema*/
  data[0] = temperatura;
 data[1] = t1; 
 data[2] = repeticiones;
  
   radio.write(data, sizeof data);
   delay(1000);
  }

void sonidoFin(){
  /*Emitará un pitido con cuatro repeticiones a una frecuencia determinada*/
  int frecuencia = 400;
    
  for (int i=0; i<4; i++){
    tone(pinBuzzer, frecuencia);
    delay(tespera);
    noTone(pinBuzzer);
    delay(tespera);
  }
}


/*
void translateIR() // takes action based on IR code received

// describing Remote IR codes 

{

  switch(results.value)
    
  {
  case 0xFFA25D: Serial.println("POWER"); break;
  case 0xFFE21D: Serial.println("FUNC/STOP"); break;
  case 0xFF629D: Serial.println("VOL+"); break;
  case 0xFF22DD: Serial.println("FAST BACK");    break;
  case 0xFF02FD: Serial.println("PAUSE");    break;
  case 0xFFC23D: Serial.println("FAST FORWARD");   break;
  case 0xFFE01F: Serial.println("DOWN");    break;
  case 0xFFA857: Serial.println("VOL-");    break;
  case 0xFF906F: Serial.println("UP");    break;
  case 0xFF9867: Serial.println("EQ");    break;
  case 0xFFB04F: Serial.println("ST/REPT");    break;
  case 0xFF6897: Serial.println("0");    break;
  case 0xFF30CF: Serial.println("1");    break;
  case 0xFF18E7: Serial.println("2");    break;
  case 0xFF7A85: Serial.println("3");    break;
  case 0xFF10EF: Serial.println("4");    break;
  case 0xFF38C7: Serial.println("5");    break;
  case 0xFF5AA5: Serial.println("6");    break;
  case 0xFF42BD: Serial.println("7");    break;
  case 0xFF4AB5: Serial.println("8");    break;
  case 0xFF52AD: Serial.println("9");    break;
  case 0xFFFFFFFF: Serial.println(" REPEAT");break;  

  default: 
    Serial.println(" other button   ");

  }// End Case

  delay(500); // Do not get immediate repeat
}*/


void setup() {
 Serial.begin(9600);
 // Inicializar el sensor de temperatura
 dht.begin(); 
 inicializarTemperatura();
 //inicializar los pines del motor
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(en, OUTPUT);
  //inicializar comunicación
  radio.begin();
  radio.openWritingPipe(pipe);//abrir canal de comunicacion pq es el emisor
  //inicializar boton
  boton.begin();
  //inicializar led
  pinMode(led1, OUTPUT);
  //inicializar IR
  irrecv.enableIRIn(); // Start the receiver

}

void loop() {
  //comprobar si se ha pulsado alguna de las dos opciones
  encendido = botonPulsado();
  power = detectarIR();
 if (power == 1 or encendido == 1){ //si se ha pulsado, comienza el funcionamiento
  digitalWrite(led1, HIGH); //encender el LED de funcionamiento
  delay(1000); //tiempo de espera para colocarlo y que empiece
  T = leerTemperatura(); //leer temperatura
  inicializarTiempo(T); 
  v, estado = movMotor(T); //establecer una velocidad y encender el motor o no
  Info(T, v, numeroRepes); //enviar la información
 //la otra forma en la que se apagaría el sistema es si ha superado una periodo de tiempo
  if ( (estado == 1) && (millis() >= (t1 + duracion))){
    Serial.println("estoy aqui");
    desactivarMotor(); //apagar el motor 
    sonidoFin();
    //ponemos las variables iniciales a false, de esta manera, cuando se vuelva a pulsar alguna de ellas, se volverá a iniciar el funcionamiento
    encendido = false;
    power = false;
    t1 += duracion;
    }
 }
 else digitalWrite(led1, LOW); //mantener el LED apagado ya que no esta funcionando el sistema

 delay(10);
}
