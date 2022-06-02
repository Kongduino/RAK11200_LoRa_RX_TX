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

/*
  uint32_t SX126xGetRandom(void) {
  uint8_t buf[] = {0, 0, 0, 0};
  SX126xReadRegisters(RANDOM_NUMBER_GENERATORBASEADDR, buf, 4);
  return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
  }
*/
#define RANDOM_NUMBER_GENERATORBASEADDR 0x0819
#define REG_ANA_LNA 0x08E2
#define REG_ANA_MIXER 0x08E5
uint8_t randomStock[256], randomPos = 0;

void fillUp() {
  uint8_t regAnaLna = 0, regAnaMixer = 0;
  uint16_t cnt = 0;
  uint32_t number = 0;
  regAnaLna = Radio.Read(REG_ANA_LNA);
  Radio.Write(REG_ANA_LNA, regAnaLna & ~(1 << 0));
  regAnaMixer = Radio.Read(REG_ANA_MIXER);
  Radio.Write(REG_ANA_MIXER, regAnaMixer & ~(1 << 7));
  // Set radio in continuous reception
  Radio.Rx(0xFFFFFF);
  // SX126xSetRx(0xFFFFFF); // Rx Continuous
  while (cnt < 256) {
    Radio.ReadBuffer(RANDOM_NUMBER_GENERATORBASEADDR, (uint8_t*)&number, 4);
    // Serial.println("number 0x" + String(number, HEX));
    memcpy(randomStock + cnt, (uint8_t*)&number, 4);
    cnt += 4;
  }
  randomPos = 0;
  Radio.Standby();
  Radio.Write(REG_ANA_LNA, regAnaLna);
  Radio.Write(REG_ANA_MIXER, regAnaMixer);
  Radio.Rx(RX_TIMEOUT_VALUE);
}

void fillRandom(uint8_t *myBuff, uint8_t stockSize) {
  if (randomPos == 0) fillUp();
  uint8_t bytesLeft = 256 - randomPos;
  if (stockSize <= bytesLeft) {
    memcpy(myBuff, randomStock + randomPos, stockSize);
    randomPos += stockSize;
  } else {
    memcpy(myBuff, randomStock + randomPos, bytesLeft);
    fillUp();
    memcpy(myBuff + bytesLeft, randomStock, stockSize - bytesLeft);
    randomPos = stockSize - bytesLeft;
  }
}
