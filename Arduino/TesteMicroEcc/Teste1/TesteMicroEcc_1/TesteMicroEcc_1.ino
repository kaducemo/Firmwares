


#define uECC_SUPPORTS_secp160r1 0
#define uECC_SUPPORTS_secp192r1 0
#define uECC_SUPPORTS_secp224r1 0
#define uECC_SUPPORTS_secp256r1 1
#define uECC_SUPPORTS_secp256k1 0

#define LED_ON  0
#define LED_OFF 1


#include <uECC.h>
#include <Crypto.h>
#include <AES.h>

#include <iostream>

using namespace std;

#define LED_BUILTIN_TOGGLE    digitalWrite(LED_BUILTIN,   !digitalRead(LED_BUILTIN))

static int RNG(uint8_t *dest, unsigned size) {
  // Use the least-significant bits from the ADC for an unconnected pin (or connected to a source of 
  // random noise). This can take a long time to generate random data if the result of analogRead(0) 
  // doesn't change very frequently.
  while (size) 
  {
    uint8_t val = 0;
    for (unsigned i = 0; i < 8; ++i) 
    {
      int init = analogRead(0);
      int count = 0;
      while (analogRead(0) == init) 
      {
        ++count;
      }
      
      if (count == 0) 
      {
         val = (val << 1) | (init & 0x01);
      } 
      else 
      {
         val = (val << 1) | (count & 0x01);
      }

    }
    *dest = val;
    ++dest;
    --size;
  }
  // NOTE: it would be a good idea to hash the resulting random data using SHA-256 or similar.
  return 1;
}




// static int RNG(uint8_t *dest, unsigned size) 
// {  
//   uint8_t val = 0;
//   while (size) 
//   {     
//     *dest = val++;
//     ++dest;
//     --size;
//   }  
//   return 1;
// }




void setup() {
  // put your setup code here, to run once:
  uECC_set_rng(&RNG);
  pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN,   LED_OFF);    //Inicia com LED desligado
  Serial.begin(115200); 
  Once();

}
void Once()
{
  uint8_t meuBuffer[256] = {0};

INICIO:
  while(!Serial.available());  //ESPERA por 'A'  
  Serial.readBytes(meuBuffer,Serial.available());
  if(meuBuffer[0] != 'A')
    goto INICIO;

  else
  {
    uint8_t private1[32] = {0};
    uint8_t private2[32] = {0};
    
    uint8_t public1[65] = {0};
    uint8_t public2[65] = {0};
    
    uint8_t secret1[32] = {0};
    uint8_t secret2[32] = {0};
    

    const struct uECC_Curve_t * curve = uECC_secp256r1(); // NOK      
    
    uECC_make_key(public1+1, private1, curve);  
    public1[0] = 0x04; // Adiciona Prefixo na chave

    Serial.write(public1, sizeof(public1)); //Envia a chave pela Serial

    while(Serial.available() != 65);  // Espera até que a chave Publica REMOTA seja completamente recebida 
    Serial.readBytes(public2,Serial.available());

    // SEGREDO 1    
    int r = uECC_shared_secret(public2+1, private1, secret1, curve);   
    
    if (r) 
    // Chave Valida
    {
      digitalWrite(LED_BUILTIN,   LED_ON);      
      Serial.write(secret1, sizeof(secret1)); //Envia a chave pela Serial
    }
    else
    // Chave Invalida
    {
      digitalWrite(LED_BUILTIN,   LED_OFF);
      return;
    }

    
    // Mensagem a ser criptografada (deve ser múltiplo de 16 bytes)
    uint8_t message[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};    
    uint8_t cypher[16] = {0};    

    //Criptografa a mensagem usando a chave (truncada nos primeiros 16 bytes) e envia em seguida
    AES128 aes128;    
    aes128.setKey(secret1,16);// Setting Key for AES 
    aes128.encryptBlock(cypher,message);//cypher->output block and plaintext->input block
    Serial.write(cypher, sizeof(cypher)); //Envia a mensagem pela Serial 
    goto INICIO;
  }
}

void loop() 
{
  if(Serial.available())
  {
  
    int i = 0;

    // put your main code here, to run repeatedly:
    // const struct uECC_Curve_t * curve = uECC_secp160r1(); // OK
    // const struct uECC_Curve_t * curve = uECC_secp256k1(); // NOK
    // const struct uECC_Curve_t * curve = uECC_secp192r1(); // NOK
    // const struct uECC_Curve_t * curve = uECC_secp224r1(); // NOK
    // const struct uECC_Curve_t * curve = uECC_secp256r1(); // NOK


  //uECC_secp160r1
    // uint8_t private1[21];
    // uint8_t private2[21];
    
    // uint8_t public1[40];
    // uint8_t public2[40];
    
    // uint8_t secret1[20];
    // uint8_t secret2[20];

  // secp256r1
    // uint8_t private1[32];
    // uint8_t private2[32];
    
    // uint8_t public1[65] = {0};
    // uint8_t public2[64];
    
    // uint8_t secret1[32];
    // uint8_t secret2[32];

  
    // unsigned long a = millis();
    // uECC_make_key(public1+1, private1, curve);  
    // public1[0] = 0x04;
    // for(i = 0; i < 64; i++)
    //   public1[i] = i;
      
    // unsigned long b = millis();

    // CHAVES 1
    // Serial.print("\nChave Publica 1: ");

    // Serial.write(public1,65);

    // for(i = 0; i < 64; i++)
      // Serial.print(public1[i]);
      // Serial.print(i);
  
  

    // Serial.print("\nChave Privada 1: ");
    // for(i = 0; i < 64; i++)
      // Serial.print(private1[i]);
    // Serial.print("\nCriadas em ");
    // Serial.println(b-a);

    // delay(1000);
    
    // CHAVES 2
    // a = millis();
    // uECC_make_key(public2, private2, curve);
    // b = millis();

    // Serial.print("\nChave Publica 2: ");
    // for(i = 0; i < 64; i++)
      // Serial.print(public2[i]);
  
  

    // Serial.print("\nChave Privada 2: ");
    // for(i = 0; i < 64; i++)
      // Serial.print(private2[i]);
    // Serial.print("\nCriadas em ");
    // Serial.println(b-a);  

    // delay(1000);
    

    // SEGREDO 1
    // a = millis();
    // int r = uECC_shared_secret(public2, private1, secret1, curve);
    // b = millis();
    
    // if (!r) 
    // {
      // Serial.print("\nFalhou Segredo 1\n");
      // return;
    // }
    // else
    // {
      // Serial.print("\nSegredo 1 Compartilhado em "); 
      // Serial.println(b-a);
    // } 

    // delay(1000);
    

    // SEGREDO 2
    // a = millis();
    // r = uECC_shared_secret(public1, private2, secret2, curve);
    // b = millis();
  

    // if (!r) 
    // {
      // Serial.print("\nFalhou Segredo 2\n");
      // return;
    // }
    // else
    // {
      // Serial.print("\nSegredo 2 Compartilhado em "); 
      // Serial.println(b-a);
    // }

    // delay(1000);


    // Compara Segredos    
    // if (memcmp(secret1, secret2, 20) != 0)   
      // Serial.print("\nSegredos NAO bateram!\n");    
    // else   
      // Serial.print("\nSegredos IDENTICOS");
      
    
    //  delay(5000);
    // LED_BUILTIN_TOGGLE;
  }  
}
