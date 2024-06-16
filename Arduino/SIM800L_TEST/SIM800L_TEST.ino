/*
 *    Teste SIM800L
 *    AUTOR:   Carlos Eduardo Mayer de Oliveira
 *    DATA:    08/06/2023
*/

void setup()
{
 
  Serial.begin(9600);
  while (!Serial);

  Serial1.begin(9600);
  while (!Serial1);
  
  Serial.println("Inicializando...\n"); 
  delay(1000);

  //Serial.println("ATI"); 
  Serial1.println("ATI"); 
  updateSerial();
  delay(1000);
  
  // Serial.println("AT+CMGF=1"); 
    Serial1.println("AT+CMGF=1"); 
    updateSerial();
    delay(1000);
  
  

  Serial.println("AT+CNMI=2,2,0,0,0");
  Serial1.println("AT+CNMI=2,2,0,0,0");
  updateSerial();
  delay(1000);

  Serial1.println("AT+CMGS=\"+5541987785995\"");
  updateSerial();
  delay(1000);

  Serial1.println("Oi minha Filha...");
  updateSerial();
  delay(500);

  Serial1.println((char)26);
  updateSerial();
  delay(500);

  Serial1.println("\nMensagem Enviada!\n");

}

void loop()
{
  updateSerial();
}

void updateSerial()
{  
  delay(100);
  while (Serial.available()) 
  {
    Serial1.write(Serial.read());
  }
  while(Serial1.available()) 
  { 
    Serial.write(Serial1.read());
  }
}
