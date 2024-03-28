/* R17 has been updated to check for caps lock before typing password */
/* R19 re-wrote staticly assigned password handling to be dynamic */
/* R20 finished changes on the dynamic password handling */
/* 2018 re-wrote bluetooth parsing of encryption key */
/* 2018r3 changed code to get number of password entries from android app */
/* 2018r4 if password is not set, it displays blank passowrd instead of unreadable text.  The same goes for when you have the unit type your password */
/* 2018r5 changed renaming code from 10 characters to 15 for profile naming */
/* renamed to r23 and uploaded to old password keepers */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Entropy.h>
#include "Arduino.h"
#include <AES.h>
#include <SD.h>
#include "usb_keyboard.h"

int version_number = 2018;
int revision_number = 5;

/* Number of Password Entries, defualt value, Actual number of entries supplied by android app */
int NumPassEntries = 15;  

// Bit numbers for each led - used to make it easier later to know what you were actually testing for...
#define USB_LED_NUM_LOCK 0
#define USB_LED_CAPS_LOCK 1
#define USB_LED_SCROLL_LOCK 2
#define USB_LED_COMPOSE 3
#define USB_LED_KANA 4

/* number of encryption bits to be used */
#define KEYBITS 256
/* oled display reset pin */
#define OLED_RESET 23
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

/* SD card CS pin */
const int CS_Pin = 10;

/* SD card file constructor */
File myFile;

/* encryption key */
uint8_t encryptionKey[32] = {
  0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,
  0,0
};

/* used when reading sd card */
String sd_data = "";

/* temporary plain text password buffer */
unsigned char plaintext[16];
/* temporary encrypted password buffer*/
unsigned char ciphertext[16];

/* holds encrypted passwords  */
unsigned char pwd[100][16];  //was pwd[NumPassEntries][16];

/* Initialise the encryption code */
unsigned long rk[RKLENGTH(KEYBITS)];
/* used during encryption / decryption */
int nrounds = 0;
/* password creation buffer */
char pw[9];

/* button pad connection variables */
const int buttonPin_0 = 8;  /* Select IN */ 
const int buttonPin_1 = 7;  /* Right */
const int buttonPin_2 = 6;  /* Down */ 
const int buttonPin_3= 5;   /* Up */
const int buttonPin_4= 4;   /* Left */ 
const int buttonPin_5= 3;   /* Back Switch */

/* button pas states */
int buttonState_0 = 0;
int buttonState_1 = 0;
int buttonState_2 = 0;
int buttonState_3 = 0;
int buttonState_4 = 0;
int buttonState_5 = 0;

/* used to keep track of which profile is selected */
int profile_selected = 99;

char *yes_no_options[] = {
  "No", "Yes"};

/* holds profile list loaded from SD */
String profile_options[100];

char *main_menu_options[] = {
  "- Select Profile","- Display Password","- Generate Password","- Transmit Password"};

/* initial selection (first option = 0) */
int highlight_option=0; 

void setup()
{
  /* clear profile options memory*/
  for(int i = 0; i <= NumPassEntries-1; i++){
    profile_options[i] = "";
  }
  
  /* clear password memory*/
  for (int i = 0; i <= NumPassEntries-1; i++){
    for (int j = 0; j <= 15; j++){ 
       pwd[i][j] = ' ';
    }
  }
  /* init oled display - addresses 0x3C, 0x3D */
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  /* show splashscreen */
  display.display(); 
  /* wait for 2 seconds */
  delay(2000);
  /* init serial interface */
  Serial.begin(115200);
  /* init bluetooth interface */
  Serial1.begin(115200);
  /* init entropy library */
  Entropy.Initialize();
  /* clear oled display */
  display.clearDisplay();
  /* set text size */
  display.setTextSize(1);
  /* set text color */
  display.setTextColor(WHITE);
  /* set cursor position */
  display.setCursor(30,0);
  /* display text to lcd */
  display.println("PIN Needed"); 
  /* send display data to lcd */ 
  display.display();
  /* get encryption key from android app */
  get_authorization();
  /* get number of password entries from android app */
  get_password_entries();
  /* clear oled display */
  display.clearDisplay();
  /* SD card CS pin */
  pinMode(CS_Pin, OUTPUT);
  /* set select button to input */
  pinMode(buttonPin_0, INPUT);
  /* set right button to input */
  pinMode(buttonPin_1, INPUT);
  /* set down button to input */ 
  pinMode(buttonPin_2, INPUT);  
  /* set up button to input */   
  pinMode(buttonPin_3, INPUT); 
  /* set left button to input */   
  pinMode(buttonPin_4, INPUT);   
  /* set back button to input */   
  pinMode(buttonPin_5, INPUT); 
  /* if SD card fails to init display error on lcd */
  if(!SD.begin(CS_Pin))
  {
    /* do nothing */
  }
    /* display software version number */
  Serial.print("Software Revision: ");
  Serial.println(revision_number);
  
  load_menu_options();  
  
  read_all_password_data();
  
  Serial.println("Done Initalizing.");
}

boolean loop_draw = false;
void loop()
{
  /* Read button inputs */
  buttonState_0 = digitalRead(buttonPin_0); /* IN */
  buttonState_1 = digitalRead(buttonPin_1); /* Left */
  buttonState_2 = digitalRead(buttonPin_2); /* Down */
  buttonState_3 = digitalRead(buttonPin_3); /* Up */
  buttonState_4 = digitalRead(buttonPin_4); /* Right */
  buttonState_5 = digitalRead(buttonPin_5); /* Switch */

  /* if the down button is pressed */
  if(buttonState_2 == LOW)           
  {
    /* clear the display */
    display.clearDisplay();
    /* increment screen selection */
    highlight_option++; 
    /* allow the lcd to re-draw */
    loop_draw = false;    
  }
  /* if the up button it pressed*/
  else if (buttonState_3 == LOW)
  {
    /* clear the display */
    display.clearDisplay();
    /* decrement screen selection */
    highlight_option--; 
    /* allow the lcd to re-draw */  
    loop_draw = false;    
  }
  
  /* if a profile has been selected show all the options */
  if(profile_selected != 99)
  {
  
    /* if we get to the last botton selection move to the top selection 
     if we get tot the last top selection move to bottom selection */
    if (highlight_option < 0) highlight_option = 3; 
    else if (highlight_option > 3) highlight_option = 0;  
  
  /* otherwirse show a reduced number of options */
  }else{
    
    /* if we get to the last botton selection move to the top selection 
     if we get tot the last top selection move to bottom selection */
    if (highlight_option < 0) highlight_option = 0; 
    else if (highlight_option >= 1) highlight_option = 0;      
    
  }
  
  /* re-draw the screen if something changed otherwise dont both re-drawing display */
  if(loop_draw == false)
  {
    /* set the text font size */
    display.setTextSize(1);
    /* set the text color */
    display.setTextColor(WHITE);
    /* set the starting location of the cursor */
    display.setCursor(40,0);
    /* display menu to screen */
    display.println("Menu");

    if(profile_selected != 99)
    {
      /* display the profile selections */
      for (int i=0;i<4;i++) 
      {
        /* if we are currently drawing what we have selected */
        if(highlight_option == i) 
        { 
          /* highlight the option */
          display.setTextColor(BLACK, WHITE); 
        } 
        /* otherwise */
        else 
        {
          /* draw the text normally */
          display.setTextColor(WHITE); 
        }
        /* go the the next line of the display  */
        display.setCursor(0,10+i*10); 
        /* display the menu options as we progress throught the loop */
        display.println(main_menu_options[i]); 
      }
    }else{
      /* display the profile selections */
      for (int i=0;i<1;i++) 
      {
        /* if we are currently drawing what we have selected */
        if(highlight_option == i) 
        { 
          /* highlight the option */
          display.setTextColor(BLACK, WHITE); 
        } 
        /* otherwise */
        else 
        {  
          /* draw the text normally */
          display.setTextColor(WHITE); 
        }
        /* go the the next line of the display  */
        display.setCursor(0,10+i*10); 
        /* display the menu options as we progress throught the loop */
        display.println(main_menu_options[i]);      
      }
    }    /* send the display data to the lcd */ 
    display.display();
    /* set flag to indicate that we dont need to re-draw the
       screen unless something changes.                   */
    loop_draw = true; 
    }
  /* small delay between itterations (otherwise 
     the button presses occur too quickly)*/
  delay(75);
  
  /* if the select button is pressed */
  if (buttonState_0 == LOW)
  {
    /* handle menu selection */
    perform_action();
  }
}

void perform_action()
{
  /* used to determine when the screen needs to be re-drawn */
  boolean option0_draw = false;
  /* if the user chooses select profile */
  if(highlight_option == 0)
  {
    /* small delay to wait for select button to settle */
    delay(75);
    /* clear the lcd */
    display.clearDisplay();
    /* while we are in the profile selection menu */
    while(1)
    {
      /* Read button inputs */
      buttonState_0 = digitalRead(buttonPin_0); /* IN */
      buttonState_1 = digitalRead(buttonPin_1); /* Left */
      buttonState_2 = digitalRead(buttonPin_2); /* Down */
      buttonState_3 = digitalRead(buttonPin_3); /* Up */
      buttonState_4 = digitalRead(buttonPin_4); /* Right */
      buttonState_5 = digitalRead(buttonPin_5); /* Switch */
      
      /* if the user pressed the down button */
      if(buttonState_2 == LOW)            
      {
        /* clear the display */
        display.clearDisplay();
        /* increment screen selection */
        highlight_option++;
        /* allow menu 1 to re-draw */
        option0_draw = false;  
 
      }
      /* if the user pressed the up button */
      else if (buttonState_3 == LOW)   
      {
        /* clear the display */
        display.clearDisplay();
        /* decrement screen selection */
        highlight_option--; 
        /* allow menu 1 to re-draw */ 
        option0_draw = false;   
      
      }
      
      /* if we get to the last botton selection move to the top selection 
         if we get tot the last top selection move to bottom selection */
      if (highlight_option < 0) highlight_option = NumPassEntries-1; 
      else if (highlight_option > NumPassEntries-1) highlight_option = 0;
      
      /* if the user presses the right button, enter re-name mode */
      if(buttonState_4 == LOW)
      {
        if(profile_selected != 99) {  
          reNameProfile();
          option0_draw = false;
        }
      }
      
      /* re-draw the screen if something changed otherwise dont both re-drawing display */
      if(option0_draw == false)
      {
        /* set the text font size */
        display.setTextSize(1);
        /* set the text color */
        display.setTextColor(WHITE);
        /* set the cursor position */
        display.setCursor(25,0);
        /* display text to lcd */
        display.println("Select Profile");
        
        int offset = 0;
		
        /* calculate offset based on highlighted value */		
        offset = highlight_option / 5;
        offset = offset * 5;
        
        /* display the profile selections */
        for (int i=offset;i<offset+5;i++) 
        { 
          /* if we are currently drawing what we have selected */
          if(highlight_option == i) 
          { 
            /* highlight the option */
            display.setTextColor(BLACK, WHITE); 
          } 
          /* otherwise */
          else 
          {
            /* draw the text normally */
            display.setTextColor(WHITE); 
          }
          /* go the the next line of the display  */
          display.setCursor(0,10+(i-offset)*10); 
          /* display the menu options as we progress throught the loop */
          display.print(profile_options[i]);
          /* if we are drawing the profile we selected */
          if(profile_selected == i){
            /* place indicator for currently selected profile */
            display.println("<");
            /* otherwise */
            }else{
              /* dont do anything*/
              display.println();
            } 
          }
          Serial.print("Profile Selected # ");
          Serial.println(profile_selected);
        
      /* send the display data to lcd */
      display.display();
      /* dont allow the lcd to update until another change occurs */
      option0_draw = true;
      }
      
      /* small delay otheriwse button presses occur too quickly */
      delay(75);
      
      /* if the user presses the select button */
      if (buttonState_0 == LOW){
        /* profile selected becomes the option we selected */
        profile_selected = highlight_option; 
        /* clear the display */
        display.clearDisplay(); 
        /* allow menu 1 to re-draw */
        option0_draw = false; 
      }

      /* if the user pressed the back button */
      buttonState_5 = digitalRead(buttonPin_5); 
      if(buttonState_5 == HIGH)
      {
        /* clear the display */
        display.clearDisplay();
        /* re-set the highlight to first selection */
        highlight_option = 0;
        /* allow main menu to re-draw */
        loop_draw = false;
        /* return to main menu */
        break;
      }
    }
  } //end option 0
  
  /* if the user selected display password */
  if(highlight_option == 1)
  {
    /* used to determine when the screen needs to be re-drawn */
    boolean option1_draw = false;
    /* clear the display */
    display.clearDisplay();
    /* set decrypt flag to false */
    boolean decryptDone = false;
    /* while we are in the display password menu */
    while(1)
    {  
      if(option1_draw == false)
      {
        /* set the text font size */
        display.setTextSize(1);
        /* set the text color */
        display.setTextColor(WHITE);
        /* set the cursor position */
        display.setCursor(0,0);
      
        /* if a profile has been selected */
        if(profile_selected != 99)
        {
          Serial.print("Profile To Display # ");
          Serial.println(profile_selected);
          
          /* and we have not already decrypted the password */
          if(decryptDone == false)
          {
            /* read all the passwords from sd */
            read_all_password_data();
            /* decrypted the current profile selected */
            decrypt(profile_selected);
            /* set decryption flag to true */
            decryptDone = true;
          }
        
          /* if the first character of the password is alphanumeric then display password, otherwise display blank password */
          if (isAlphaNumeric((char)plaintext[0])){
            /* display the decrypted password */
            for (uint8_t i=0; i < 16; i++) {
              /* display text to lcd */
              display.write((char)plaintext[i]);
            }              
          }else{
            display.println("No Password Set");
            /* send display data to lcd */
            display.display();     
          }

          /* clear non-encrypted data from memory */
          for(int i = 0; i <= 16; i++){
            plaintext[i] = ' ';
          }
                 
        }
        /* otherwise */
        else
        {
          /* display text to lcd */
          display.println("No profile selected.");
        }
      
        /* send display data to lcd */
        display.display();
     
        option1_draw = true; 
      }
      
      /* if the user pressed the back button */
      buttonState_5 = digitalRead(buttonPin_5); 
      if(buttonState_5 == HIGH)
      {
        /* clear the display */
        display.clearDisplay();
        /* set the highlight option to what is was in the previous menu */
        highlight_option = 1;
        /* allow the main menu to re-draw */
        loop_draw = false;
        /* return to main menu */
        break;   
      }   
    } 

  }// end option 1
  
  /* flag that controls when option 2 menu can re-draw */
  boolean option2_draw = false;

  /* if the user selected Generate Password */
  if(highlight_option == 2)
  {
    /* small delay to wait for select button to settle */
    delay(75);
    /* clear the display */
    display.clearDisplay();
    /* set the highlight option to the first selection */
    highlight_option = 0;
    /* while we are in the password generation menu */    
    while(1)
    {
      /* Read button inputs */
      buttonState_0 = digitalRead(buttonPin_0); /* IN */
      buttonState_1 = digitalRead(buttonPin_1); /* Left */
      buttonState_2 = digitalRead(buttonPin_2); /* Down */
      buttonState_3 = digitalRead(buttonPin_3); /* Up */
      buttonState_4 = digitalRead(buttonPin_4); /* Right */
      buttonState_5 = digitalRead(buttonPin_5); /* Switch */
      
      /* if the user pressed the down button */
      if(buttonState_2 == LOW)            
      {
        /* clear the display */
        display.clearDisplay();
        /* increment screen selection */
        highlight_option++; 
        /* allow option2 menu to re-draw */
        option2_draw = false;     
      }
      /* if the user pressed the up button */
      else if (buttonState_3 == LOW)   
      {
        /* clear the display */
        display.clearDisplay();
        /* decrement screen selection */
        highlight_option--;   
        /* allow option2 menu to re-draw */
        option2_draw = false;    
      }
      
      /* if we get to the last botton selection move to the top selection 
         if we get tot the last top selection move to bottom selection */
      if (highlight_option < 0) highlight_option = 0; 
      else if (highlight_option > 1) highlight_option = 1;
      
      /* re-draw the screen if something changed otherwise dont both re-drawing display */
      if(option2_draw == false)
      {
        /* set the text font size */
        display.setTextSize(1);
        /* set the text color */
        display.setTextColor(WHITE);
        /* */
        display.setCursor(40,0);
        /* display text to lcd */
        display.println("Menu");
  
        /* display the profile selections */
        for (int i=0;i<2;i++) 
        {
          /* if we are currently drawing what we have selected */
          if(highlight_option == i) 
          { 
            /* highlight the selected option */
            display.setTextColor(BLACK, WHITE); 
          } 
          /* otherwise */
          else 
          {
            /* draw the text normally */
            display.setTextColor(WHITE); 
          }
        /* go the the next line of the display  */
        display.setCursor(0,10+i*10); 
        /* display the menu options as we progress throught the loop */
        display.println(yes_no_options[i]); 
        }  
        /* send the display data to lcd */
        display.display();
        /* allow the option 2 menu to re-draw */
        option2_draw = true;
      }
      
      /* small delay otheriwse button presses occur too quickly */
      delay(75);
      
      /* if the user presses the select button */
      if (buttonState_0 == LOW){
        /* if the user chooses no */
        if(highlight_option == 0)
        {
          /* clear the display */
          display.clearDisplay();
          /* set the highlight option to what is was in the previous menu */
          highlight_option = 2;
          /* allow the main menu to re-draw */
          loop_draw = false;
          /* small ammount of delay */
          delay(50);
          /* return to main menu */
          break;       
        }
        
        /* if the user chooses yes */
        if(highlight_option == 1)
        {
          /* if there is already a profile selected */
          if(profile_selected != 99)
          {
            /* generate a new password */
            generate_new_password();
            /* encrypt the password and store the password in the profile selected */
            encrypt(profile_selected);
            /* delete the file that stores the passwords */
            SD.remove("test.txt");
            /* write all the passwords to the sd card */
            write_to_sd();
            /* clear the display */
            display.clearDisplay();
            /* set the text font size */
            display.setTextSize(1);
            /* set the text color */
            display.setTextColor(WHITE);
            /* set the cursor position */
            display.setCursor(0,0);
            /* display text to screen */
            display.println("New Password Created");
            /* send the display data to lcd */
            display.display();
            /* give the user 1 seconds to read the message */
            delay(1000);
            /* clear the display */
            display.clearDisplay();
            /* allow the loop menu to re-draw */
            loop_draw = false;
            /* set the highlight option to what is was in the previous menu */
            highlight_option = 2;
            /* return to main menu  */
            break; 
          /* */            
          }else{
            /* clear the display */
            display.clearDisplay();
            /* set the text font size */
            display.setTextSize(1);
            /* set the text color */
            display.setTextColor(WHITE);
            /* set the cursor position */
            display.setCursor(0,0);  
            /* display text to the lcd */
            display.println("No profile selected.");
            /* send the display data to lcd */
            display.display(); 
            /* give the user 1 seconds to read the message */
            delay(1000);
            /* clear the display */
            display.clearDisplay();
            /* set the highlight option to what is was in the previous menu */
            highlight_option = 2;
            /* allow the loop menu to re-draw */
            loop_draw = false;
            /* return to main menu */
            break;             
          }
        }
      }  
     
      /* if the user pressed the back button */
      buttonState_5 = digitalRead(buttonPin_5); 
      if(buttonState_5 == HIGH)
      {
        /* clear the display */
        display.clearDisplay();
        /* set the highlight option to what it was in the previous menu */
        highlight_option = 2;
        /* allow the main menu to re-draw */
        loop_draw = false;
        /* return to main menu */
        break;
      }
    }  
  }// end option 2

  /* if the user chooses to transmit password to pc */
  if(highlight_option == 3)
  {
    /* while we are in the transmit password menu */
    while(1)
    {
      /* clear the display */
      display.clearDisplay();
      /* set the text size */
      display.setTextSize(1);
      /* set the text color */
      display.setTextColor(WHITE);
      /* set the cursor position */
      display.setCursor(0,0); 
      
      /* if a profile has been selected */
      if(profile_selected != 99)
      {
        /* read all the passwords from the sd */
        read_all_password_data();
        /* decrypt password for currently selected profile */
        decrypt(profile_selected);
        
        // checks that the caps lock bit is set
        if (keyboard_leds & (1<<USB_LED_CAPS_LOCK)) 
        {
          //If caps lock is on, turn it off.
          Keyboard.press(KEY_CAPS_LOCK);
          delay(100);
           Keyboard.releaseAll();
         }else{
           //Do Nothing
         }

       
          /* if the first character of the password is alphanumeric then type the password, otherwise type blank password */
          if (isAlphaNumeric((char)plaintext[0])){
            /* transmit passowrd to pc */
            for (uint8_t i=0; i < 16; i++) {
              /* using the keyboard interface type the password */
              Keyboard.print((char)plaintext[i]);
            }             
          }else{
            Keyboard.println("No Password Set, Please Generate A Password For This Account.");   
          }
       
        

        /* clear non-encrypted data from memory */
        for(int i = 0; i <= 15; i++){
          plaintext[i] = ' ';
        }        
        
        /* display text to lcd */
        display.println("Sent Password to PC");
        /* send the display data to lcd */
        display.display();
        /* small delay for user to read text */
        delay(500);
        /* clear the display */
        display.clearDisplay();
        /* set the highlight option to what is was in the previous menu */
        highlight_option = 3;
        /* allow the main menu to re-draw */
        loop_draw = false;
        /*return to main menu  */
        break;
      /* otherwise */     
      }else{
        /* display text to lcd */
        display.println("No profile selected.");
        /* send display data to lcd */
        display.display();
        /* small delay for user to read text */
        delay(2000);
        /* clear the display */
        display.clearDisplay();
        /* set the highlight option to what it was in the previous menu */
        highlight_option = 3;
        /* allow the main menu to re-draw */
        loop_draw = false;
        /* return to main menu */
        break;         
      }
      
      /* if the user pressed the back button */
      buttonState_5 = digitalRead(buttonPin_5); 
      if(buttonState_5)
      {
        /* clear the display */
        display.clearDisplay();
        /* set the highlight option to what is was in the previous menu */
        highlight_option = 3;
        /* allow the main menu to re-draw */
        loop_draw = false;
        /* return to main menu */
        break;
      }
    } 
  }
}


void reNameProfile()
{
  
    display.clearDisplay();
    /* set the text size */
    display.setTextSize(1);
    /* set the text color */
    display.setTextColor(WHITE);
    /* set the cursor position */
    display.setCursor(24,0); 
    display.println("Rename Profile");
    /* send display data to lcd */
    display.display();
    
    boolean updateDisplay = false;
    int character = 1;
    int characterPosition = 0;
    char name[15] = {'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};
  
  
    display.setCursor(0,20);
    while(1)
    {
      /* Read button inputs */
      buttonState_0 = digitalRead(buttonPin_0); /* IN */
      buttonState_1 = digitalRead(buttonPin_1); /* Left */
      buttonState_2 = digitalRead(buttonPin_2); /* Down */
      buttonState_3 = digitalRead(buttonPin_3); /* Up */
      buttonState_4 = digitalRead(buttonPin_4); /* Right */
      buttonState_5 = digitalRead(buttonPin_5); /* Switch */
      

      /* if the user pressed the down button */
      if(buttonState_2 == LOW)            
      {
        /* decrease the character count */
        character --;
        
        if(character < 0)
          character = 27;
          
        refrenceCharacter(character); 
        
        name[characterPosition] = refrenceCharacter(character);
       
        updateDisplay = true; 
        
      }
      /* if the user pressed the up button */
      else if (buttonState_3 == LOW)   
      {
        /* increment the character count */
        character ++;

        if(character > 27)
          character = 1;
        
        refrenceCharacter(character);
        
        name[characterPosition] = refrenceCharacter(character);
        
        updateDisplay = true;
        
      }
      
      /* if the select button is pressed */
      if (buttonState_0 == LOW)
      {       
        name[characterPosition] = refrenceCharacter(character);
        if(characterPosition < 14){
          characterPosition += 1;
        }
        delay(75);
        updateDisplay = true; 
      }

      /* if the Left button is pressed */
      if (buttonState_1 == LOW)
      {
        if(characterPosition >= 1){
          characterPosition -= 1;
        }
        delay(75);
        updateDisplay = true;
      }

      /* if the Right button is pressed */
      if (buttonState_4 == LOW)
      {
        if(characterPosition >= 1){
          characterPosition += 1;
        }
        delay(75);
        updateDisplay = true;
      }
      
      if(updateDisplay  == true){
         display.clearDisplay();
         /* set the text size */
         display.setTextSize(1);
         /* set the text color */
         display.setTextColor(WHITE);
         /* set the cursor position */
         display.setCursor(24,0); 
         /* print the header at the top of the screen*/
         display.println("Rename Profile");
         /* set the cursor postion */
         display.setCursor(0,20);
         /* set the text size */
         display.setTextSize(1);
         /* itterate over characters in name and hightlight the character selected */ 
         for(int i = 0; i < 14; i++)
         {
           if(characterPosition == i) 
           { 
             /* highlight the option */
             display.setTextColor(BLACK, WHITE); 
           } 
           /* otherwise */
           else 
           {
             /* draw the text normally */
             display.setTextColor(WHITE); 
           }
           display.print(name[i]);
         }
 
         display.display();  
         updateDisplay = false;       
      }

      delay(75);


      /* if the user pressed the back button */
      buttonState_5 = digitalRead(buttonPin_5); 
      if(buttonState_5 == HIGH)
      {
        /* save yes/no dialog needs to appear */

        profile_options[profile_selected] = String(name);
        /* delete the menu text file, so we can replace it */
        SD.remove("menu.txt");
        /* small ammount of delay to finish delete operation*/
        delay(5);
        /* re-write the menu file to the SD card */
        writeMenu();      
        /* clear the display */
        display.clearDisplay();
        /* break and return to menu selection */  
        break;
      }

    }

}

char refrenceCharacter(int val)
{
    switch(val)
    {
      case 1:
         return 'A';
         break;
      case 2:
         return 'B';
         break;  
      case 3:
         return 'C';
         break;    
      case 4:
         return 'D';
         break;    
      case 5:
         return 'E';
         break; 
      case 6:
         return 'F';
         break;  
      case 7:
         return 'G';
         break;  
      case 8:
         return 'H';
         break;  
      case 9:
         return 'I';
         break; 
      case 10:
         return 'J';
         break;            
      case 11:
         return 'K';
         break;  
      case 12:
         return 'L';
         break;  
      case 13:
         return 'M';
         break;  
      case 14:
         return 'N';
         break;  
      case 15:
         return 'O';
         break;  
      case 16:
         return 'P';
         break;  
      case 17:
         return 'Q';
         break;  
      case 18:
         return 'R';
         break;  
      case 19:
         return 'S';
         break;  
      case 20:
         return 'T';
         break;  
      case 21:
         return 'U';
         break;  
      case 22:
         return 'V';
         break;  
      case 23:
         return 'W';
         break;  
      case 24:
         return 'X';
         break;            
      case 25:
         return 'Y';
         break;  
      case 26:
         return 'Z';
         break;
      case 27:
         return ' ';
         break;
      default:
         return ' ';
         break;                
    }  
}






