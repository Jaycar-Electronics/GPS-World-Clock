#include <U8g2lib.h>    //version 2.24.3
#include <NMEAGPS.h>    //version 4.2.9
using namespace NeoGPS; //C++ namespace

#include <SPI.h>

#include "tz.h"
#include "bitmap.h"

//set up the screen
U8G2_ST7920_128X64_F_HW_SPI u8g2(U8G2_R0, /* CS=*/53);
//also can use U8G2_R0, R1,R3 for rotation options

//gps reading:
NMEAGPS::decode_t result;
static NMEAGPS gps;
gps_fix fix;

//pin definitions
#define PIN_A 21
#define PIN_B 19
#define PIN_BUTTON 18

#define LOCAL_HOURS 10
#define LOCAL_MINUTES 0
#define LOOP_DELAY 1000
#define LOOP_MENU_DELAY 100

//variable storage
short enc_value = 0;
bool button_trig = false; //triggered button press

/***********************
  Interrupt processing routines and "private" functions
*/
void isr_enc()
{
  if (digitalRead(PIN_A) == digitalRead(PIN_B))
    enc_value++;
  else
    enc_value--;
}

void isr_button()
{                         //called every change
  delayMicroseconds(10); //debounce,
  if (!button_trig and digitalRead(PIN_BUTTON))
  {
    //we test if the button change went HIGH, which means button release
    button_trig = true;
  }
}

void serialEvent2()
{
  char c = Serial2.read();
  Serial.write(c);
  gps.decode(c);
}

short read_encoder()
{ //not an interrupt, but consumable reading
  short ret = 0;
  if (enc_value)
    ret = enc_value;
  enc_value = 0;
  return ret;
}

short breakable_delay(unsigned int timer)
{
  for (long ctime = millis();       //initialise
       millis() < (ctime + timer); // test
  )
  { //empty increment

    if (button_trig)
    {
      break;
    }
  }
}
//-----------------------------------------------

short selected_tz = 0;
bool menu = false;
short cursor = 0;

char UTC_str[] = "UTC:   XX:XX:XX";
char LOCAL_str[] = "Local: XX:XX:XX";
char TZ_str[] = "XX:XX:XX";
char *TZNAME_str = 0;
char *CNAME_str = 0;

short tz_mod_hours = 4;
short tz_mod_minutes = 5;
short UTC_hours = 1;
short UTC_minutes = 23;
short UTC_seconds = 45;

void setup()
{
  Serial.begin(9600);  //debugging to USB
  Serial2.begin(9600); //GPS signal.

  pinMode(15, OUTPUT); // button GND
  pinMode(20, OUTPUT); // encoder GND

  //setting "PULLUP" means that the value will be high until it is pressed.
  pinMode(PIN_BUTTON, INPUT_PULLUP); //button press
  pinMode(PIN_A, INPUT_PULLUP);       //encoder up
  pinMode(PIN_B, INPUT_PULLUP);       //encoder down

  attachInterrupt(digitalPinToInterrupt(PIN_B), isr_enc, CHANGE); //change to PIN_A for opposite direction
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), isr_button, CHANGE);

  u8g2.begin();

  Serial.print("We have ");
  Serial.print(tz_length);
  Serial.print("Timezones");

  selected_tz = 1; //eeprom read
  change_time();

  menu = false; // reset accidental trigger of interrupt
}
long looptimer = 0;

void loop()
{

  if (millis() > looptimer + (menu ? LOOP_MENU_DELAY : LOOP_DELAY))
  {
    looptimer = millis();
    u8g2.clearBuffer(); //clear internal buffer
    //drawing functions-------------------------
    if (menu)
    {
      cursor += read_encoder();
      show_menu(cursor);
    }
    else
    {
      show_clock();
    }
    u8g2.sendBuffer();
  }
  //-------------------------------------------

  //breakable_delay(10); //conditionally delay, either 10 or 1000,

  //input processing
  if (button_trig)
  {
    button_trig = false;
    if (menu)
      activate_menu(cursor);
    else
      menu = true;
  }
  //----------------------

  while (gps.available(Serial2))
  {
    //gps code
    fix = gps.read();

    if (fix.valid.time)
    {
      UTC_minutes = fix.dateTime.minutes;
      UTC_hours = fix.dateTime.hours;
      UTC_seconds = fix.dateTime.seconds;
    }
    mod_strings();
  }
}

void change_time()
{
  struct tzinfo *p = &tz_list[selected_tz % tz_length];

  TZNAME_str = (const char *)p->tz;
  CNAME_str = (const char *)p->name;

  short mod = p->hours;
  tz_mod_hours = (mod) & (0x0F); //only bottom nibble
  if (mod & TZ_NEG)
    tz_mod_hours = -tz_mod_hours;
  if (mod & TZ_HALF)
    tz_mod_minutes = 30;
  else
    tz_mod_minutes = 0;
}
void modify_time(char str[], short idx, short set_hour, short set_minute, short set_second)
{

  //wrap around error prevention
  set_hour = set_hour % 24;
  set_minute = set_minute % 60;
  set_second = set_second % 60;

  str[idx] = (set_hour > 9 ? '0' + set_hour / 10 : ' '); //space instead of "09"
  str[idx + 1] = '0' + set_hour % 10;

  str[idx + 3] = '0' + set_minute / 10;
  str[idx + 4] = '0' + set_minute % 10;

  str[idx + 6] = '0' + set_second / 10;
  str[idx + 7] = '0' + set_second % 10;
}
void mod_strings()
{

  modify_time(LOCAL_str, 7, //localtime string starts at idx7
              UTC_hours + LOCAL_HOURS,
              UTC_minutes + LOCAL_MINUTES,
              UTC_seconds);

  modify_time(UTC_str, 7, //utc string starts at idx7
              UTC_hours,
              UTC_minutes,
              UTC_seconds);

  modify_time(TZ_str, 0,
              UTC_hours + tz_mod_hours,
              UTC_minutes + tz_mod_minutes,
              UTC_seconds);
}

void show_menu(short cursor)
{
  cursor += tz_length;
  cursor %= tz_length; //wrap around;
  struct tzinfo *p = &tz_list[cursor];

  //optionally add some datetime strings here.

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, (const char *)p->name);
  u8g2.drawStr(0, 20, (const char *)p->tz);
}
void show_clock()
{
  mod_strings(); //set up all strings

  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(25, 10, "World Clock");
  u8g2.drawStr(30, 50, CNAME_str);

  u8g2.setFont(u8g2_font_profont12_tr);
  u8g2.drawStr(30, 30, UTC_str);
  u8g2.drawStr(30, 40, LOCAL_str);
  u8g2.drawStr(30, 62, TZNAME_str);
  u8g2.drawStr(72, 62, TZ_str);

  //draw world bitmap
  u8g2.drawXBM(0, 20, 26, 26, bitmap_bits);
}

void activate_menu(short cursor)
{
  cursor += tz_length;
  cursor %= tz_length; //wrap around;
  //make cursor new tz
  selected_tz = cursor;
  menu = false;
  change_time();
}
