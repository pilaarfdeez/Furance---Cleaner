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
//LIBRERIAS
#include <LiquidCrystal_I2C.h>
#include <Wire.h> 

//Librerías para la comunicación
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>

//----------------
//BOTON
#define BOTON1 2
Button boton1 = Button(BOTON1);

//----------------
//LEDs
#define led1 5
#define led2 4
#define led3 3

//----------------
//COMUNICACIÓN
const int pinCE = 9;
const int pinCSN = 10;
RF24 radio(pinCE, pinCSN);
const uint64_t pipe = 0xE8E8F0F0E1LL;
float data[4];

//----------------
//LCD
LiquidCrystal_I2C lcd(0x27,16,2);

//----------------
//Variables necesarias para el desarrollo del proyecto
int seconds = 0;
int vecesBoton = 0;
int num = 0;
bool pulsado = false;
int v;
int Tumbral = 19;
int tespera = 2000;

//----------------
//FUNCIONES
int botonPulsado(){
  /*Detectar si el botón ha sido pulsado y según las veces que se haya pulsado establecer una variable a diferentes valores, de esta manera, aparecerá diferentes valores
  en la pantalla*/
  if (boton1.pressed()){
    pulsado = true;
    lcd.clear();
  }
  if (pulsado == true && num == 0){
    num = 1;
    digitalWrite(led3, LOW);
    digitalWrite(led1, HIGH);
    //Mostrará la temperatura y la velocidad
    //Serial.println("num= 1");
    lcd.setCursor(0,0);
    lcd.print("Temp y velc:");
    pulsado = false;
    
  }
  if (pulsado == true && (num == 1)){
    num = 2;
    digitalWrite(led2, HIGH);
    digitalWrite(led1, LOW);
    //Serial.println("num= 2");
    //Mostrará el número de veces que ha funcionando el sistema
    lcd.setCursor(0,0);
    lcd.print("Limpezas hechas:");
    
    pulsado = false;
  }
  if (pulsado == true && num == 2){
    num = 0;
    digitalWrite(led3, HIGH);
    digitalWrite(led2, LOW);
    //Serial.println("num= 3");
    //Mostrará el tiempo de cuando se inicio el motor en el otro arduino
    lcd.setCursor(0,0);
    lcd.print("Duracion (s):");
    
    pulsado = false;
  }
  return num;
}

void inicioLCD(){
  /*Escribir las primeras palabras en la pantalla*/
  lcd.print("Hola!");
  lcd.setCursor(0, 1);
  lcd.print("Hoy es 19/12/22");
  lcd.setCursor(0,0);
  delay(tespera);
  lcd.clear();
  lcd.print("En poco comenzará");
  lcd.setCursor(0, 1);
  lcd.print ("la limpieza");
  delay(tespera);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ha empezado!");
}

void obtenerInfo(){
  /*Recibir la información que está transmitiendo el otro arduino*/
   bool ok = radio.available();
  if (ok){    
      radio.read(data, sizeof data);
      //Temperatura
      Serial.print("Dato0= " );
      Serial.print(data[0]);
      //Numero de repeticiones
      Serial.print("Dato1= " );
      Serial.print(data[1]);
      Serial.println("");
      //Duración
      Serial.print("Dato2= " );
      Serial.print(data[2]);
      Serial.println("");
   }
  }

int calcularVelocidad(){
  /*establece una velocidad según la tempetura que se haya medido*/
  int velocidad;
  int temp = data[0];
  velocidad = map(temp, 10, 30, 50, 255);
  if (temp < Tumbral) velocidad = 0;
  return velocidad; 
}

void mostrardatosLCD(int num){
  /*Una vez se ha recibido la información se procederá a mostrarlos en la pantalla según el orden especificado*/
  int T, t1, repes;
  T = data[0];
  t1 = data[1]/1000;
  repes = data[2];
  
  if (num == 1){ //Mostrar la temperatura y la velocidad calculada
  lcd.setCursor(0,1);
  lcd.print(T);
  v = calcularVelocidad();
  lcd.setCursor(7,1);
  lcd.print(v);
  }
    if (num == 2){ //Mostrar el número de repeticiones
  lcd.setCursor(0,1);
  lcd.print(repes);
  }
    if (num == 0){ //Mostrar el tiempo que se ha recibido
  lcd.setCursor(0,1);
  lcd.print(t1);
  }
  
}



void setup()
{
  //inicializar boton
  boton1.begin(); 
  //inicializar LCD
  lcd.begin();
  inicioLCD();
  //inicializar puertoSerie
  Serial.begin(9600);
  //inicializar LEDs
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  //inicializar radio
   radio.begin();
   radio.openReadingPipe(1, pipe);
   radio.startListening();
}


void loop()
{
 vecesBoton = botonPulsado(); //conocer el número de veces que se ha pulsado el botón y mostrar un contenido u otro
 obtenerInfo(); //recibir la información
 mostrardatosLCD(vecesBoton); //mostrar dicha información según el número de veces que se haya pulsado
  delay(10);
}
