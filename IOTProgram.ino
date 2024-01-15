

// Included librarys
#include <LiquidCrystal.h>
#include <IRremote.h>
#include <IRremoteInt.h>
#include <SoftwareSerial.h>
#include <stdlib.h>

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2; //Ports the LCD Is Connected To
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

IRrecv irrecv(10); //IR detector connected to port 10
decode_results results; //stores Hex code from the IR detector

int Stage; //stores current state of device
float timesave; //stores time timer was started

String Hexcodes[] = {"c101e57b","9716be3f","3d9ae3f7","6182021b","8c22657b","488f3cbb","449e79f","32c6fdf7","1bc0157b","3ec3fc1b","f076c13b","e318261b"};// HEX IR codes off Remote (0-9buttons downbutton offbutton)
String TheCode; //store code use had entered
String Password = "9656"; //store correct code



SoftwareSerial ESP8266(13, 9); // RX, TX

float WifiTimeDelay = 0; //helps control delay between sending data to thingspeak

float Plotterdelay = 0;//adds a delay to the serial plotter to help stop it from being too fast
int motionsave = 0; //saves if motion has been detected in the plotter delay

int RemoteTranslate(String HexValue){ //Translates HEX code from IR sensor to Button Number.
  for(int i = 0; i < 12; i++){ //Loops through Hexcodes array
    if(Hexcodes[i] == HexValue){ //if Hexvalue matches Hex code in array
      return(i); //return the location of the code in the list
    }
  }
  return(12);//return 12 if no Hex code is found
}
void ClearLCD(int line, int coord1, int coord2) //clears specified section of LCD (Line your clearing on, Clear from coord 1 to coord 2)
{               
        lcd.setCursor(coord1,line); //set lcd cursor to the correct line and coord
        for(int i = coord1; i < coord2 && i < 20; i++) //move from coord one to coord two
        {
                lcd.print(" "); //clear digit on the coord
        }
}


void setup() { //runs at the start of the program
  Serial.begin(9600); //start serial monitor
  pinMode(7, INPUT); //set motion sensor port to Input
  pinMode(8, OUTPUT); //set Buzzer port to Output
  lcd.begin(16, 2); // set up the LCD's number of columns and rows:
  lcd.clear(); //clear the LCD screen
  lcd.print("No one here"); //write msg to screen
  irrecv.enableIRIn(); //start IR sensor
  Stage=0;//set device state to stage 0 

  //wifi connection code

  ESP8266.begin(115200);
  ESP8266.print("***VER:");
  delay(2000);
  ESP8266.println("AT+RST");
  delay(1000);
  ESP8266.println("AT+GMR");
  delay(1000);
  ESP8266.println("AT+CWMODE=3");
  delay(1000);
  ESP8266.println("AT+CWLAP");
  delay(1000);
  ESP8266.print("AT+CWJAP=\"NetworkName\",\"NetworkPassword\"\r\n"); //ADD Network Name and Password here
  ESP8266.setTimeout(5000);
}

void loop() { //runs continuously
  int motion = digitalRead(7); //read current state of motion sensor
  if(motion) //if motion has been detected
  {
    motionsave=motion; //save the detection
  }
  int Plottertime = 1000 - (millis() -Plotterdelay);//calc how long is left on the plotter delay
  if(Plottertime <= 0 || Plottertime > 1000){ //if delay has ended or int overflow has occured 
    Serial.println(motionsave); //plot data to seriel plotter
    motionsave=0;//reset motion save
    Plotterdelay=millis(); //reset the delay
  }
  if(Stage == 0){ //State Of Device: Waiting for motion
    digitalWrite(8, LOW);//Turn Buzzer Off
    if(motion){ //if motion sensor is triggered
      Stage=1; //set to countdown setup stage
    }
  }
  else if(Stage == 1){  //State Of Device: Setup for countdown stage
    lcd.clear(); //clear lcd screen
    lcd.setCursor(0, 0); //set lcd cursor to beginning
    lcd.print("Motion Detected"); //display msg on lcd 
    timesave=millis(); //save current time for counter
    Stage=2; //set satge to countdown stage
    TheCode=""; //reset the users input
    digitalWrite(8, HIGH); //turn on buzzer
  }
  else if(Stage == 2){ //State Of Device: Countdown Stage
    int counter = 20 - ((millis() - timesave)/1000); //Calculates how long is left on the timer
    if(counter < 0 || counter > 20){ //if the timer goes below 0 or int overflow has occured 
      counter=0; //set counter to 0
      Stage=3; //change stage to alarm setup
    }
    if(counter <  19){ //after 3 seconds turn buzzer off
      digitalWrite(8, LOW);//turn buzzer off
    }
    lcd.setCursor(0, 1);//set lcd cursor line to 2nd line
    if(counter <  10){//if smaller than 10 add a "0" to the digit
      lcd.print("0"+ String(counter)); //print current time left on the counter to LCD
    }
    else{
      lcd.print(counter); //print current time left on the counter to LCD
    }
  }
  else if(Stage == 3){//State Of Device: alarm stage Setup
    ClearLCD(0,0,20); //clear first line of lcd
    ClearLCD(1,0,2); //clear first counter section of lcd
    lcd.setCursor(0, 0); //set lcd cursor to beginning
    lcd.print("Security Alert");//Display msg on lcd
    digitalWrite(8, HIGH);//turn on buzzer
    Stage=4;//change to Alarmed stage
  }
  else if(Stage == 5)//State Of Device:alarm cooldown setup
  {
    lcd.clear();//clear LCD screen
    lcd.setCursor(0, 0);//set lcd cursor to beginning
    lcd.print("Alarm Cooldown");//display msg on lcd
    timesave=millis();//save current time for counter
    Stage=6; //set stage to alarm cooldown
    TheCode=""; //reset user input
    digitalWrite(8, LOW);//turn buzzer off
  }
  else if(Stage == 6){ //alarm cooldown
    int counter = 60 - ((millis() - timesave)/1000); //calculate time left on the counter
    if(counter < 0 || counter > 60){ //if counter is smaller than 0 or int overflow has occured 
      counter=0;//set counter to 0
      lcd.clear(); //clear the lcd
      lcd.setCursor(0, 0);//set lcd cursor to beginning
      lcd.print("No one here");//display msg on lcd
      Stage=0;//reset the device stage back to waiting for motion
    }
    lcd.setCursor(0, 1);//set lcd cursor to second line
    if(counter <  10 && Stage == 6){ //if stage is still 6 and counter is smaller than 10
      lcd.print("0"+ String(counter));//add a 0 to the time left and display it on lcd
    }
    else if(Stage == 6){//if stage is still 6 
      lcd.print(counter);//display current time left on counter
    }
  }

  if(Stage > 1 && Stage < 5)  //If in either stages 2 , 3 or 4 (Alarm stages) check for user input
  {
    if(irrecv.decode(&results)){ //if IR signal recieved
      int result = RemoteTranslate(String(results.value,HEX)); //Translate hexvalue and store button number
      if(result != 12){ //if valid button number
        if(result < 10){ //if button number is less than 10 (That means it's a 0-9 button on the remote)
          TheCode=TheCode+String(result);//add button number to current code inputed by the user
          ClearLCD(1,4,20);//clear any previous code writen to the screen
        }
        else if(result == 10 && TheCode.length() > 0){ //if button number is 10 (that means the back button was pressed) and there is user input to delete
          TheCode.remove(TheCode.length() - 1); //remove the last digit in the user input
          ClearLCD(1,4,20);//clear any previous code writen to the screen
        }
        lcd.setCursor(4, 1);//set lcd cursor to the section it displays the code in
        lcd.print("Code:" + TheCode); //display user input to lcd
        if(result == 11) //if button number is 11 (That means the power button / submit button has been pressed)
        {
          ClearLCD(1,4,20);//clear code section of lcd screen
          lcd.setCursor(4, 1);//set cursor to the section it displays the code in
          if(TheCode == Password){//if the code is correct
            Stage=5;//move onto the alarm cooldown setup stage
          }
          else{ //if the code is incorrect
            Stage=3;//set stage to alarm setup stage
            lcd.print("CodeInvalid"); //display msg in the code section of the lcd screen
            TheCode="";//reset the user input
          }
        }
      }
      irrecv.resume();//resume the IR detection 
    }
  }
  //Wifi
  float wifidelay = 1500 - (millis() - WifiTimeDelay); //calc how long is left on the wifi delay
  if(wifidelay <= 0 || wifidelay > 1500){ //if delay is finished or int overflow has occured 
      String getData="GET /update?api_key= APIKEY &field1=" + motionsave; //create command for sending data (add api code here)
      ESP8266.println("AT+CIPMUX=1");
      ESP8266.println("AT+CIPSTART=0,\"TCP\",\" api.thingspeak.com \",80"); //connection code with thingspeak
      ESP8266.println("AT+CIPSEND=0," + String(getData.length()+4));
      ESP8266.println(getData);//send data
      WifiTimeDelay = millis();//reset delay
  }
  
}
