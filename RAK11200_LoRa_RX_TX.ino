/**
  @file LoRaP2P_RX.ino
  @author rakwireless.com
  @brief Receiver node for LoRa point to point communication
  @version 0.1
  @date 2021-08-21
  @copyright Copyright (c) 2020

*/
#include <Arduino.h>
#include <SX126x-Arduino.h> //http://librarymanager/All#SX126x
#include <SPI.h>

// Function declarations
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnRxTimeout(void);
void OnRxError(void);
// Function declarations
void OnTxDone(void);
void OnTxTimeout(void);
void OnCadDone(bool);

// Define LoRa parameters
#define RF_FREQUENCY 868300000  // Hz
#define TX_OUTPUT_POWER 22    // dBm
#define LORA_BANDWIDTH 0    // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1   // [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0 // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 30000
#define TX_TIMEOUT_VALUE 6000

static RadioEvents_t RadioEvents;
static uint8_t TxdBuffer[64];
static uint8_t RcvBuffer[64];
int16_t myRSSI;
int8_t mySNR;
uint32_t cadTime;

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
}

void loop() {
  // Put your application tasks here, like reading of sensors,
  // Controlling actuators and/or other functions.
}

void hexDump(uint8_t* buf, uint16_t len) {
  // Something similar to the Unix/Linux hexdump -C command
  // Pretty-prints the contents of a buffer, 16 bytes a row
  char alphabet[17] = "0123456789abcdef";
  uint16_t i, index;
  Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
  Serial.print(F("   |.0 .1 .2 .3 .4 .5 .6 .7 .8 .9 .a .b .c .d .e .f | |      ASCII     |\n"));
  for (i = 0; i < len; i += 16) {
    if (i % 128 == 0) Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
    char s[] = "|                                                | |                |\n";
    // pre-formated line. We will replace the spaces with text when appropriate.
    uint8_t ix = 1, iy = 52, j;
    for (j = 0; j < 16; j++) {
      if (i + j < len) {
        uint8_t c = buf[i + j];
        // fastest way to convert a byte to its 2-digit hex equivalent
        s[ix++] = alphabet[(c >> 4) & 0x0F];
        s[ix++] = alphabet[c & 0x0F];
        ix++;
        if (c > 31 && c < 128) s[iy++] = c;
        else s[iy++] = '.'; // display ASCII code 0x20-0x7F or a dot.
      }
    }
    index = i / 16;
    // display line number then the text
    if (i < 256) Serial.write(' ');
    Serial.print(index, HEX); Serial.write('.');
    Serial.print(s);
  }
  Serial.print(F("   +------------------------------------------------+ +----------------+\n"));
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
  Serial.printf("Sending:\n%s\n", (char*)TxdBuffer);
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
