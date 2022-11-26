#include "RTClib.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


#define BTN_BACKLIGHT 2  //referencia para el boton. GPIO 33.
#define BTN_CHEER 3
#define analogInput A7   //referencia para el pin al que va conectado el divisor resistivo de ~0v - ~3.3v (ponele, deberia ser asi)
#define Entradainte A6    //referencia para el pin al que va conectado el AMPERIMETRO de efecto hall
#define TM_CHAR 60

/*
      notas:
      CODEADO CASI ENTERAMENTE POR ALAN VOLKER, CON INSPIRACIONES DE NICOLAS DUARTE.
      ESTE CODIGO VA A SER MEJORADO -- VA A SER ADAPTADO PARA CORRER EN UN MEJOR MICRO ( PREFERIBLMENTE UN MICRO NO-AVR, COMO EL ESP-32)
      MEJORAS:
        - COMUNICACION LORA ( +WEB SERVER )
        - MENU CON VARIOS BOTONES EN EL TABLERO DEL AUTO
        - MEJORES CALCULOS ( SI ES QUE SE PUEDE ), CON MEJORES SENSORES.


*/
String dias[7] = {"Dom", "Lun", "Mar", "Mie", "Jue", "Vie", "Sab"};
String frases[9] = {
    "Vamos! Tu puedes",
    "El de adelante es tu rival.",
    "Recuerda que al tomar una curva, debes de ir desde afuera, hacia dentro, y hacia afuera!",
    "Muy sugerente, pero por favor, no toques bajo ninguna circunstancia las baterias del auto, Podrian Explotar!",
    "Promo 21         AUTOCONTACTO",
    "Duro 2hs corriendo - AUTOCONTACTO",
    "Recuerda: Si le tiras el auto a un piloto rival, puede que salga herido.",
    "Si pudieses ir a 100km/h en esto...Lo harias?",
    "Te aseguro que si lees esto, las chances de que te hagas pomada contra la pared suben en un 98.2%"
};

typedef enum  {
  Triste,
  Feliz,
} EstadoPiloto;
bool backlight_state = true;

int getBateriaRestante();
float getCorriente();
void toggleDeBacklight();
void comprobacionDeBotones();
void verHora();
void bromita();
void caritasTristes();
EstadoPiloto estado = Feliz;
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
byte tool[8] = {
  B11100,
  B01000,
  B01111,
  B00101,
  B11111,
  B10100,
  B11100,
  B00111,
};
byte feliz[8] = {
  B00000,
  B10001,
  B10001,
  B00000,
  B10001,
  B10001,
  B11111,
  B00000,
};
byte triste[8] = {
  B00000,
  B10001,
  B10001,
  B00000,
  B11111,
  B10001,
  B10001,
  B00000,
};
int previo = 0;
float previoCorriente = 0.0;
void setup() {
  // initialize Serial Monitor
  Serial.begin(9600);
  pinMode(analogInput, INPUT);
  pinMode(Entradainte, INPUT);
  pinMode(BTN_BACKLIGHT, INPUT_PULLUP);
  pinMode(BTN_CHEER, INPUT_PULLUP);
  if (! rtc.begin()) {
    //Serial.println("Couldn't find RTC");
    //Serial.flush(); sin esta linea, aparentemente tira basura rtc.now()
    while (1) delay(10);
  }
  lcd.init();
  lcd.backlight();  //default.
  lcd.createChar(0, tool);
  lcd.createChar(1, feliz);
  lcd.createChar(2, triste);
  lcd.setCursor(0, 0);
  lcd.print("DESAFIO ECO YPF");
  lcd.setCursor(1, 1);
  lcd.print("Status: ON");
  delay(2000);
  lcd.clear();
  //timer
}

void loop() {
  int batRest = getBateriaRestante();
  if ((batRest > 9 && previo < 9) || (previo > 9 && batRest < 9) || (previo > 99 && batRest < 99) || (previo == 0 && batRest > 0) || (previo == 100 && batRest < 100)) {
    lcd.clear();
  }
  lcd.setCursor(0, 0);
  lcd.print("Bat rest: ");
  if (batRest == 0) {
    lcd.print("N/A");
  }
  else {
    lcd.print(batRest);
    lcd.print("%");
  }

  lcd.setCursor(13, 1);
  lcd.write(0);
  lcd.print(":");
  switch (estado) {
    case Feliz:
      lcd.write(1);
      break;
    case Triste:
      lcd.write(2);
      break;
    default:
      lcd.print("6");
      break;
  }
  unsigned long startTime = millis();
  while ((millis() - startTime) < 1000) { //por 1 segundos actualizo la corriente solo la corriente.
    float corriente = getCorriente();
    if(previoCorriente > 10.0 && corriente < 10.0) {
      lcd.setCursor(11, 1);
      lcd.print(" ");
    }
    lcd.setCursor(1, 1);
    lcd.print("Amps: ");
    lcd.print(corriente, 2);  //Inserte amperimetro aqui.
    comprobacionDeBotones();
    previoCorriente = corriente;
  }
  previo = batRest;
  if (backlight_state) {
    estado = Feliz;
  }
  else {
    estado = Triste;
  }
}
void comprobacionDeBotones()
{
  if (!digitalRead(BTN_BACKLIGHT)) {  //PARA APAGAR LA BACKLIGHT --> AHORRO DE ENERGIA.
    unsigned long startTime = millis();
    //    Serial.print("start: ");
    //    Serial.println(startTime);
    //    Serial.println("BOTON BACKLIGHT PRESIONADO");

    while (!digitalRead(BTN_BACKLIGHT)) {
      if (!digitalRead(BTN_CHEER)) {
        while(!digitalRead(BTN_CHEER) || !digitalRead(BTN_BACKLIGHT));
        bromita();
        return;
      }
      if(millis() - startTime > 1000) break;
    }
    //Serial.print("Ms: ");
    //Serial.println(millis() - startTime);
    if (millis() - startTime < 750) {
      toggleDeBacklight();
      return;
    }
    lcd.clear();
    verHora();
    lcd.clear();


  }
  else if (!digitalRead(BTN_CHEER)) {
    while (!digitalRead(BTN_CHEER)) {
      if (!digitalRead(BTN_BACKLIGHT)) {
        while(!digitalRead(BTN_CHEER) || !digitalRead(BTN_BACKLIGHT));
        bromita();
        return;
      }
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    //randomSeed(analogRead(A3));
    int n = random(0, 10);
    Serial.print("Numero sorteado: ");
    Serial.println(n);
  if (n == 9) {
    lcd.print("Autodestruccion");
    lcd.setCursor(0, 1);
    lcd.print("en: ");
    for (int i = 3; i > 0; i--) {
      lcd.setCursor(4, 1);
      lcd.print(i);
      delay(1000);
    }
    lcd.clear();
    return;
  }
  String frase = "";
   frase += frases[n];
   Serial.println(frases[n]);
   Serial.println(frase);
      if (frase.length() > 15) {
        while (frase.length()) { //mientras haya algo que imprimir...
          if (frase.length() > 15) {
            for (int i = 0; i < 16; i++) {
              lcd.print(frase[i]);
              delay(TM_CHAR);
            }
            frase.remove(0, 15);
          }
          else {
            for (int i = 0; i < frase.length(); i++) {
              lcd.print(frase[i]);
              delay(TM_CHAR);
            }
            break;
          }
          lcd.setCursor(0, 1);
          if (frase.length() > 15) {
            for (int i = 0; i < 16; i++) {
              lcd.print(frase[i]);
              delay(TM_CHAR);
            }
            lcd.setCursor(0, 0);
            frase.remove(0, 15);
          }
          else {
            for (int i = 0; i < frase.length(); i++) {
              lcd.print(frase[i]);
              delay(TM_CHAR);
            }
            break;
          }
          delay(850); //cuando se llene la pantalla
          lcd.clear();
        } //while(novacia)
      } //si la frase tenia mas de 15 en primer lugar.
      else {
        lcd.print(frase);
      }

    delay(2000);
    lcd.clear();
  }
}
void toggleDeBacklight() {
  if (backlight_state) {
    lcd.noBacklight();
  } else {
    lcd.backlight();
  }
  backlight_state = !backlight_state;
}
int getBateriaRestante() {
  float R1 = 10000000.0;  //  R1 (10M) Valor de la resistencia R1 del divisor de tensión
  float R2 = 1000000.0;    //  R2 (1M) Valor de la resistencia R2 del divisor de tensión
  long value = 0;
  for (int i = 0; i != 5000; i++) {
    value += analogRead(analogInput);
  }
  value /= 5000;                                          //promedio de las mediciones de tension.
  int bateriaRestante = map(value, 795, 950, 0, 99);      // valores tentativos calculados por Dri. 3,81v-4,7v
  float vin = ((value * 5) / 1023.0) / (R2 / (R1 + R2));  // Cálculo para obtener Vin del divisor de tensión.

//  Serial.print("Vin: ");
//  Serial.println(vin);
//  Serial.println(analogRead(analogInput));                  // DEBUG.
//  Serial.print("Bateria: ");
//  Serial.println(bateriaRestante);
  if (bateriaRestante > 100) bateriaRestante = 100;
  else if (bateriaRestante < 0) bateriaRestante = 0;
  //SE MIDIO LA TENSION.
  return bateriaRestante;
}
float getCorriente() {
  float sensibilidad = 0.066;
  long tensionSensor = 0;
  float corriente = 0.0;
  for (int i = 0; i != 1000; i++) {
    tensionSensor = analogRead(Entradainte) * (5.0 / 1023.0);
    corriente += (tensionSensor - 2.5) / sensibilidad;
  }
  corriente /= 1000;
  //Serial.print("corriente: ");
  //Serial.println(corriente);
  if (corriente < 0) corriente = 0;
  return corriente;
}
void verHora() {
  unsigned long startTime = millis();
  bool apretado = false;
  if(!digitalRead(BTN_BACKLIGHT)) apretado = true;
  while (millis() - startTime < 10000) { //10 segundos de ver la hora
    DateTime now = rtc.now();
    lcd.setCursor(0, 0);
    lcd.print(now.day());
    lcd.print("/");
    lcd.print(now.month());
    lcd.print("/");
    lcd.print(now.year());
    lcd.print(" ");
    lcd.print(dias[now.dayOfTheWeek()]);
    lcd.setCursor(0, 1);
    if (now.hour() < 10 ) {
      lcd.print("0");
      lcd.print(now.hour());
    } else lcd.print(now.hour());
    lcd.print(':');
    if (now.minute() < 10 ) {
      lcd.print("0");
      lcd.print(now.minute());
    } else lcd.print(now.minute());
    lcd.print(':');
    if (now.second() < 10 ) {
      lcd.print("0");
      lcd.print(now.second());
    } else lcd.print(now.second());
    if(digitalRead(BTN_BACKLIGHT)) apretado = false;
    if(apretado) continue;
    if (!digitalRead(BTN_BACKLIGHT)) {
      while (!digitalRead(BTN_BACKLIGHT));
      return;
    }
  }
}
void bromita() {
  lcd.clear();
  for(int i=0;i<75;i++){
    if(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER)){
      while(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER));
        return;
    }
    
    delay(10);
  }
  
  caritasTristes();
  for(int i=0;i<85;i++){
    if(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER)){
      while(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER));
        lcd.clear();
        return;
      }
    delay(10);
  }
  
  lcd.setCursor(1, 0);
  String str[2] = {"Auto", "destruccion"};
  for (int i = 0; i < str[0].length(); i++) {
    lcd.print(str[0][i]);
    for (int i = 0; i < 75; i++)
    {
      if (!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER)) {
        while(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER));
        lcd.clear();
        return;
      }
      delay(10);
    }

  }
  lcd.setCursor(2, 1);
  for (int i = 0; i < str[1].length(); i++) {
    lcd.print(str[1][i]);
    for (int i = 0; i < 40; i++) {
      if (!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER)) {
        while(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER));
        lcd.clear();
        return;
      }
      delay(10);
    }

  }
  //Se mostro autodestruccion
  caritasTristes();
  for(int i=0;i<30;i++){
    if(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER)){
      while(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER));
        lcd.clear();
        return;
      }
    delay(10);
  }
  
  lcd.setCursor(1, 0);
  lcd.print("e");
  for(int i=0;i<50;i++){
    if(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER)){
      while(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER));
        lcd.clear();
        return;
      }
    delay(10);
  }
  
  lcd.print("n");
  for(int i=0;i<50;i++){
    if(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER)){
        while(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER));
        lcd.clear();
        return;
      }
    delay(10);
  }
  
  for (int i = 5; i > 0; i--) {
    lcd.setCursor(7, 1);
    lcd.print(i);
    for(int j=0;j<95;j++){
      if(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER)){
        while(!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER));
        lcd.clear();
        return;
      }
      delay(10);
    }
    
  }

  lcd.clear();

}

void caritasTristes() {
  lcd.setCursor(0, 0);
  for (int i = 0; i < 16; i++) {
    lcd.write(2);
    if (!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER))
      return;

    delay(TM_CHAR);
  }
  lcd.setCursor(0, 1);
  for (int i = 0; i < 16; i++) {
    lcd.write(2);
    if (!digitalRead(BTN_BACKLIGHT) || !digitalRead(BTN_CHEER))
      return;

    delay(TM_CHAR);
  }
}






//*/
