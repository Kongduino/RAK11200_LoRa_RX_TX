// Define LoRa parameters
#define RF_FREQUENCY 868300000 // Hz
#define TX_OUTPUT_POWER 22 // dBm
#define LORA_BANDWIDTH 0 // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1 // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0 // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 30000
#define TX_TIMEOUT_VALUE 6000

#include <Arduino.h>
#include <SX126x-Arduino.h> //http://librarymanager/All#SX126x
#include <SPI.h>
#include "aes.c"
#include "Sx1262LoRandom.h"

// Function declarations
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnRxTimeout(void);
void OnRxError(void);
// Function declarations
void OnTxDone(void);
void OnTxTimeout(void);
void OnCadDone(bool);

static RadioEvents_t RadioEvents;
static uint8_t TxdBuffer[64];
static uint8_t RcvBuffer[64];
int16_t myRSSI;
int8_t mySNR;
uint32_t cadTime;
char buf[256] = {0};
char encBuf[256] = {0}; // Let's make sure we have enough space for the encrypted string
char decBuf[256] = {0}; // Let's make sure we have enough space for the decrypted string
uint8_t pKey[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint8_t pKeyLen = 16;

void hexDump(unsigned char *, uint16_t);
void fillRandom(uint8_t *, uint8_t);

void setup() {
  pinMode(WB_IO2, OUTPUT);
  digitalWrite(WB_IO2, HIGH);
  // Initialize Serial for debug output
  time_t timeout = millis();
  Serial.begin(115200);
  while (!Serial) {
    if ((millis() - timeout) < 5000) {
      delay(100);
    } else {
      break;
    }
  }
  Serial.println("=====================================");
  Serial.println("LoRaP2P Rx Test");
  Serial.println("=====================================");
  // Initialize the Radio callbacks
  RadioEvents.RxDone = OnRxDone;
  RadioEvents.TxDone = OnTxDone;
  RadioEvents.TxTimeout = OnTxTimeout;
  RadioEvents.RxTimeout = OnRxTimeout;
  RadioEvents.RxError = OnRxError;
  RadioEvents.CadDone = OnCadDone;
  // Initialize LoRa chip.
  lora_rak13300_init();
  // Initialize the Radio
  Radio.Init(&RadioEvents);
  // Set Radio channel
  Radio.SetChannel(RF_FREQUENCY);
  // Set Radio RX configuration
  Radio.SetRxConfig(
    MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
    LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
    0, true, 0, 0, LORA_IQ_INVERSION_ON, true);
  Radio.SetTxConfig(
    MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
    LORA_SPREADING_FACTOR, LORA_CODINGRATE,
    LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
    true, 0, 0, LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);
  // Start LoRa
  Serial.println("Starting Radio.Rx");
  Radio.Rx(RX_TIMEOUT_VALUE);

  char *msg = "Hello user! This is a plain text string!";
  uint8_t msgLen = strlen(msg);
  // please note dear reader – and you should RTFM – that this string's length isn't a multiple of 16.
  // but I am foolish that way.
  Serial.println("Plain text:");
  hexDump((unsigned char *)msg, msgLen);
  Serial.println("pKey:");
  hexDump(pKey, 16);
  uint8_t IV[16] = {1};
  double t0, t1;

  uint16_t olen;
  uint32_t count = 0;
  t0 = millis();
  while (true) {
    olen = encryptECB((uint8_t*)msg, strlen(msg));
    count++;
    t1 = millis() - t0;
    if (t1 > 999) break;
  }
  sprintf(buf, "%d ECB Encoding rounds in 1 second:", count);
  Serial.println(buf);
  hexDump((unsigned char *)encBuf, olen);
  memcpy(decBuf, encBuf, olen);

  count = 0;
  t0 = millis();
  while (true) {
    olen = decryptECB((uint8_t*)decBuf, olen);
    count++;
    t1 = millis() - t0;
    if (t1 > 999) break;
  }
  sprintf(buf, "%d ECB Decoding rounds in 1 second:", count);
  Serial.println(buf);
  hexDump((unsigned char *)encBuf, olen);
}

void loop() {
  // Put your application tasks here, like reading of sensors,
  // Controlling actuators and/or other functions.
}

/**@brief Function to be executed on Radio Rx Done event
*/
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
  Serial.println("OnRxDone");
  delay(10);
  memcpy(RcvBuffer, payload, size);
  mySNR = snr;
  myRSSI = rssi;
  Serial.printf("RssiValue=%d dBm, SnrValue=%d\n", rssi, snr);
  hexDump(payload, size);
  send();
  Radio.Rx(RX_TIMEOUT_VALUE);
}

/**@brief Function to be executed on Radio Rx Timeout event
*/
void OnRxTimeout(void) {
  Serial.println("OnRxTimeout");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

/**@brief Function to be executed on Radio Rx Error event
*/
void OnRxError(void) {
  Serial.println("OnRxError");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

/**@brief Function to be executed on Radio Tx Done event
*/
void OnTxDone(void) {
  Serial.println("OnTxDone");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

/**@brief Function to be executed on Radio Tx Timeout event
*/
void OnTxTimeout(void) {
  Serial.println("OnTxTimeout");
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void send() {
  memset(TxdBuffer, 0, 64);
  sprintf((char*)TxdBuffer, "Received at RSSI %d, SNR %d", myRSSI, mySNR);
  uint8_t bufLen = strlen((char*)TxdBuffer) + 1;
  uint8_t remainderLen = 64 - bufLen;
  fillRandom(TxdBuffer + bufLen, remainderLen);
  Serial.println("Sending:");
  hexDump(TxdBuffer, 64);
  Radio.Standby();
  delay(500);
  Radio.SetCadParams(LORA_CAD_08_SYMBOL, LORA_SPREADING_FACTOR + 13, 10, LORA_CAD_ONLY, 0);
  cadTime = millis();
  Radio.StartCad();
}

void OnCadDone(bool cadResult) {
  time_t duration = millis() - cadTime;
  if (cadResult) {
    Serial.printf("CAD returned channel busy after %ldms\n", duration);
    Radio.Rx(RX_TIMEOUT_VALUE);
  } else {
    Serial.printf("CAD returned channel free after %ld ms\nSending...", duration);
    Radio.Send(TxdBuffer, 64); // strlen((char*)TxdBuffer)
    Serial.println(" done!");
  }
}

int16_t decryptECB(uint8_t* myBuf, uint8_t olen) {
  // Test the total len vs requirements:
  // AES: min 16 bytes
  // HMAC if needed: 28 bytes
  uint8_t reqLen = 16;
  if (olen < reqLen) return -1;
  uint8_t len;
  // or just copy over
  memcpy(encBuf, myBuf, olen);
  len = olen;
  struct AES_ctx ctx;
  AES_init_ctx(&ctx, pKey);
  uint8_t rounds = len / 16, steps = 0;
  for (uint8_t ix = 0; ix < rounds; ix++) {
    // void AES_ECB_encrypt(const struct AES_ctx* ctx, uint8_t* buf);
    AES_ECB_decrypt(&ctx, (uint8_t*)encBuf + steps);
    steps += 16;
    // decrypts in place, 16 bytes at a time
  } encBuf[steps] = 0;
  return len;
}

uint16_t encryptECB(uint8_t* myBuf, uint8_t len) {
  // first ascertain length
  uint16_t olen = len;
  struct AES_ctx ctx;
  if (olen != 16) {
    if (olen % 16 > 0) {
      if (olen < 16) olen = 16;
      else olen += 16 - (olen % 16);
    }
  }
  memset(encBuf, (olen - len), olen);
  memcpy(encBuf, myBuf, len);
  encBuf[len] = 0;
  AES_init_ctx(&ctx, pKey);
  uint8_t rounds = olen / 16, steps = 0;
  for (uint8_t ix = 0; ix < rounds; ix++) {
    AES_ECB_encrypt(&ctx, (uint8_t*)(encBuf + steps));
    steps += 16;
    // encrypts in place, 16 bytes at a time
  }
  return olen;
}
