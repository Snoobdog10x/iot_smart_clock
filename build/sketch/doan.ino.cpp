#line 1 "d:\\study\\UN_4th\\IoT\\doan\\doan.ino"
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <DS3232RTC.h>
#include <TM1637Display.h>
#define LAMP D7
#define BTN1 D3
#define BTN2 D4
#define CLK D5
#define DIO D6
#define CLICK_MILLIS 400
#define LONGPRESS_MILLIS 1000
#define BTN_INTERVAL_MILLIS 135
#define WAIT_BTN2_MILLIS 30
#define BTN1_CLICK 0
#define BTN2_CLICK 1
#define BTN1_LONG_PRESS 2
#define BTN2_LONG_PRESS 3
#define DOUBLE_BTN_CLICK 4
#define DOUBLE_BTN_LONGPRESS 5
#define BTN_DO_NOTHING 6
unsigned long button_time = 0;
bool check = false;
bool is_double = false;
bool is_btn1 = false;
byte buttons_handle()
{
    if ((digitalRead(BTN1) == LOW || digitalRead(BTN2) == LOW) && button_time == 0)
    {
        button_time = millis();
        check = true;
    }
    if (check && millis() - button_time > WAIT_BTN2_MILLIS)
    {
        if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == LOW)
        {
            is_double = true;
        }
        if (digitalRead(BTN1) == LOW && digitalRead(BTN2) == HIGH)
            is_btn1 = true;
        check = false;
    }
    if (button_time != 0)
    {

        if (millis() - button_time > BTN_INTERVAL_MILLIS)
        {

            if (is_double)
            {
                if (digitalRead(BTN2) == LOW && digitalRead(BTN1) == LOW)
                    if ((millis() - button_time) >= LONGPRESS_MILLIS + 1500)
                    {
                        Serial.println("double BTN long press");
                        return DOUBLE_BTN_LONGPRESS;
                    }
                if (digitalRead(BTN2) == HIGH && digitalRead(BTN1) == HIGH)
                {
                    if ((millis() - button_time) <= CLICK_MILLIS)
                    {
                        button_time = 0;
                        Serial.println("double BTN click");
                        is_double = false;
                        return DOUBLE_BTN_CLICK;
                    }
                    is_double = false;
                    button_time = 0;
                }
            }
            else
            {

                if (is_btn1)
                {
                    if (digitalRead(BTN1) == LOW)
                        if ((millis() - button_time) > LONGPRESS_MILLIS)
                        {
                            Serial.println("BTN1 long press");
                            return BTN1_LONG_PRESS;
                        }
                    if (digitalRead(BTN1) == HIGH)
                    {
                        if ((millis() - button_time) <= CLICK_MILLIS)
                        {
                            button_time = 0;
                            Serial.println("BTN1 click");
                            is_btn1 = false;
                            return BTN1_CLICK;
                        }
                        is_btn1 = false;
                        button_time = 0;
                    }
                }
                else
                {
                    if (digitalRead(BTN2) == LOW)
                        if ((millis() - button_time) > LONGPRESS_MILLIS)
                        {
                            Serial.println("BTN2 long press");
                            return BTN2_LONG_PRESS;
                        }
                    if (digitalRead(BTN2) == HIGH)
                    {
                        if ((millis() - button_time) <= CLICK_MILLIS)
                        {
                            button_time = 0;
                            Serial.println("BTN2 click");
                            return BTN2_CLICK;
                        }
                        button_time = 0;
                    }
                }
            }
        }
    }
    return BTN_DO_NOTHING;
}
DS3232RTC myRTC;
void set_time(byte hours, byte mins, byte secs)
{
    time_t t;
    t = now();
    tmElements_t tm;
    int y = year(t);
    if (y >= 1000)
        tm.Year = CalendarYrToTm(y);
    else // (y < 100)
        tm.Year = y2kYearToTm(y);
    tm.Month = month(t);
    tm.Day = day(t);
    tm.Hour = hours;
    tm.Minute = mins;
    tm.Second = secs;
    t = makeTime(tm);
    myRTC.set(t);
    setTime(t);
}
byte get_tempurature()
{
    return myRTC.temperature();
}
TM1637Display display(CLK, DIO);

unsigned long dot_bink_time = 0;
void on_at(int displaytime, bool at_hour)
{
    if (at_hour)
    {
        display.showNumberDecEx(displaytime / 100, 0b11100000, true, 2, 0);
    }
    else
    {
        display.showNumberDecEx(displaytime % 100, 0b11100000, true, 2, 2);
    }
}
void off_at(bool at_hour)
{
    uint8_t data[] = {0x0};
    if (at_hour)
    {

        display.setSegments(data, 1, 0);
        display.setSegments(data, 1, 1);
    }
    else
    {
        display.setSegments(data, 1, 3);
        display.setSegments(data, 1, 2);
    }
}
void blink_digit(int displaytime, unsigned long &time_blink, bool at_hour)
{
    if (millis() - time_blink >= 500)
    {
        on_at(displaytime, at_hour);
    }
    if (millis() - time_blink >= 1000)
    {
        off_at(at_hour);
        time_blink = millis();
    }
}

void show_tempu_time(bool is_time_show)
{
    if (is_time_show)
    {
        if (millis() - dot_bink_time >= 500)
        {
            time_t t;
            t = now();
            int displaytime = (hour(t) * 100) + minute(t);
            display.showNumberDecEx(displaytime, 0b11100000, true);
        }
        if (millis() - dot_bink_time >= 1000)
        {
            time_t t;
            t = now();
            int displaytime = (hour(t) * 100) + minute(t);
            display.showNumberDec(displaytime, true); // Prints displaytime without center colon.
            dot_bink_time = millis();
        }
    }
    else
    {
        const uint8_t SEG_DONE[] = {
            SEG_A | SEG_B | SEG_G | SEG_F,
            SEG_A | SEG_F | SEG_E | SEG_D,
        };
        byte tempur = get_tempurature();
        display.showNumberDec(tempur, false, 2, 0);
        display.setSegments(SEG_DONE, 2, 2);
    }
}
void adjust_time()
{
    time_t t;
    t = now();
    int displaytime = (hour(t) * 100) + minute(t);
    byte hours = hour(t), mins = minute(t);
    unsigned long time_blink = millis();
    bool at_hour = true;
    unsigned long auto_break = millis();
    int count = 0;
    while (millis() - auto_break < 5000)
    {
        byte state = buttons_handle();
        if (state == DOUBLE_BTN_CLICK)
            break;
        blink_digit(displaytime, time_blink, at_hour);
        if (state == BTN1_CLICK || state == BTN1_LONG_PRESS)
        {
            if (state == BTN1_LONG_PRESS)
            {
                delay(200);
            }
            if (at_hour)
            {
                hours++;
                if (hours == 24)
                {
                    hours = 0;
                }
            }
            else
            {
                mins++;
                if (mins == 60)
                {
                    mins = 0;
                }
            }
            displaytime = hours * 100 + mins;
            auto_break = millis();
        }
        else if (state == BTN2_CLICK)
        {
            auto_break = millis();
            at_hour = !at_hour;
            on_at(displaytime, true);
            on_at(displaytime, false);
        }
        yield();
    }
    set_time(hours, mins, 0);
}

bool led_state = false;
byte value = 0;
void lamp_display(byte brighness)
{
    analogWrite(LAMP, brighness);
}
void turn_on_off_led()
{
    led_state = !led_state;
}
void change_led_value(byte slider_value)
{
    if (led_state)
        value = slider_value;
}
void led_display()
{
    if (led_state)
    {
        lamp_display(value);
    }
    else
    {
        lamp_display(0);
    }
}
const char *dinor_ssid = "dinausor_RGB"; // SSID
const char *dinor_password = "";         // Password
AsyncWebServer server(80);
ESPDash dashboard(&server);
Card LED_CTL(&dashboard, BUTTON_CARD, "RGB ON");
Card SHOW_TEMP(&dashboard, BUTTON_CARD, "Show tempurature");
Card CHANG_LED(&dashboard, SLIDER_CARD, "RGB choice", "", 0, 255);
Card CHANG_HOUR(&dashboard, SLIDER_CARD, "Hour", "", 0, 23);
Card CHANG_MIN(&dashboard, SLIDER_CARD, "Minute", "", 0, 59);
Card CHANG_SEC(&dashboard, SLIDER_CARD, "Second", "", 0, 59);
int led_value = 0;
byte step = 32;
unsigned long time_btn2_long = 0;
bool is_time_display = true;
void btn_ctroller()
{
    byte state = buttons_handle();
    if (state == BTN1_CLICK)
    {
        turn_on_off_led();
        return;
    }
    if (state == BTN2_CLICK)
    {
        led_value += step;
        if (led_value == 255 || led_value == 0)
            step = -step;
        change_led_value(led_value);
        return;
    }
    if (state == BTN2_LONG_PRESS)
    {
        if (millis() - time_btn2_long >= 200)
        {
            led_value += step;
            if (led_value == 255 || led_value == 0)
                step = -step;
            change_led_value(led_value);
            time_btn2_long = millis();
        }
        return;
    }
    if (state == DOUBLE_BTN_CLICK)
    {
        is_time_display = !is_time_display;
        return;
    }
    if (state == DOUBLE_BTN_LONGPRESS)
    {
        adjust_time();
        return;
    }
}
unsigned long sync_time = 0;
void sync_time_slider()
{
    if (millis() - sync_time >= 1000)
    {
        time_t t;
        t = now();
        byte hours = hour(t), mins = minute(t), secs = second(t);
        CHANG_HOUR.update(hours);
        CHANG_MIN.update(mins);
        CHANG_SEC.update(secs);
        dashboard.sendUpdates();
        sync_time = millis();
    }
}
void setup()
{
    Serial.begin(115200);
    WiFi.softAPConfig(IPAddress(2, 2, 2, 2), IPAddress(2, 2, 2, 2), IPAddress(255, 255, 255, 0));
    WiFi.softAP(dinor_ssid, dinor_password);
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    LED_CTL.attachCallback([&](bool value)
                           {
    turn_on_off_led();
  LED_CTL.update(value);
  dashboard.sendUpdates(); });
    CHANG_LED.attachCallback([&](int value)
                             {
    change_led_value(value);
  CHANG_LED.update(value);
  dashboard.sendUpdates(); });
    SHOW_TEMP.attachCallback([&](bool value)
                             {
    is_time_display = !value;
  SHOW_TEMP.update(value);
  dashboard.sendUpdates(); });
    CHANG_HOUR.attachCallback([&](int value)
                              {
    time_t t;
t = now();
byte hours = value, mins = minute(t),secs=second(t);
set_time(hours,mins,secs);
  CHANG_HOUR.update(value);
  dashboard.sendUpdates(); });
    CHANG_MIN.attachCallback([&](int value)
                             {
    time_t t;
t = now();
byte hours = hour(t), mins = value,secs=second(t);
set_time(hours,mins,secs);
  CHANG_MIN.update(value);
  dashboard.sendUpdates(); });
    CHANG_SEC.attachCallback([&](int value)
                             {
    time_t t;
t = now();
byte hours = hour(t), mins = minute(t),secs=value;
set_time(hours,mins,secs);
  CHANG_SEC.update(value);
  dashboard.sendUpdates(); });
    server.begin();
    myRTC.begin();
    setSyncProvider(myRTC.get); // the function to get the time from the RTC
    if (timeStatus() != timeSet)
        Serial.println("Unable to sync with the RTC");
    else
        Serial.println("RTC has set the system time");
    display.setBrightness(5);
    display.clear();
    pinMode(BTN1, INPUT);
    pinMode(BTN2, INPUT_PULLUP);
}

void loop()
{
    btn_ctroller();
    show_tempu_time(is_time_display);
    led_display();
    sync_time_slider();
}
