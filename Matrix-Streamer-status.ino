/*******************************************************************
    Using a 64 * 32 RGB Matrix to display YT Subscriber stats
 *                                                                 *
    Original code written by Brian Lough and highly modified by Colin Hickey

    Brian's Youtube Channel
    https://www.youtube.com/channel/UCezJOfu7OtqGzd5xrP3q6WA

    My Channel
    https://www.youtube.com/colinhickey1m
 *******************************************************************/

// ----------------------------
// Standard Libraries - Already Installed if you have ESP8266 set up
// ----------------------------

#include <TwitchApi.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

//#define double_buffer
#include <PxMatrix.h>
// The library for controlling the LED Matrix
// Needs to be manually downloaded and installed
// https://github.com/2dom/PxMatrix

#include <YoutubeApi.h>
// For fetching the stats from the YouTube API
// Search for "YouTube" in the Arduino Library manager
// https://github.com/witnessmenow/arduino-youtube-api

#include <ArduinoJson.h>
// This Sketch doesn't technically need this, but the YT library does so it must be installed.
// Search for "ArduinoJson" in the library manger
// https://github.com/bblanchon/ArduinoJson


//------- Replace the following! ------
char ssid[] = "xxxx";       // your network SSID (name)
char password[] = "xxxx";  // your network key
#define API_KEY "xxxx"  // your google apps API Token
#define CHANNEL_ID "xxxx" // Channel ID of the channel you wish to monitor

// Create a new application on https://dev.twitch.tv/
#define TWITCH_CLIENT_ID "xxxx"
//------------------------------

//Set the twitch login you wish to monitor
#define TWITCH_LOGIN "benfruit"

//Set these to Twitch or Youtube if you wish to monitor when your chosen Youtube or Twitch person goes live
String Monitor = "";

//Friendly name of the Youtube person your monitoring the live status of
String YT_displayname = "Colin Hickey";

//Available screen saver options are Time, Youtube and Twitch
String DisplayOption = "Youtube";

Ticker display_ticker;

WiFiClientSecure client;
YoutubeApi api(API_KEY, client);
TwitchApi twitch(client, TWITCH_CLIENT_ID);

unsigned long api_mtbs = 60000; //mean time between api requests
unsigned long api_lasttime = 60001;   //last time api request has been done set to 60001 so it will do a check straight away on start

unsigned long yt_mtbs = 900000; //mean time between Youtube Live api requests, standard is 15 mins as each request is 100 and free daily limit is 10,000
unsigned long yt_lasttime = 900001;   //last time Youtube live api request has been done

long YTSubcount;
long TWSubcount;
bool TwitchLive = false;
bool YoutubeLive = false;

const long utcOffsetInSeconds = 3600;


String currenttime,currenthour,currentmin;
String channelstatus;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// Pins for LED MATRIX
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_OE 2
#define P_D 12
#define P_E 0

// PxMATRIX display(32,16,P_LAT, P_OE,P_A,P_B,P_C);
// PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);
PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);
//PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);

#define matrix_width 64
#define matrix_height 32

// Some standard colors
uint16_t myRED = display.color565(255, 0, 0);
uint16_t myGREEN = display.color565(0, 255, 0);
uint16_t myBLUE = display.color565(0, 0, 255);
uint16_t myWHITE = display.color565(255, 255, 255);
uint16_t myYELLOW = display.color565(255, 255, 0);
uint16_t myCYAN = display.color565(0, 255, 255);
uint16_t myMAGENTA = display.color565(255, 0, 255);
uint16_t myBLACK = display.color565(0, 0, 0);
uint16_t backgroundColor = display.color565(0, 0, 0);

uint16 myCOLORS[8] = {myRED, myGREEN, myBLUE, myWHITE, myYELLOW, myCYAN, myMAGENTA, myBLACK};

// Converted using the following site: http://www.rinkydinkelectronics.com/t_imageconverter565.php
uint16_t static youTubeBigger[] = {
  0x0000, 0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0010 (16) pixels
  0xF800, 0xF800, 0xF800, 0x0000, 0x0000, 0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0020 (32) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0030 (48) pixels
  0xF800, 0xF800, 0xFFFF, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0040 (64) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xFFFF, 0xFFFF, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0050 (80) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xFFFF, 0xFFFF, 0xFFFF, 0xF800,   // 0x0060 (96) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0070 (112) pixels
  0xF800, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0080 (128) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0090 (144) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,   // 0x00A0 (160) pixels
  0xFFFF, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x00B0 (176) pixels
  0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x00C0 (192) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x00D0 (208) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0xF800, 0xF800,   // 0x00E0 (224) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xFFFF,   // 0x00F0 (240) pixels
  0xFFFF, 0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0100 (256) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xFFFF, 0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0110 (272) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0120 (288) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0x0000, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800,   // 0x0130 (304) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0x0000, 0x0000, 0x0000, 0xF800, 0xF800, 0xF800,   // 0x0140 (320) pixels
  0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0xF800, 0x0000, 0x0000,   // 0x0150 (336) pixels
};

uint16_t static TwitchBigger[] = {
  0x0000, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214,   // 0x0010 (16) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214,   // 0x0020 (32) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6214, 0x6214,   // 0x0030 (48) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214,   // 0x0040 (64) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214,   // 0x0050 (80) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214,   // 0x0060 (96) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214,   // 0x0070 (112) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214,   // 0x0080 (128) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x6214, 0x6214,   // 0x0090 (144) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214,   // 0x00A0 (160) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x6214,   // 0x00B0 (176) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214,   // 0x00C0 (192) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x0000,   // 0x00D0 (208) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000,   // 0x00E0 (224) pixels
0x0000, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x0000,   // 0x00F0 (240) pixels
0x0000, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0100 (256) pixels
0x0000, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0110 (272) pixels
0x0000, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0110 (288) pixels
0x0000, 0x0000, 0x0000, 0x6214, 0x6214, 0x6214, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,   // 0x0110 (304) pixels
};

// ISR for display refresh
void display_updater()
{
  display.display(70);
}

void setup() {
  
  Serial.begin(9600);
  
  // Define your display layout here, e.g. 1/8 step
  display.begin(16);

  display.clearDisplay();
  display.setTextColor(myCYAN);
  display.setCursor(2, 0);
  display.print("Connecting");
  display.setTextColor(myMAGENTA);
  display.setCursor(2, 8);
  display.print("to the");
  display.setTextColor(myGREEN);
  display.setCursor(2, 16);
  display.print("WiFi!");
  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  Serial.print("Pixel draw latency in us: ");
  unsigned long start_timer = micros();
  display.drawPixel(1, 1, 0);
  unsigned long delta_timer = micros() - start_timer;
  Serial.println(delta_timer);

  Serial.print("Display update latency in us: ");
  start_timer = micros();
  display.display(0);
  delta_timer = micros() - start_timer;
  Serial.println(delta_timer);

  display_ticker.attach(0.002, display_updater);
  yield();

  timeClient.begin();
  
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);
  
  client.setInsecure();
}

void drawYouTube(int x, int y)
{
  int counter = 0;
  for (int yy = 0; yy < 16; yy++)
  {
    for (int xx = 0; xx < 21; xx++)
    {
      display.drawPixel(xx + x , yy + y, youTubeBigger[counter]);
      counter++;
    }
  }
}

void drawTwitch(int x, int y)
{
  int counter = 0;
  for (int yy = 0; yy < 16; yy++)
  {
    for (int xx = 0; xx < 16; xx++)
    {
      display.drawPixel(xx + x , yy + y, TwitchBigger[counter]);
      counter++;
    }
  }
}

void drawTime(int x, int y)
{
  displayText(currenttime, y);
}

void displayText(String text, int yPos) {
  int16_t  x1, y1;
  uint16_t w, h;

  display.setTextSize(2);
  char charBuf[text.length() + 1];
  text.toCharArray(charBuf, text.length() + 1);
  display.getTextBounds(charBuf, 0, yPos, &x1, &y1, &w, &h);
  int startingX = 33 - (w / 2);
  if (startingX < 0) {
    display.setTextSize(1);
    display.getTextBounds(charBuf, 0, yPos, &x1, &y1, &w, &h);
    startingX = 33 - (w / 2);
  }
  display.setCursor(startingX, yPos);
  display.println(text);
}

void updateScreen(String platform) {
  display.clearDisplay();
  
  if (platform=="Youtube") {
    if(api.getChannelStatistics(CHANNEL_ID))
    {
      drawYouTube(21, 0);
      displayText(String(api.channelStats.subscriberCount), 17);
      YTSubcount = api.channelStats.subscriberCount;
      Serial.print("Youtube Sub count = ");
      Serial.println(api.channelStats.subscriberCount);
      delay(5000);
    }
  } else if (platform=="Twitch"){
        UserData user = twitch.getUserData(TWITCH_LOGIN);
        delay(1000);
        if(!user.error){
        
        FollowerData followerData = twitch.getFollowerData(user.id);
        delay(1000);
        if(!followerData.error){
          drawTwitch(24,0);
          displayText(String(followerData.total), 17);
        }
        }
     delay(5000);
  } else if (platform=="Time"){
    drawTime(0,9);
  }
}

void displayName(String first, String last)
{
  display.setTextColor(myCYAN);
  displayText(first, 1);
  display.setTextColor(myMAGENTA);
  displayText(last, 17);
}

unsigned long last_draw=0;
void scroll_text(uint8_t ypos, unsigned long scroll_delay, String text, uint8_t colorR, uint8_t colorG, uint8_t colorB)
{
    uint16_t text_length = text.length();
    display.clearDisplay();
    display.setTextWrap(false);  // we don't wrap text so it scrolls nicely
    display.setTextSize(1);
    display.setRotation(0);
    display.setTextColor(display.color565(colorR,colorG,colorB));

    // Asuming 5 pixel average character width
    for (int xpos=matrix_width; xpos>-(matrix_width+text_length*5); xpos--)
    {
      display.setTextColor(display.color565(colorR,colorG,colorB));
      display.clearDisplay();
      if(Monitor=="Twitch") {
        drawTwitch(24,1);
      } else if(Monitor=="Youtube") {
        drawYouTube(21, 0);
      }
      display.setCursor(xpos,ypos);
      display.print(text);
      delay(scroll_delay);
      yield();

      // This might smooth the transition a bit if we go slow
      // display.setTextColor(display.color565(colorR/4,colorG/4,colorB/4));
      // display.setCursor(xpos-1,ypos);
      // display.println(text);

      delay(scroll_delay/5);
      yield();
    }
}


void loop() {
  //Select colour which shall be used for displayed Text or time
  display.setTextColor(myCYAN);
  
  //Check every 1 min for enabled options
  if (millis() - api_lasttime > api_mtbs)  {
    if (DisplayOption=="Time") {  
      //retrieve current time (auto limited to every 60 seconds)
      timeClient.update();
    
      //format received time and add a leading 0 if required
      currenthour = timeClient.getHours() < 10 ? "0" + String(timeClient.getHours()): String(timeClient.getHours());
      currentmin = timeClient.getMinutes() < 10 ? "0" + String(timeClient.getMinutes()) : String(timeClient.getMinutes());
      currenttime = currenthour + ":" + currentmin;
    }

    if (Monitor=="Twitch") {
      StreamInfo stream = twitch.getStreamInfo(TWITCH_LOGIN);
      delay(500);
      if(!stream.error){
        scroll_text(21,2,String(TWITCH_LOGIN) + " is live!",0,255,0);
        if (String(stream.type) == "live") TwitchLive = true; 
          else TwitchLive = false;
        }
    }
    //Send update to screen
    updateScreen(DisplayOption);
    
    api_lasttime = millis();
    }
  
  //Timer for Youtube live status which must be every 15mins
  if (millis() - yt_lasttime > yt_mtbs)  {
    if (Monitor=="Youtube") {
        if(api.getChannelLive(CHANNEL_ID))
        {
          if (api.channelStats.channellive) {
            YoutubeLive=true;
            Serial.println("Youtube live");
          } else YoutubeLive=false;
        }
    }
    yt_lasttime = millis();
  }
  
  if (TwitchLive==true) scroll_text(21,2,String(TWITCH_LOGIN) + " is live!",0,255,0);
  if (YoutubeLive==true) scroll_text(21,2,YT_displayname + " is live!",0,255,0);
}
