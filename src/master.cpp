#ifdef MASTER

#include "_util.h"
#include "SoftwareSerial.h"

// Pins softwareserial
#define RX_PIN 10
#define TX_PIN 11

// *** Objects *** // 

// Software Serial -- used as bus for encrypted communication
SoftwareSerial mySerial = SoftwareSerial(RX_PIN, TX_PIN);

// Encryption and decryption
const uint8_t MASTER_KEY[BLOCK_LEN] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}; // Master and slave should have the same hardcoded MASTER KEY
uint8_t iv[BLOCK_LEN]; // Need to fill during setup
uint8_t sk_init[BLOCK_LEN]; // Need to fill during setup
uint8_t session_key[BLOCK_LEN]; // Need to fill during setup

aes_context AES_CTX = NULL; // Used for chatbox encryption and decryption -- not needed during setup

/**
 * Fill the sk_init array with random numbers
 *
 * @param void
 * @return void
 */
void gen_sk_init(){
  gen_random_block(sk_init);
}

/**
 * Fill the initialization vector (IV) array with random numbers.
 *
 * @param void
 * @return void
 */
void gen_iv(){
  gen_random_block(iv);
}

/**
 * Write a message on the softserial bus.
 *
 * @param msg pointer to the message char array (length of char array must be multiple of 16 bytes).
 * @param numBlocksOfLength16 number of blocks of 16 bytes in the message char array
 * @return void
 */
void write_msg(uint8_t *msg, uint8_t numBlocksOf16Bytes){
  for (uint8_t i = 0; i < numBlocksOf16Bytes; i++)
  {
    for (uint8_t i = 0; i < BLOCK_LEN; i++)
    {
      mySerial.write(msg[i]);
    }
  }
}

/**
 * Write a 'newline' on the softserial bus.
 * Serial.println
 * @param void
 * @return void
 */
void write_newline(){
  mySerial.println();
}

/**
 * Read a message from the softserial bus.
 * 
 * @param res pointer to char buffer to be filled with the message read from the softserial bus.
 * @return void
 */
void read_msg(char *res){
  for (uint8_t i = 0; i < BLOCK_LEN; i++)
  {
    res[i] = mySerial.read();
  }
}

/**
 * Read an encrypted message from the softserial bus and decrypt it.
 *
 * @param msg pointer to the char buffer to be filled with the received and decrypted message
 * @return void
 */
void read_enc_msg(char *msg){
  char res[BLOCK_LEN];
  mySerial.listen();
  read_msg(res);
  decrypt(res, session_key, iv, msg);
}

/**
 * Encrypt a plain text message and send it to the softserial bus.
 *
 * @param msg pointer to the char buffer containing the plain text to be encrypted and transmitted
 * @return void
 */
void write_enc_msg(char *msg){
  char res[BLOCK_LEN];

  encrypt(res, session_key, iv, msg);
  write_msg((uint8_t *)res, 1);
}

/**
 * Print a char array (length of 16 bytes) as decimals in the serial monitor.
 *
 * @param msg pointer to the char array to be printed in the serial monitor
 * @return void
 */
void print_uint8_arr(uint8_t *arr){
  String tmp = "";
  for (uint8_t i = 0; i < BLOCK_LEN; i++)
  {
    tmp += arr[i];
    tmp += "\t";
  }
  mySerial.println(tmp);
}

/**
 * Authentication master - slave
 * 
 * The master and slave need to agree upon the same SESSION KEY to enable encrypted communication.
 * The master must authenticate the slave and the slave must authenticate the master.
 *
 * @param void
 * @return void
 */
void auth(){

#ifdef DEBUG
  Serial.println("============ INIT AUTH PROC ============");
  Serial.println("================ MASTER ================");
#endif

  mySeria.listen();
  while(mySerial.)

#ifdef DEBUG
  Serial.println(F("============ END AUTH PROC ============"));
#endif
}

void setup()
{
  // Serial
  Serial.begin(57600);
  Serial.println(F(""));
  Serial.println(F("---------------------------------------------"));
  
  // You have 5000 seconds to send a message after typing otherwise it will timeout
  Serial.setTimeout(5000);

  // Configurate and start softserial 
  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);
  mySerial.begin(SOFT_SERIAL_BAUDRATE);

  // Fill the initialization vector (IV) and sk_init array with random nummbers
  gen_iv();
  gen_sk_init();

  // Start key generation and authentication process
  auth();

  // Make a static AES context with the session key and IV 
  // Used for message encryption in the chatbox
  AES_CTX = aes128_cbc_enc_start(session_key, iv);

  // Start listening on the softserial bus
  mySerial.listen();
}

void loop()
{
  // Check if a message needs to be send
  if (Serial.available() > 0){
    // Blocks until timeout

    // Read string from the serial monitor
    String s = Serial.readStringUntil('\n');

#ifdef DEBUG
    Serial.print("Read from keyboard: ");
    Serial.println(s);
#endif
    
    // Re-init AES context
    AES_CTX = aes128_cbc_enc_start(session_key, iv);

    // Get length of the message
    uint16_t num_chars = s.length() - 1;

    // Cut message in pieces of 16 bytes
    uint16_t num_blocks = ceil(double(num_chars) / BLOCK_LEN);
    uint16_t total_chars = num_blocks * BLOCK_LEN;

    // Copy the string into a char array
    char str_buf[total_chars] = {0};
    for (uint8_t i = 0; i < num_chars; i++){
      str_buf[i] = s.charAt(i);
    }

    // Encrypt the message blocks (of 16 bytes) and place it again in str_buf
    aes128_cbc_enc_continue(AES_CTX, str_buf, total_chars);

    // Send the encrypted message to the slave
    write_msg((uint8_t*)str_buf, num_blocks);
    write_newline();

    // Start listening again to the softserial bus
    mySerial.listen();

    // Free the AES_CTX structure
    aes128_cbc_enc_finish(AES_CTX);
  }

  // RX mode
  if (mySerial.available() > 0){
    // blocks till timeout

    // Read string from the softserial bus
    String s = mySerial.readStringUntil('\n');

#ifdef DEBUG
    Serial.print("Read from chatbox: ");
    Serial.println(s);
#endif

    // Re-init AES context
    AES_CTX = aes128_cbc_dec_start(session_key, iv);

    // Get length of the string message
    uint16_t num_chars = s.length();

    // Cut message in pieces of 16 bytes
    uint16_t num_blocks = ceil(double(num_chars) / BLOCK_LEN);
    uint16_t total_chars = num_blocks * BLOCK_LEN;

    // Place string into a char array
    char str_buf[total_chars];
    for (uint8_t i = 0; i < num_chars; i++)
    {
      str_buf[i] = s.charAt(i);
    }

    // Decrypted the received encrypted message and place it again in str_buf
    aes128_cbc_dec_continue(AES_CTX, str_buf, total_chars);

    // Print out the decrypted message for the master
    Serial.println(str_buf);

    // Free the AES_CTX structure
    aes128_cbc_dec_finish(AES_CTX);
  }
}
#endif