#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

int tick = 25;

class CrossingLight{
  private:
    int pin1;
    int pin2;
    int delay_ticks;
  public:
    int trigger_time;
    int counter;
    bool is_on = false;
    
    void start(int p1, int p2, int d_t){
      pin1 = p1;
      pin2 = p2;
      delay_ticks = d_t;
      pinMode(pin1, OUTPUT);
      pinMode(pin2, OUTPUT);
      
      digitalWrite(pin1, HIGH);
      digitalWrite(pin2, HIGH);
      delay(1000);
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, LOW);
    }
    void Triggered(int t){ trigger_time = t+80; }
    void TakeAction(){
        if(trigger_time>0){
          if(counter<delay_ticks) counter++;
          else{
            if(is_on){
              digitalWrite(pin1, LOW);
              digitalWrite(pin2, HIGH);
              counter = 0;
              is_on = false;
            }
            else{
              digitalWrite(pin1, HIGH);
              digitalWrite(pin2, LOW);
              counter = 0;
              is_on = true;
            }
          }
          trigger_time--;
      }
      else{
        digitalWrite(pin1, LOW);
        digitalWrite(pin2, LOW);
      }  
    }  
};

class Barrier {
  private:
    Servo servo;
    int step_distance;
    int first_move_delay;
    int first_move_delay_temp;
    int waiting_time;

    int pos_close;
    int pos_open;
    int pos_actual;
    
  public:
    int opening = false;
    int waiting = false;
    int closing = false;
    int triggered = false;
    
    void start(int servo_pin, int p_c, int p_o, int s_d, int d = 0){
      servo.attach(servo_pin);
      pos_close = p_c;
      pos_open = p_o;
      step_distance = s_d;
      first_move_delay = d;
      first_move_delay_temp = first_move_delay;

      servo.write(pos_open);
      pos_actual = pos_open;    
    }
    int getWaitingTime(){ return waiting_time; }
    void Triggered(int t){
      waiting_time = t;
      triggered = true;
      }
    void TakeAction(){
      //jeśli nie jest zamknięty (w pełni) to ustaw zamykanie (jeśli otwierał to przerwij)
      if((!waiting) && (triggered)){
        closing = true;
        opening = false;
        triggered = false;
      }
      //Jesli ustawiono zamykanie to zamykaj szlaban o 1 krok do czasu az osiagnie pozycje zamkniecia.
      if(closing){
        if(first_move_delay_temp>0) first_move_delay_temp--;
        else{
          if(pos_actual - step_distance <= pos_close) pos_actual += step_distance;
          else{
            pos_actual = pos_close;
            closing = false;
            waiting = true;
          }
        }
      }
      //Czekaj w zamkniecu do poki nie skonczy sie odliczanie i odejmij od odliczania jeden.
      else if(waiting){ 
        if(first_move_delay_temp!= first_move_delay) first_move_delay_temp = first_move_delay; //dodatkowo przywraca czas opoznienia rozpoczecia zamykania
        if(waiting_time>0) waiting_time--;
        else{
          waiting = false;
          opening = true;
        }
      }
      //Jesli ustawiono otwieranie to otwieraj szlaban o 1 krok do czasu az osiagnie pozycje otwarcia.
      else if(opening){
        if(pos_actual + step_distance >= pos_open) pos_actual -= step_distance;
        else{
          pos_actual = pos_open;
          opening = false;
        }
      }
      servo.write(pos_actual);
    }
};

class Detection{
  private:
    int trigger_pin;
    int echo_pin;
    bool after_det = false;
    
  public:
    int ticks;
    int last_detection_ticks;
    int distance;
    int return_time;

    void start(int t_p, int e_p){
      trigger_pin = t_p;
      echo_pin = e_p;
      pinMode(trigger_pin, OUTPUT);
      pinMode(echo_pin, INPUT);
    }
    int getDistance(){
      digitalWrite(trigger_pin, LOW);
      delayMicroseconds(2);
      digitalWrite(trigger_pin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigger_pin, LOW);
      return_time = pulseIn(echo_pin, HIGH);
      distance = return_time/58;
      return distance;
    }
    int getDetectionTime(int val, int range){
      if((distance<=val+range) && (distance>=val-range)){
          ticks++;
          after_det = true;
          last_detection_ticks = ticks;
          return last_detection_ticks;
      }
      else if(after_det){
        ticks=0;  
        after_det = false;
      }
    } 
  
};


Barrier szlaban1;
CrossingLight swiatlo1;
Detection czujnik;
LiquidCrystal_I2C  lcd(0x27,2,1,0,4,5,6,7);

void setup() {
  Serial.begin (9600);
  
  lcd.begin (16,2); // for 16 x 2 LCD module
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home ();
  lcd.print("Uruchamianie...");
  
  czujnik.start(11, 12);
  
  szlaban1.start(10, 70, 3, 2, 12); //pin | open pos | close pos | move per 50ms | delay (default 0)
  
  swiatlo1.start(8, 9, 12); //pin | ticks delay betwwen change (~500ms)
  
  lcd.home ();
  lcd.print("Gotowy do pracy");
  lcd.setCursor (7,1);
  lcd.print(":)");
}

void loop() { //<- pelta wykonywana przez caly czas dzialania Arduino.

  delay(tick); //<- ustawmy tick na 25 milisekund (caly loop wykonywac sie bedzie co 25 milisekund)
  int distance = czujnik.getDistance(); //<- funkcja zwracajaca odleglosc w cm zmierzona przez czujnik odleglosci.
  //Serial.println(distance);
  /*Serial.print(" | ");
  Serial.print(szlaban1.getWaitingTime());
  Serial.print(" | ");
  Serial.print(szlaban1.closing);
  Serial.print(" | ");
  Serial.print(szlaban1.waiting);
  Serial.print(" | ");
  Serial.print(szlaban1.opening);
  Serial.println();
  Serial.print(swiatlo1.is_on);
  Serial.print(" | ");
  Serial.print(swiatlo1.counter);
  Serial.print(" | ");
  Serial.print(swiatlo1.trigger_time);
  Serial.println();*/
  if(distance<10){
    szlaban1.Triggered(100);
    swiatlo1.Triggered(100);
    
  } 
  szlaban1.TakeAction();
  swiatlo1.TakeAction();
  czujnik.getDetectionTime(8,4);

  /*
  if(nowy_czas_przejazdu != czas_przejazdu){
    lcd.setCursor (0,1);
    lcd.print("Predkosc:       ");
    lcd.setCursor (10,1);
    lcd.print(nowy_czas_przejazdu);
    czas_przejazdu = nowy_czas_przejazdu;
  }
  if(nowa_odleglosc != odleglosc){
    lcd.setCursor (0,0);
    lcd.print("Odleglosc:       ");
    lcd.setCursor (11,1);
    lcd.print(nowa_odleglosc);
    odleglosc = nowa_odleglosc;
  }*/
  lcd.clear();
  lcd.home ();
  lcd.print("Odleglosc: ");
  lcd.print(distance);
  lcd.setCursor (0,1);
  lcd.print("Przejazd: ");
  lcd.print(czujnik.last_detection_ticks);
  
}
