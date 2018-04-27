/*................................................... Object Tracking System ................................................................*/
#include<SoftwareSerial.h>
#include <TinyGPS.h>

#include <SPI.h>
#include <SD.h>

#define LED 5 
#define PUSH_BUTTON 16   // Used to bookmark a location
                                                                              

/* set up variables using the SD utility library functions: */
const int chipSelect = 15;
File Cfg_file, Track_file;
char Ph_Num_1[14]={0}, Ph_Num_2[14]={0};
char Freq[3] = {0};
int Ph_idx = 0, frequency = 0;


char data[100] = {0}, ch = 0;
int idx = 0, First_Msg_Sent_Fg = 0, push_button = 0;
char TIME[9] = {0}, latitude[10] = {0}, longitude[11] = {0};
char Next_TimeStamp[9] = {0};
char Maps_link[32] = "https://maps.google.com/maps?q=";            


SoftwareSerial Gpsserial(3, 1); // RX | TX

/************************************************************ Initialisation ************************************************************/
void setup()
{ 
  pinMode(LED, OUTPUT);
  pinMode(PUSH_BUTTON, INPUT);  
  Gpsserial.begin(9600);     
  Serial1.begin(9600);       
  delay(1000);
  /* Sending AT command to GPS in order to  configure to GPRMC protocol */
  Gpsserial.println("$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"); 

  
  if (!SD.begin(chipSelect))
  {
    Serial1.println("initialization failed. Things to check:"); //debug
    Serial1.println("* is a card inserted?"); //debug
    Serial1.println("* is your wiring correct?"); //debug
    Serial1.println("* did you change the chipSelect pin to match your shield or module?"); //debug
    return;
  }

    /******************************************************** Reading Configuration Details *******************************************/
  Cfg_file = SD.open("Config.txt");
  if (Cfg_file) 
  {
       Ph_idx = 0;
       //Serial1.println("Config.txt:");// for debug only
       /* Read from the file until there's nothing else in it */
       while (Cfg_file.available()) 
       {
            if(Ph_idx < 14)
            {
               Ph_Num_1[Ph_idx] = Cfg_file.read();
            } 
            else if((Ph_idx >= 14) && (Ph_idx < 28))
            {
               Ph_Num_2[Ph_idx - 14] = Cfg_file.read();
            }
            else
            {
               Freq[(Ph_idx) - 28] = Cfg_file.read(); 
            }
            Ph_idx++;
       }
    
        if(Freq[1] != '\0')
        {
            frequency = (((Freq[0] - 48) * 10) + (Freq[1] - 48));
        }
        else
        {
            frequency = (Freq[0] - 48);
        }

       //Serial1.print("Frequency is  "); // debug only
       //Serial1.println(frequency);  // debug only

       Ph_Num_1[13] = '\0';
       Ph_Num_2[13] = '\0';

       //Serial1.print("Numbers are ");  // debug only
       //Serial1.println(Ph_Num_1);  // debug only
       //Serial1.println(Ph_Num_2);  // debug only
       
       // close the file:
       Cfg_file.close();
  }
  
  digitalWrite(LED, LOW); 
}

/********************************* Calculating Next time stamp at whcih message has to be send again *********************************/
void CalNextTimeStamp(void)
{
    int Minutes = 0, Hr_fg = 0, Hours = 0;
    strcpy(Next_TimeStamp,TIME);
    Minutes = (((Next_TimeStamp[3] - 48) * 10) + (Next_TimeStamp[4] - 48));
    Minutes = (Minutes + frequency);
    if(Minutes > 59)
    {
      Minutes = (Minutes - 60);
      Hr_fg = 1;
    }
    Next_TimeStamp[4] = (Minutes % 10) + 48;
    Next_TimeStamp[3] = (Minutes / 10) + 48;
    
    if(1 == Hr_fg)
    {
        Hours = (((Next_TimeStamp[0] - 48) * 10) + (Next_TimeStamp[1] - 48));
        Hours++;
        if(Hours > 23)
        {
             Hours = (Hours - 24);
        }
        Next_TimeStamp[1] = (Hours % 10) + 48;
        Next_TimeStamp[0] = (Hours / 10) + 48;   
    }        
}

/***************************************************** Converting to Indain Time ********************************************************/
void CnvrtTime(void)
{
  int Hr_fg = 0, Hours = 0;
 
  TIME[3] = TIME[3] + 3;
  if(TIME[3] > 53)
  {
    TIME[3] = (TIME[3] - 6);
    Hr_fg = 1;
  }

  Hours = (((TIME[0] - 48) * 10) + (TIME[1] - 48));
  Hours = (Hours + 5 + Hr_fg);
  if(Hours > 23)
  {
    Hours = (Hours - 24);
  }

  TIME[1] = (Hours % 10) + 48;
  TIME[0] = (Hours / 10) + 48;  
}

/***************************************** Extracting lat, long and time from received GPS data ****************************************/
void GetData(void)
{
   int idx = 0, index = 0;
   long int lat = 0, longi = 0; 
   for(index = 0; index < 6; index++)
   {
      TIME[idx++] = data[index + 7];
      if((1 == index) || (3 == index))
      {
          TIME[idx++] = ':';
      }
   }
      TIME[idx] = '\0';

   lat = 0;
   for(index = 0; index < 9; index++)
   {
      if('.' != data[index + 20])
      {
         lat = ((lat * 10) + (data[index + 20] - 48));
      }
   }
   lat = ((lat/1000000)*1000000 + (((lat % 1000000) * 10) + 3)/6);

   latitude[9] = '\0';
   for(idx = 0; idx< 9; idx++)
   {
       latitude[8-idx] = (lat%10) + 48;
       if(lat)
       {
          lat = lat/10;
       }       
       if(idx == 5)
       {
          idx++;
          latitude[8-idx] = '.';
       }
   }
   

   longi = 0;
   for(index = 0; index < 10; index++)
   {
      if('.' != data[index + 32])
      {
        longi = ((longi * 10) + (data[index + 32] - 48));
      }
   }

   longi = ((longi/1000000)*1000000 + (((longi % 1000000) * 10) + 3)/6);

   longitude[10] = '\0';
   for(idx = 0; idx< 10; idx++)
   {
       longitude[9-idx] = ((longi%10) + 48);
       if(longi)
       {
          longi = longi/10;
       }
       if(idx == 5)
       {
          idx++;
          longitude[9-idx] = '.';
       }
   }
   
      
    CnvrtTime();    
}

/*********************************************** Runs forever ***************************************/
void loop() 
{   
  if(Gpsserial.available())
  {
      ch = Gpsserial.read();
      if('$' == ch)
      {
          idx = 0;
          data[idx++] = '$';                               
      }
      else if(idx > 0)
      {
          data[idx++] = ch;                               
      }

      if(idx > 18)
      {
         /* If vaild data is coming from GPS */
         if('A' == data[18])
         {
            /* Glow indication LED */
            digitalWrite(LED, HIGH); 

            /* If all bytes are received */
            if(idx > 50)
            {
                /* Extract the data */
                GetData();

                push_button = 0;
                 
                /* If bookmark button is pressed or next time is equal to current time stamp or during first message to be sent */
                if((0 == First_Msg_Sent_Fg) || (0 == strcmp(Next_TimeStamp, TIME)) || (HIGH == digitalRead(PUSH_BUTTON)))
                {
                    First_Msg_Sent_Fg = 1;
                    
                    if(HIGH == digitalRead(PUSH_BUTTON))
                    {
                        push_button = 1;
                    }

                    /* Sending AT commands to GSM in order to send a message to phone number-1 */
                    Serial1.println("AT");   
                    delay(1000); 
                    Serial1.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
                    delay(1000);  
                    Serial1.print("AT+CMGS=\"");
                    Serial1.print(Ph_Num_1);
                    Serial1.println("\""); 
                    delay(1000);              
                    Serial1.print(Maps_link);
                    Serial1.print(latitude);
                    Serial1.print(",");
                    Serial1.print(longitude);                    
                    delay(100);
                    Serial1.println((char)26);// ASCII code of CTRL+Z
                    delay(1000);


                    /* Writting google maps location link along with time stamp to SD card */
                    Track_file = SD.open("Tracking.txt", FILE_WRITE);

                    if (Track_file) 
                    {
                        //Serial1.println("Writing to data to Tracking.txt file in SD card............."); // Debug data

                        /* If push button is pressed */
                        if(1 == push_button)
                        {
                             Track_file.print("\"Bookmark\"-->   ");
                             delay(1000);
                        }                        
                        Track_file.print(TIME);/////
                        delay(1000);
                        Track_file.print("  ");/////
                        delay(1000);                        
                        Track_file.print(Maps_link);/////
                        delay(1000);
                        Track_file.print(latitude);/////
                        delay(1000);
                        Track_file.print(",");/////
                        delay(1000);
                        Track_file.println(longitude);/////                     
                        // close the file:
                        Track_file.close();
                        delay(100);
                        //Serial1.println("Writting to SD card is done."); // debug data
                    } 
                    else
                    {
                        // if the file didn't open, print an error:
                        //Serial1.println("error opening test.txt"); // debug data
                    }

                    /* Sending AT commands to GSM in order to send a message to phone number-2 */
                    Serial1.println("AT");   
                    delay(1000); 
                    Serial1.println("AT+CMGF=1");    
                    delay(1000); 
                    Serial1.print("AT+CMGS=\"");
                    Serial1.print(Ph_Num_2);
                    Serial1.println("\""); 
                    delay(1000);                  
                    Serial1.print(Maps_link);
                    Serial1.print(latitude);
                    Serial1.print(",");
                    Serial1.print(longitude);                    
                    delay(100);
                    Serial1.println((char)26);// ASCII code of CTRL+Z
                    delay(1000);                

                    /* Calcuate next time stamp */
                    CalNextTimeStamp();
                    //Serial1.print("next time stamp is "); ////////////////////////////////////////////debug
                    //Serial1.println(Next_TimeStamp); ////////////////////////////////////////////debug
                }
                idx = 0;  
                //data[18] = 'v';// to make the led off when gps is not there           
            }                                    
         }
         else
         {
            idx = 0;
            // off LED
            digitalWrite(LED, LOW); 
         }
      }    
  }
}
