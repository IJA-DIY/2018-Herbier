/*
 *  Herbier interactif
 */


 /*  
 * Typical pin layout used (NFC) :
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */

// attachInterrupt possible sur Arduino Leonardo sur les pins 0,1,2,3,7
// VISIBLEMENT, les interruptions sont AUSSI attachées aux touch 

// compiler error handling
#include "Compiler_Errors.h"

// librairies à inclure
#include <MPR121.h>
#include <Wire.h>

#include <SPI.h>
#include <SdFat.h>
#include <SFEMP3Shield.h>

#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

boolean toPlay;

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
char PREFIXE[3]="__";
char son[13]="___011.mp3"; // son par DEFAUT (à changer)

MFRC522::MIFARE_Key key; 
// Init array that will store new NUID 
byte nuidPICC[4];

// variables mp3 
SFEMP3Shield MP3player;
byte result;

// sd card instantiation
SdFat sd;

/**                          */
void setup() { 
  // pinMode (BTN1, INPUT); // Bouton 1 en mode entrée
  toPlay=false;
  
  Serial.begin(57600);
  //while (!Serial) {}; // wait for serial port to connect. Needed for native USB port only
  
  SPI.begin(); // Init SPI bus

  Serial.println(F("This code does nothing very interesting ;)"));
  // détecte un tag correspondant à une feuille

  if(!sd.begin(SD_SEL, SPI_HALF_SPEED))
    sd.initErrorHalt();

  if(!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);

  MPR121.setTouchThreshold(40);
  MPR121.setReleaseThreshold(20);

  result = MP3player.begin();
  MP3player.setVolume(0,0);
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

/**                          */ 
void loop() {
        
  // play a file ?
  if (toPlay == true){
     playFile(); 
  }  
  // lire les entrées touch 
  readTouchInputs();

  if (!MP3player.isPlaying()) {
  Serial.println("tentative NFC");  
  // savoir si on a une nouvelle feuille --> lecture d'un tag potentiel
  if ( ! rfid.PICC_IsNewCardPresent()) {
    // Serial.println("Pas de nouveau tag");
  }
  else {    
    // Verify if the NUID has been readed
    if ( ! rfid.PICC_ReadCardSerial()) {
      //  Serial.println("Tag déjà lu --> on ne fait rien");
    }
    else {
      // Serial.println("Try to read a card");      
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      if (rfid.uid.uidByte[0] != nuidPICC[0] || rfid.uid.uidByte[1] != nuidPICC[1] || rfid.uid.uidByte[2] != nuidPICC[2] || rfid.uid.uidByte[3] != nuidPICC[3] ) {
        // A new card has been detected and store NUID into nuidPICC array
        for (byte i = 0; i < 4; i++) {
          nuidPICC[i] = rfid.uid.uidByte[i];
        }
        // récupérer le préfixe pour les sons
        getPrefixe(rfid.uid.uidByte, rfid.uid.size);
        strcpy(PREFIXE, getPrefixe(rfid.uid.uidByte, rfid.uid.size));    
      }   
    }
  }
       // Halt PICC
      rfid.PICC_HaltA();
      // Stop encryption on PCD
      rfid.PCD_StopCrypto1();
  }
}
void playFile() {
  if(MP3player.isPlaying()){ // si un son est joué, on l'arrête
    MP3player.stopTrack();
  }
    toPlay=false;
    Serial.println(son);
    // joue le son à jouer
    result = MP3player.playMP3(son,0); 
     
}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
char* getPrefixe(byte *buffer, byte bufferSize) {
  char tag[30]="";

  // lire le tag et affecter le préfixe pour lire les sons
  for (byte i = 0; i < bufferSize; i++) {
    sprintf(tag, "%s%x",tag, buffer[i]);
  }
  Serial.print("nouveau tag lu : "); 
  Serial.println(tag);
      
  if (strcmp(tag, "4bf9dda8f5480") == 0)
      return("ma"); // Magnolia tag 5
  else {
    if (strcmp(tag, "4bf95da8f5480") == 0)
      return("ge"); // Gerbera tag 4
    else {
    if (strcmp(tag, "48dac42835681") == 0)
      return("te"); // Test    
     else
      return("__"); 
    }
  }     
}

/* LIRE LES ENTREES TOUCH */
void readTouchInputs(){    
  if(MPR121.touchStatusChanged()){    
    MPR121.updateTouchData();
    // only make an action if we have one or fewer pins touched ignore multiple touches    
    if(MPR121.getNumTouches()<=1){
      for (int i=0; i < 12; i++){  // Check which electrodes were pressed
        if(MPR121.isNewTouch(i)){                                
            switch (i) {
              case 0: // Bouton 1
                sprintf(son,"%s_btn1.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 1: // Bouton 2
                sprintf(son,"%s_btn2.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 2: // Bouton 3
                sprintf(son,"%s_btn3.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 3: // Zone de la feuille pin 3
                sprintf(son,"%s_003.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 4: // Bouton 4
                sprintf(son,"%s_btn4.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 5: // Bouton 5
                sprintf(son,"%s_btn5.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 6: // Zone de la feuille pin 6
                sprintf(son,"%s_006.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 7: // Zone de la feuille pin 7
                sprintf(son,"%s_007.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 8: // Bouton 6
                sprintf(son,"%s_btn6.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 9: // Bouton 7
                sprintf(son,"%s_btn7.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 10: // Bouton 8
                sprintf(son,"%s_btn8.mp3", PREFIXE);
                toPlay=true;                
                break;

              case 11: // Bouton 9
                sprintf(son,"%s_btn9.mp3", PREFIXE);
                toPlay=true;                
                break;

              default:
                break;  
            } 
          }         
         }      
      }
   }
}


