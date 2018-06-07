#include <ledc.h>

//trigger and echo pins for ultrasonic sensors
#define trigPin1 16
#define echoPin1 17
#define trigPin2 32
#define echoPin2 34
#define trigPin3 25
#define echoPin3 33

//output pins for buzzer
//setup includes two transistors between pins 26 and 27
#define pdcPin 27
#define soundPin 26

volatile long echo_start1 = 0;
volatile long echo_end1 = 0;  
volatile long echo_start2 = 0;
volatile long echo_end2 = 0; 
volatile long echo_start3 = 0;
volatile long echo_end3 = 0; 

volatile long echo_duration1 = 0;
volatile long echo_duration2 = 0; 
volatile long echo_duration3 = 0;

volatile int trigger_time_count1 = 0;
volatile int trigger_time_count2 = 0;
volatile int trigger_time_count3 = 0;

int dist1, dist2, dist3;

//piezo buzzer settings
int freq = 440;
int channel = 0;
int resolution = 8;

//initial buzzer state
bool buzzStatus = false;

//sensor distance settings
int sensordist = 700; //distance between sensors (equidistant) [mm]
int alpha = 15; //opening angle of sensors [deg]
int maxdistdefault = 150; //maximum sensor distance to prevent crosstalk [cm]

//working pin pairs for sensors: 16,17; 32/34; 25/33

/*
 * initialize pin states
 * load settings for ledc piezo control
 * check for valid sensor distance settings
 */
void setup() {
  Serial.begin (9600);
  pinMode(trigPin1, OUTPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(trigPin3, OUTPUT);
  pinMode(soundPin, OUTPUT);

//  pinMode(pdcPin, OUTPUT);
 
  pinMode(echoPin1, INPUT_PULLUP);
  pinMode(echoPin2, INPUT_PULLUP);
  pinMode(echoPin3, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(echoPin1), echo_interrupt1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(echoPin2), echo_interrupt2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(echoPin3), echo_interrupt3, CHANGE);

  ledcSetup(channel, freq, resolution);
  checkDistance();
}

/*
 * check sensor distance
 * warn if sensor distance would result in echo interferences
 */
void checkDistance() {
  float maxdist = 0.9*(tan((180-alpha)/2)*(d/2));
  if (maxdist < (maxdistdefault*10)) {
    Serial.println("check sensor distance and increase it or change maximum beep distance values");
  }
}

/*
 * measure echo travel duration for calculation of distance
 */
void echo_interrupt1() {
  switch (digitalRead(echoPin1)) {
    case HIGH:                                      
      echo_end1 = 0;                                 
      echo_start1 = micros();                        
      break;
      
    case LOW:                                       
      echo_end1 = micros();                          
      echo_duration1 = echo_end1 - echo_start1;        
      break;
  }
}

void echo_interrupt2() {
  switch (digitalRead(echoPin2)) {
    case HIGH:                                      
      echo_end2 = 0;
      echo_start2 = micros();                        
      break;
      
    case LOW:                                       
      echo_end2 = micros();                          
      echo_duration2 = echo_end2 - echo_start2;        
      break;
  }
}

void echo_interrupt3() {
  switch (digitalRead(echoPin3)) {
    case HIGH:                                      
      echo_end3 = 0;
      echo_start3 = micros();                        
      break;
      
    case LOW:                                       
      echo_end3 = micros();                          
      echo_duration3 = echo_end3 - echo_start3;        
      break;
  }
}

/*
 * calculate last ping time from each pin
 * re-read sensor ping after a delay 200ms
 */
void pingTime() {
  static int lastping1 = 0;
  static int lastping2 = 0;
  static int lastping3 = 0;
  
  int now = millis();

  int deltat1 = now - lastping1;
  int deltat2 = now - lastping2;
  int deltat3 = now - lastping3;

  if (deltat1 >= 200) {
    sendPing(trigPin1);
    lastping1 = now;
  }
  
  if (deltat2 >= 200) {
    sendPing(trigPin2);
    lastping2 = now;
  }
  
  if (deltat3 >= 200) {
    sendPing(trigPin3);
    lastping3 = now;
  }
}

/*
 * trigger pin echo measure for selected pin
 * set pin to low after ping signal was sent
 */
void sendPing(int pin) {
  digitalWrite(pin, LOW);
  delayMicroseconds(2);
  digitalWrite(pin, HIGH);
  delayMicroseconds(10); 
  digitalWrite(pin, LOW);
}

/*
 * configure beep status according to distance
 * Low: low beep for measurable distance
 * Med: medium beep for semi-critical distance
 * High: violent beep for critical distance to objects
 */
void beepLow() {
  if (buzzStatus == false) {
    digitalWrite(soundPin, HIGH);
    ledcAttachPin(pdcPin, channel);
    buzzStatus = true;    
  }
  ledcWriteTone(channel, 440);
}

void beepMed() {
  if (buzzStatus == false) {
   digitalWrite(soundPin, HIGH);
   ledcAttachPin(pdcPin, channel);
   buzzStatus = true;    
  }
  ledcWriteTone(channel, 880);
}

void beepHigh() {
  if (buzzStatus == false) {
   digitalWrite(soundPin, HIGH);
   ledcAttachPin(pdcPin, channel);
   buzzStatus = true;    
  }
  ledcWriteTone(channel, 1760);
}

/*
 * turn off piezo buzzer in case distance is not measurable or non-critical
 */
void beepOff() {
  if (buzzStatus == true) {
   digitalWrite(soundPin, LOW);
  }
   buzzStatus = false;
   Serial.println("beepoff");
}

/*
 * calculate distance from echo duration
 * echo travel time is calculated for HC-SR04
 * trigger according beep signal
 */
void distBeep() {
  dist1 = echo_duration1 / 58;
  dist2 = echo_duration2 / 58;
  dist3 = echo_duration3 / 58;

  if ((dist1 <= 40 || dist2 <= 40 || dist3 <= 40) && (dist1 > 0 && dist2 > 0 && dist3 > 0)) {
    Serial.println(dist1);
    beepHigh();
  }

  else if ((dist1 <= 80 || dist2 <= 80 || dist3 <= 80) && (dist1 > 0 && dist2 > 0 && dist3 > 0)) {
    Serial.println(dist1);
    beepMed();
  }

  else if ((dist1 <= maxdistdefault || dist2 <= maxdistdefault || dist3 <= maxdistdefault) && (dist1 > 0 && dist2 > 0 && dist3 > 0)) {
    Serial.println(dist1);
    beepLow();
  }
  
  else {
   beepOff();
   Serial.println(dist1);
   Serial.println("beepOffelse");
  }
}

void loop() {
  pingTime();
  distBeep();
}
