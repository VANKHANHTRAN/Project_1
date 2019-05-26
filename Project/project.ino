/*
 * RESET                9       RST
 * SPI          SDA     10      SDA
 * SPI          MOSI    11      MOSI
 * SPI          MISO    12      MISO
 * SPI          SCK     13      SCL
 * LCD - I2C
 * SCL                  A5
 * SDA                  A4
*/

#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10 //set chan chon chip cho RFID 
#define RST_PIN 9 // chan reset cua RFID

LiquidCrystal_I2C lcd(0x3F,2,1,0,4,5,6,7,3,POSITIVE);   //khoi dong LCD voi module I2C
MFRC522 rfid(SS_PIN, RST_PIN); 
Servo myservo;  //Tao bien myservo co kieu servo. Tuong tu typedef trong C 
//unsigned long uidDEC, uidDECTemp; //hien thi UID duoi dang DEC

//Cac bien va mang can dung
byte nuidPICC[4];
byte knownTag[4] = {0xAB,0x6E,0xA9,0x89};
//  {0xB7,0xFA,0x4C,0x79}

MFRC522::MIFARE_Key key;
byte trailerBlock = 3;
byte buffer1[18];
byte buffer2[18]; 
byte buffer3[18];
boolean KNOW = true;
byte block;
char n[3];

void setup() 
{ 
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  Serial.begin(9600);   //khoi dong giao tiep Serial voi toc do baud 9600
  SPI.begin(); // Khoi dong giao tiep SPI
  rfid.PCD_Init(); // Khoi dong module MFRC522
  myservo.attach(5);  // chan ket noi servo la chan 5
  myservo.write(0); //Goc cua dong co luc dau
  
  lcd.begin(16,2);
  lcd.backlight();
  lcd.setCursor(4,0);
  lcd.write("XIN CHAO");
  delay(3000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.write("QUET THE TAI DAY");
  delay(3000);
  lcd.clear();
}

void loop() 
{
  
  // Cho co the quet 
  if ( ! rfid.PICC_IsNewCardPresent())
  {
    return;
  }
  if ( ! rfid.PICC_ReadCardSerial())
  {
    return;
  }
  
  // Kiem tra neu the duoc quet 2 lan
  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) 
    {
      //Store NUID into nuidPICC array
      for (byte i = 0; i < 4; i++) 
      {
        nuidPICC[i] = rfid.uid.uidByte[i];
      }
    }
    else
    {
      lcd.setCursor(0,0);
      lcd.write("The da quet");
      delay(2000);
      lcd.clear();
      return 0;
    }
    
    // Doc du lieu tu cac block
    block = 1;
    readBlock(block,buffer1);
    block = 2;
    readBlock(block,buffer2);
    block = 4;
    readBlock(block,buffer3);
    
    for(int i = 0; i < 4; i++)
    { 
      if (knownTag[i] != nuidPICC[i])
      {
        KNOW = false;
        lcd.setCursor(0,0);
        lcd.write("INVALID CARD");
        delay(2000);
        lcd.clear();
        return 0;
      }
      else
      {
        KNOW = true;
      }
    }
      
    lcd.setCursor(0,0);
    lcd.write("VALID CARD");
    delay(1000);
    lcd.clear();
    
    for(uint8_t i = 0; i < 2; i++)
      {
        if(buffer3[i] != 32)
        {
          n[i] = buffer3[i];
        }
      }
      if(n[1] != 48)
      {
        n[1] = n[1] - 1;
      }
      else
      {
        n[0] = n[0] - 1;
        n[1] = 57;
      }
      
      block = 4;
      writeBlock(block, n);
      
    if(KNOW)
    {     
      lcd.setCursor(0,0);
      for (uint8_t a = 0; a < 16; a++)
      {    
        lcd.write(buffer1[a]);
      }
      lcd.setCursor(0,1);
      for (uint8_t a = 0; a < 16; a++)
      {
        lcd.write(buffer2[a]);
      }
      delay(2000);
      lcd.clear();
      
      if (n[0] < 48)
      {
        lcd.setCursor(0,0);
        lcd.write("The het han");
        delay(2000);
        lcd.clear();
      }
      
        //Dieu khien dong co mo va dong cua
        myservo.write(70);
        delay(2000);
        myservo.write(0);
      
        //Xuat so lan quet con lai
        lcd.setCursor(0,0);
        lcd.write("So lan: ");
        lcd.setCursor(8,0);
        for (uint8_t a = 0; a < 2; a++)
        {
          lcd.write(n[a]);
        }
        delay(2000);
        lcd.clear();     
    }

  // Cho lan quet the tiep theo
  rfid.PICC_HaltA();

  // Dung giai ma the
  rfid.PCD_StopCrypto1();
}

// Ham ghi du lieu
int writeBlock(int blockNumber, byte arrayAddress[]) 
{
  //kiem tra block can ghi khong phai là block trailer
  int largestModulo4Number=blockNumber/4*4;
  int trailerBlock=largestModulo4Number+3;
  if (blockNumber > 2 && (blockNumber+1)%4 == 0)
  {
    return 2;//tra ve nhu mot tin nhan loi
  }
  
  byte status = rfid.MIFARE_Write(blockNumber, arrayAddress, 16); // valueBlockA la khoi du lieu, ghi du lieu trong mang arrayAdress vao blockNumber va du lieu co 16 bytes
  //status = mfrc522.MIFARE_Write(9, value1Block, 16);
  if (status != MFRC522::STATUS_OK) 
  {
    Serial.print("MIFARE_Write() failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return 4;//return "4" as error message
  }
  Serial.println("block was written");
}

//Hàm đọc dữ liệu
int readBlock(int blockNumber, byte arrayAddress[]) 
{
  int largestModulo4Number=blockNumber/4*4; //CHIA LAY PHAN NGUYEN. VD: 5/4 = 1, 1*4 = 4, 4+3 = 7
  int trailerBlock=largestModulo4Number+3;//determine trailer block for the sector

  /*****************************************authentication of the desired block for access***********************************************************/
  byte status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(rfid.uid));
  
  if (status != MFRC522::STATUS_OK) 
  {
         Serial.print("PCD_Authenticate() failed (read): ");
         Serial.println(rfid.GetStatusCodeName(status));
         return 3;//return "3" as error message
  }
         
  byte buffersize = 18;//we need to define a variable with the read buffer size, since the MIFARE_Read method below needs a pointer to the variable that contains the size... 
  status = rfid.MIFARE_Read(blockNumber, arrayAddress, &buffersize);//&buffersize is a pointer to the buffersize variable; MIFARE_Read requires a pointer instead of just a number
  if (status != MFRC522::STATUS_OK) {
          Serial.print("MIFARE_read() failed: ");
          Serial.println(rfid.GetStatusCodeName(status));
          return 4;//return "4" as error message
  }
  Serial.println("block was read");
}
