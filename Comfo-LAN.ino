/*
   Comfo LAN Interface

   ATTENTION:
         There is no warranty that this system will not damage your heating system!

   Authors Gero Schumacher (gero.schumacher@gmail.com) (basic implementation based on BSB LAN)
           Frederik Holst (bsb@code-it.de) (basic implementation based on BSB LAN)
           Air quality sensor implementation based on https://forum.arduino.cc/index.php?topic=350712.0
           ENC28J60 reset implementation based on https://github.com/ntruchsess/arduino_uip/issues/167
           Tobias Schuh
*/

//***********************************************************************
//***  Program-Version  ************************************************
//***********************************************************************

#define PrgVer 1.0

#include <UIPEthernet.h>
#include "utility\Enc28J60Network.h"
// ..utility\Enc28J60Network.h file -
// move readReg() subroutine def from private to public
#define NET_ENC28J60_ECON1                                0x1F
#define NET_ENC28J60_ECON1_RXEN                           0x04
#include <Arduino.h>
#include "Wire.h"

#include "Config.h"
#include "Defs.h"

#ifdef MQTTBrokerIP
#include "PubSubClient.h"
#endif

#define iaqaddress 0x5A

//***********************************************************************
//***  Declaration  *****************************************************
//***********************************************************************

bool badAir = false;
byte savedVentilationLevel;
int ventilationLevel = 3;
int airLoopCount = 0;

Enc28J60Network Enc28J60;
EthernetClient ethClient;
EthernetClient ethClientMqtt;

#ifdef MQTTBrokerIP
IPAddress MQTTBroker(MQTTBrokerIP);
PubSubClient MQTTClient(ethClientMqtt);
#endif
String MQTTPayload = "";
bool mqttFirst = true;

unsigned long lastMQTTTime = millis();
unsigned long lastAirQualityTime = millis();
unsigned long lastEthernetCheckTime = millis();

static const int numMqttValues = sizeof(mqttParameters) / sizeof(byte);

/* buffer to print output lines*/
#define OUTBUF_LEN  300
char outBuf[OUTBUF_LEN];
byte outBufLen = 0;

byte receiveBuffer[40];        // stores received bytes
byte lueftGetCommand[8] = { 0x07, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x07, 0x0F };
byte lueftSetCommand[30];

/**  ****************************************************************
    Function: outBufclear()
    Does:     Sets ouBufLen = 0 and puts the end-of-string character
              into the first buffer position.
    Pass parameters:
     none
    Parameters passed back:
     none
    Function value returned:
     none
   Global resources used:
     char outBuf[]
 * *************************************************************** */
void outBufclear(void) {
  outBufLen = 0;
  outBuf[0] = '\0';
}

/** *****************************************************************
    Function: findLine()
    Does:     Scans the command table struct for a matching line
              number (ProgNr) and returns the command code.
   Pass parameters:
    uint16  line     the requested match (ProgNr)
    uint16  startidx starting line (ProgNr) for search .
                     Works best if i>0 ;-)
    uint32 *cmd      pointer to 32-bit command code variable
   Parameters passed back:
    uint16 *cmd      32-bit command code value filled in
   Function value returned:
    -1        command (ProgNr) not found
     i >= 0   success, usable to index the matching table row
   Global resources used:
    none
 * *************************************************************** */
int findLine(uint16_t line
             , uint16_t start_idx
             , uint32_t *cmd)      // 32-bit command code
{
  int found;
  int i;
  uint32_t c;
  uint16_t l;

  // search for the line in cmdtbl
  i = start_idx;
  found = 0;
  do {
    c = pgm_read_dword(&cmdtbl[i].cmd); // command code
    if (c == CMD_END) break;
    l = pgm_read_word(&cmdtbl[i].line); // ProgNr
    if (l == line) {
      found = 1;
      break;
    }
    if (l > line) {
      break;
    }
    i++;
  } while (1);

  if (!found) {
    return -1;
  }
  if (cmd != NULL) *cmd = c;
  return i;
}

/** *****************************************************************
    Function:  query()
    Does:      Retrieves parameters from the heater controller.
               Addresses the controller parameters by line (ProgNr).
               The query() function can interrogate a whole range
               of ProgNrs, delimited by line_start and line_end
               inclusive.
    Pass parameters:
     uint16 linestart  begin to retrieve at this RogNr
     uint16 lineend    stop at this ProgNr
     bool   noprint    True:  do not display results in the web client
                       False: display results in the web client
   Parameters passed back:
     none
   Function value returned:
     result string
   Global resources used:
     char outBuf[]
     Serial instance
     bus    instance
     client instance
 * *************************************************************** */
char* query(uint16_t line_start  // begin at this line (ProgNr)
            , uint16_t line_end  // end with this line (ProgNr)
            , boolean no_print)  // display in web client?
{
  uint32_t c;        // command code
  uint16_t line;     // ProgNr
  int i = 0;
  int idx = 0;
  int retry;
  char *pvalstr = NULL;

  if (!no_print) {            // display in web client?
    ethClient.println("<p>"); // yes, begin HTML paragraph
  }
  for (line = line_start; line <= line_end; line++) {
    outBufclear();
    i = findLine(line, idx, &c);

    if (i >= 0) {
      idx = i;
      if (c != CMD_UNKNOWN) { // send only valid command codes
        retry = QUERY_RETRIES;

        unsigned int lineInt = c;
        ethClient.println("<p>");
        sprintf(outBuf + outBufLen, "ID: %d / Cmd: %02X - ", line, lineInt);
        ethClient.print(String(outBuf));

        String parameterTable[ParameterTableSize][3];

        while (retry) {
          if ( lueftungGetCommand(c, parameterTable)) {
            break;   // success, break out of while loop
          } else {
            Serial.println("query failed");
            retry--;          // decrement number of attempts
          }
        } // endwhile, maximum number of retries reached

        printParameterTableWeb(parameterTable, sizeof(parameterTable) / (3 * sizeof(String)));

        if (retry == 0)
          outBufLen += sprintf(outBuf + outBufLen, "%d query failed", line);
        ethClient.println("</p>");

      } else {
        Serial.println("unknown command");
        if (line_start == line_end) outBufLen += sprintf(outBuf + outBufLen, "%d unknown command", line);
      } // endelse, valid / invalid command codes
    } else {
      Serial.println("line not found");
      if (line_start == line_end) outBufLen += sprintf(outBuf + outBufLen, "%d line not found", line);
    } // endelse, line (ProgNr) found / not found
    if (outBufLen > 0) {
      if (!no_print) {  // display result in web client
        ethClient.println(outBuf);
        ethClient.println("<br>");
      }
    } // endif, outBufLen > 0
  } // endfor, for each valid line (ProgNr) command within selected range

  if (!no_print) {      // display in web client?
    ethClient.println("</p>");   // finish HTML paragraph
  }
  return pvalstr;
} // --- query() ---

/** *****************************************************************
    Function:  webPrintHeader()
    Does:      Sets up the HTML code to start a web page
    Pass parameters:
     none
    Parameters passed back:
     none
    Function value returned:
     none
    Global resources used:
     client object
 * *************************************************************** */
void webPrintHeader(void) {
  ethClient.println(F("HTTP/1.1 200 OK"));
  ethClient.println(F("Content-Type: text/html"));
  ethClient.println();
  ethClient.println(F("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">"));
  ethClient.println(F("<html>"));
  ethClient.println(F("<head>"));
  ethClient.println(F("<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">"));
  ethClient.println(F("<title>COMFO-LAN SERVER</title>"));
  ethClient.println(F("<link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"http://arduino.cc/en/favicon.png\" />"));
  ethClient.println(F("</head>"));
  ethClient.println(F("<body>"));
} // --- webPrintHeader() ---

/** *****************************************************************
    Function:  webPrintFooter()
    Does:      Sets up the closing HTML code of a web page.
    Pass parameters:
     none
    Parameters passed back:
     none
    Function value returned:
     none
    Global resources used:
     client object
 * *************************************************************** */
void webPrintFooter(void) {
  ethClient.println(F("</body>"));
  ethClient.println(F("</html>"));
  ethClient.println();

} // --- webPrintFooter() ---

/** *****************************************************************
    Function:  webPrintSite()
    Does:      Sets up HTML code to display a help page.
    Pass parameters:
     none
    Parameters passed back:
     none
    Function value returned:
     none
    Global resources used:
     client object
 * *************************************************************** */
void webPrintSite() {
  webPrintHeader();

  ethClient.println(F("<h1>Comfo-LAN Web</h1>"));
  ethClient.print(F(" <p>options:"));
  ethClient.print(F(" <table>"));
  ethClient.print(F(" <tr><td>/K</td> <td>list available categories</td></tr>"));
  ethClient.print(F(" <tr><td>/Kx</td> <td>query all values in category x</td></tr>"));
  ethClient.print(F(" <tr><td>/x</td> <td>query value for ID x</td></tr>"));
  ethClient.print(F(" <tr><td>/x-y</td> <td>query all values from ID x up to ID y</td></tr>"));
  ethClient.print(F(" <tr><td>/Sx=v</td> <td>set value v for ID x and query the new value afterwards</td></tr>"));
  ethClient.print(F(" <tr><td>/Vn</td> <td>set verbosity level for serial output</td></tr>"));
  //  ethClient.print(F(" <tr><td>/Mn</td> <td>activate/deactivate monitor functionality (n=0 disable, n=1 enable)</td></tr>"));
  ethClient.print(F(" <tr><td>/Gxx</td> <td>query GPIO pin xx</td></tr>"));
  ethClient.print(F(" <tr><td>/Gxx=y</td> <td>set GPIO pin xx to high(1) or low(0)</td></tr>"));
  ethClient.print(F(" </table>"));
  ethClient.print(F(" multiple queries are possible, e.g. /K0/710/8000-8999/T</p>"));
  webPrintFooter();
} // --- webPrintSite() ---

/** *****************************************************************
    Function:  SerialPrintHex()
    Does:      Sends the hex representation of one byte to the PC
               hardware serial interface. Adds a leading zero if
               it is a one-digit hex value.
    Pass parameters:
     byte      the value to convert and send
   Parameters passed back:
     none
   Function value returned:
     none
   Global resources used:
      Serial  instance
 * *************************************************************** */
void SerialPrintHex(byte val) {
  if (val < 16) Serial.print("0");  // add a leading zero to single-digit values
  Serial.print(val, HEX);
}

/*
    Functions to prepare the responses from the device
*/
void printProgStr(const char str[], String respArray[][3], int lineNumb)
{
  if (!str) return;
  char outBuffer[50];
  byte outBufferLen = 0;
  char c;
  while ((c = pgm_read_byte(str++)))
  {
    outBufferLen += sprintf(outBuffer + outBufferLen, "%c", c);
    if (verbose)
      Serial.print(String(c) + F(": "));
  }
  respArray[lineNumb][ParameterName] = String(outBuffer);
}

void printUInt(const char* title, int val, const char* unit, String respArray[][3], int lineNumb)
{
  printProgStr(title, respArray, lineNumb);
  char outBuffer[50];
  sprintf(outBuffer, "%u", val);
  respArray[lineNumb][ParameterValue] = String(outBuffer);
  sprintf(outBuffer, "%s", unit);
  respArray[lineNumb][ParameterUnit] = String(outBuffer);
  if (verbose)
    Serial.println(val);
}

void printInt(const char* title, int val, const char* unit, String respArray[][3], int lineNumb)
{
  printProgStr(title, respArray, lineNumb);
  char outBuffer[50];
  sprintf(outBuffer, "%d", val);
  respArray[lineNumb][ParameterValue] = String(outBuffer);
  sprintf(outBuffer, "%s", unit);
  respArray[lineNumb][ParameterUnit] = String(outBuffer);
  if (verbose)
    Serial.println(val);
}

void printDInt(const char* title, byte highByteVal, byte lowByteVal, const char* unit, String respArray[][3], int lineNumb)
{
  unsigned int val = (highByteVal << 8) + (lowByteVal);

  printProgStr(title, respArray, lineNumb);
  char outBuffer[50];
  sprintf(outBuffer, "%d", val);
  respArray[lineNumb][ParameterValue] = String(outBuffer);
  sprintf(outBuffer, "%s", unit);
  respArray[lineNumb][ParameterUnit] = String(outBuffer);
  if (verbose)
    Serial.println(val);
}

void printLInt(const char* title, byte highByteVal, byte midByteVal, byte lowByteVal, const char* unit, String respArray[][3], int lineNumb)
{
  unsigned long val = ((long)highByteVal << 16) + ((long)midByteVal << 8) + ((long)lowByteVal);

  printProgStr(title, respArray, lineNumb);
  char outBuffer[50];
  sprintf(outBuffer, "%ld", val);
  respArray[lineNumb][ParameterValue] = String(outBuffer);
  sprintf(outBuffer, "%s", unit);
  respArray[lineNumb][ParameterUnit] = String(outBuffer);
  if (verbose)
    Serial.println(val);
}

void printLInt(const char* title, uint32_t val, const char* unit, String respArray[][3], int lineNumb)
{
  printProgStr(title, respArray, lineNumb);
  char outBuffer[50];
  sprintf(outBuffer, "%ld", val);
  respArray[lineNumb][ParameterValue] = String(outBuffer);
  sprintf(outBuffer, "%s", unit);
  respArray[lineNumb][ParameterUnit] = String(outBuffer);
  if (verbose)
    Serial.println(val);
}

void printVersionString(const char* title, int major, int minor, int beta, const char* delimiter, String respArray[][3], int lineNumb)
{
  printProgStr(title, respArray, lineNumb);
  char outBuffer[50];
  sprintf(outBuffer, "%d%s%d%s%d", major, delimiter, minor, delimiter, beta);
  respArray[lineNumb][ParameterValue] = String(outBuffer);
  if (verbose)
    Serial.println(outBuffer);
}

void printString(const char* title, byte* recvBuff, int startIndex, int arrayLength, String respArray[][3], int lineNumb)
{
  printProgStr(title, respArray, lineNumb);
  char outBuffer[50];
  byte outBufferLen = 0;
  for (int i = 0; i < arrayLength; i++)
  {
    if (recvBuff[startIndex + i] == 0x00 || recvBuff[startIndex + i] == 0xFF)
      continue;

    outBufferLen += sprintf(outBuffer + outBufferLen, "%c", (char)recvBuff[startIndex + i]);

    if (verbose)
      Serial.print((char)recvBuff[startIndex + i]);
  }

  respArray[lineNumb][ParameterValue] = String(outBuffer);
  if (verbose)
    Serial.println(outBuffer);
}

void printRotation(const char* title, byte highByteVal, byte lowByteVal, const char* unit, String respArray[][3], int lineNumb)
{
  int val = int(highByteVal << 8) + int(lowByteVal);
  val = 1875000 / val;

  printProgStr(title, respArray, lineNumb);
  char outBuffer[50];
  sprintf(outBuffer, "%d", val);
  respArray[lineNumb][ParameterValue] = String(outBuffer);
  sprintf(outBuffer, "%s", unit);
  respArray[lineNumb][ParameterUnit] = String(outBuffer);
  if (verbose)
    Serial.println(val);
}

void printTemp(const char* title, float temp, const char* unit, String respArray[][3], int lineNumb)
{
  float val = temp;
  if (val != 0.0)
    val = (temp - 40) / 2;

  printProgStr(title, respArray, lineNumb);
  respArray[lineNumb][ParameterValue] = printFixpoint(val, 2);
  char outBuffer[50];
  sprintf(outBuffer, "%s", unit);
  respArray[lineNumb][ParameterUnit] = String(outBuffer);
  if (verbose)
    Serial.println(val);
}

void printScale(const char* title, float scaleVal, int lowScale, int highScale, int lowOut, int highOut, const char* unit, String respArray[][3], int lineNumb)
{
  float val = (scaleVal / (highScale - lowScale)) * (highOut - lowOut);

  printProgStr(title, respArray, lineNumb);
  respArray[lineNumb][ParameterValue] = printFixpoint(val, 2);
  char outBuffer[50];
  sprintf(outBuffer, "%s", unit);
  respArray[lineNumb][ParameterUnit] = String(outBuffer);
  if (verbose)
    Serial.println(val);
}

String printFixpoint(double dval, int precision)
{
  int a, b, i;
  char outBuffer[50];
  byte outBufferLen = 0;

  if (dval < 0) {
    outBufferLen += sprintf(outBuffer + outBufferLen, "-");
    dval *= -1.0;
  }

  double rval = 10.0;
  for (i = 0; i < precision; i++) rval *= 10.0;
  dval += 5.0 / rval;
  a = (int)(dval);
  rval /= 10.0;
  b = (int)(dval * rval - a * rval);
  if (precision == 0) {
    outBufferLen += sprintf(outBuffer + outBufferLen, "%d", a);
  } else {
    char formatstr[8] = "%d.%01d";
    formatstr[5] = '0' + precision;
    outBufferLen += sprintf(outBuffer + outBufferLen, formatstr, a, b);
  }
  return String(outBuffer);
}

String printEnum(const char* title, const char* enumstr_progm, uint16_t enumstr_len, uint16_t search_val, int print_val, String respArray[][3], int lineNumb)
{
  uint16_t val;
  char outBuffer[50];
  byte outBufferLen = 0;

  printProgStr(title, respArray, lineNumb);
  char enumstr[300];
  memcpy_P(enumstr, enumstr_progm, enumstr_len);
  enumstr[enumstr_len] = 0;

  if (enumstr != NULL) {
    uint16_t c = 0;
    while (c < enumstr_len) {
      if ((byte)enumstr[c + 1] != ' ') {
        val = uint16_t(((uint8_t*)enumstr)[c]) << 8 | uint16_t(((uint8_t*)enumstr)[c + 1]);
        c++;
      } else {
        val = uint16_t(((uint8_t*)enumstr)[c]);
      }
      // skip leading space
      c += 2;
      if (val == search_val) {
        // enum value found
        break;
      }
      while (enumstr[c] != 0) c++;
      c++;
    }
    if (c < enumstr_len) {
      if (print_val)
        outBufferLen += sprintf(outBuffer + outBufferLen, "%d - %s", val, &enumstr[c]);
      else
        outBufferLen += sprintf(outBuffer + outBufferLen, "%s", &enumstr[c]);

    } else {
      outBufferLen += sprintf(outBuffer + outBufferLen, "%d - not found", search_val);
    }

    if (verbose)
      Serial.println(val);

    respArray[lineNumb][ParameterValue] = String(outBuffer);
    return String(outBuffer);
  }
  return "";
}

void printDataError(const char* errorMessage, int providedData, const char* delimiter, char lowLimit, const char* limitSign, char highLimit, String respArray[][3], int lineNumb)
{
  printProgStr(errorMessage, respArray, lineNumb);
  char outBuffer[50];
  sprintf(outBuffer, "%d %s %c%s%c", providedData, delimiter, lowLimit, limitSign, highLimit);
  respArray[lineNumb][ParameterValue] = String(outBuffer);

  if (verbose)
    Serial.println(outBuffer);
}

// Print values from parameter table for web client
void printParameterTableWeb(String parameterTable[][3], unsigned int tableSize)
{
  for (unsigned int j = 0; j < tableSize; j++)
  {
    bool foundEntry = false;
    if (parameterTable[j][ParameterName] != NULL && parameterTable[j][ParameterName] != "")
    {
      foundEntry = true;
      if (j == 0)
        ethClient.println("<b>" + parameterTable[j][ParameterName] + "</b>");
      else ethClient.print(parameterTable[j][ParameterName] + ":");
    }
    if (parameterTable[j][ParameterValue] != NULL && parameterTable[j][ParameterValue] != "")
    {
      foundEntry = true;
      ethClient.print(" " + parameterTable[j][ParameterValue]);
    }
    if (parameterTable[j][ParameterUnit] != NULL && parameterTable[j][ParameterUnit] != "")
    {
      foundEntry = true;
      ethClient.print(" " + parameterTable[j][ParameterUnit]);
    }
    if (foundEntry)
      ethClient.println("</br>");
  }
}

// Calculate Checksum for telegram
int lueftCalcChecksum(byte telCommand, byte telCntData, byte telData[])
{
  int telCommandInt = (int)telCommand;
  int telCntDataInt = (int)telCntData;
  int telDataInt = 0;
  for (int i = 0; i < telCntDataInt; i++)
  {
    telDataInt += (int)telData[i];
  }

  return telCommandInt + telCntDataInt + telDataInt + 173;
}

// Call send/get function and evaluate response
bool lueftungGetCommand(byte commandId, String parameterTable[][3])
{
  bool answerReceived = true;
  // Don't send query for internal sensor data (0x98)
  if (commandId != 0x98)
    answerReceived = lueftSendGetCommand(commandId);

  if (answerReceived == true)
  {
    // Query inputs
    if (commandId == 0x03)
    {
      printProgStr(STR_03_0, parameterTable, 0);
      printEnum(STR_03_1, ENUM_Inaktiv_Aktiv, sizeof(ENUM_Inaktiv_Aktiv), (int)bitRead(receiveBuffer[5], 0), 1, parameterTable, 1);
      printEnum(STR_03_2, ENUM_Inaktiv_Aktiv, sizeof(ENUM_Inaktiv_Aktiv), (int)bitRead(receiveBuffer[5], 1), 1, parameterTable, 2);
      printEnum(STR_03_3, ENUM_Inaktiv_Aktiv, sizeof(ENUM_Inaktiv_Aktiv), (int)bitRead(receiveBuffer[6], 0), 1, parameterTable, 3);
      printEnum(STR_03_4, ENUM_Inaktiv_Aktiv, sizeof(ENUM_Inaktiv_Aktiv), (int)bitRead(receiveBuffer[6], 1), 1, parameterTable, 4);
      printEnum(STR_03_5, ENUM_Inaktiv_Aktiv, sizeof(ENUM_Inaktiv_Aktiv), (int)bitRead(receiveBuffer[6], 2), 1, parameterTable, 5);
      printEnum(STR_03_6, ENUM_Inaktiv_Aktiv, sizeof(ENUM_Inaktiv_Aktiv), (int)bitRead(receiveBuffer[6], 3), 1, parameterTable, 6);
      printEnum(STR_03_7, ENUM_Inaktiv_Aktiv, sizeof(ENUM_Inaktiv_Aktiv), (int)bitRead(receiveBuffer[6], 4), 1, parameterTable, 7);
      return true;
    }

    // Query ventilaton state
    if (commandId == 0x0B)
    {
      printProgStr(STR_0B_0, parameterTable, 0);
      printInt(STR_0B_1, (int)receiveBuffer[5], STR_PERCENT, parameterTable, 1);
      printInt(STR_0B_2, (int)receiveBuffer[6], STR_PERCENT, parameterTable, 2);
      printRotation(STR_0B_3, receiveBuffer[7], receiveBuffer[8], STR_RPM, parameterTable, 3);
      printRotation(STR_0B_4, receiveBuffer[9], receiveBuffer[10], STR_RPM, parameterTable, 4);
      return true;
    }

    // Query flap state
    if (commandId == 0x0D)
    {
      printProgStr(STR_0D_0, parameterTable, 0);
      printInt(STR_0D_1, (int)receiveBuffer[5], STR_PERCENT, parameterTable, 1);
      printEnum(STR_0D_2, ENUM_Geschlossen_Offen, sizeof(ENUM_Geschlossen_Offen), (int)receiveBuffer[6], 1, parameterTable, 2);
      printInt(STR_0D_3, (int)receiveBuffer[7], "", parameterTable, 3);
      printInt(STR_0D_4, (int)receiveBuffer[8], "", parameterTable, 4);
      return true;
    }

    // Query temperature state
    if (commandId == 0x0F)
    {
      printProgStr(STR_0F_0, parameterTable, 0);
      printTemp(STR_0F_1, (float)receiveBuffer[5], STR_DEGREE, parameterTable, 1);
      printTemp(STR_0F_2, (float)receiveBuffer[6], STR_DEGREE, parameterTable, 2);
      printTemp(STR_0F_3, (float)receiveBuffer[7], STR_DEGREE, parameterTable, 3);
      printTemp(STR_0F_4, (float)receiveBuffer[8], STR_DEGREE, parameterTable, 4);
      return true;
    }

    // Query key state
    if (commandId == 0x11)
    {
      printProgStr(STR_11_0, parameterTable, 0);
      printEnum(STR_11_1, ENUM_11_1, sizeof(ENUM_11_1), (int)receiveBuffer[5], 1, parameterTable, 1);
      return true;
    }

    // Query analog inputs
    if (commandId == 0x13)
    {
      printProgStr(STR_13_0, parameterTable, 0);
      printScale(STR_13_1, (float)receiveBuffer[5], 0, 255, 0, 10, STR_VOLT, parameterTable, 1);
      printScale(STR_13_2, (float)receiveBuffer[6], 0, 255, 0, 10, STR_VOLT, parameterTable, 2);
      printScale(STR_13_3, (float)receiveBuffer[7], 0, 255, 0, 10, STR_VOLT, parameterTable, 3);
      printScale(STR_13_4, (float)receiveBuffer[8], 0, 255, 0, 10, STR_VOLT, parameterTable, 4);
      return true;
    }

    // Query sensor data
    if (commandId == 0x97)
    {
      printProgStr(STR_97_0, parameterTable, 0);
      printTemp(STR_97_1, (float)receiveBuffer[5], STR_DEGREE, parameterTable, 1);
      printInt(STR_97_2, (int)receiveBuffer[6], STR_PERCENT, parameterTable, 2);
      printInt(STR_97_3, (int)receiveBuffer[7], STR_PERCENT, parameterTable, 3);
      printInt(STR_97_4, (int)receiveBuffer[8], STR_PERCENT, parameterTable, 4);
      printInt(STR_97_5, (int)receiveBuffer[9], STR_PERCENT, parameterTable, 5);
      printScale(STR_97_6, (float)receiveBuffer[10], 0, 240, 0, 2880, STR_MINUTE, parameterTable, 6);
      // 0x00
      printInt(STR_97_8, (int)receiveBuffer[12], STR_PERCENT, parameterTable, 7);
      printInt(STR_97_9, (int)receiveBuffer[13], STR_PERCENT, parameterTable, 8);
      printInt(STR_97_10, (int)receiveBuffer[14], STR_PERCENT, parameterTable, 9);
      printInt(STR_97_11, (int)receiveBuffer[15], STR_PERCENT, parameterTable, 10);
      printInt(STR_97_12, (int)receiveBuffer[16], STR_PERCENT, parameterTable, 11);
      printInt(STR_97_13, (int)receiveBuffer[17], STR_PERCENT, parameterTable, 12);
      printInt(STR_97_14, (int)receiveBuffer[18], STR_PERCENT, parameterTable, 13);
      printInt(STR_97_15, (int)receiveBuffer[19], STR_PERCENT, parameterTable, 14);
      printInt(STR_97_16, (int)receiveBuffer[20], STR_PERCENT, parameterTable, 15);
      printInt(STR_97_17, (int)receiveBuffer[21], STR_PERCENT, parameterTable, 16);
      return true;
    }

    // Query analog values
    if (commandId == 0x9D)
    {
      printProgStr(STR_9D_0, parameterTable, 0);
      printEnum(STR_9D_1, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[5], 0), 1, parameterTable, 1);
      printEnum(STR_9D_2, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[5], 1), 1, parameterTable, 2);
      printEnum(STR_9D_3, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[5], 2), 1, parameterTable, 3);
      printEnum(STR_9D_4, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[5], 3), 1, parameterTable, 4);
      printEnum(STR_9D_5, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[5], 4), 1, parameterTable, 5);
      printEnum(STR_9D_6, ENUM_Steuern_Regeln, sizeof(ENUM_Steuern_Regeln), (int)bitRead(receiveBuffer[6], 0), 1, parameterTable, 6);
      printEnum(STR_9D_7, ENUM_Steuern_Regeln, sizeof(ENUM_Steuern_Regeln), (int)bitRead(receiveBuffer[6], 1), 1, parameterTable, 7);
      printEnum(STR_9D_8, ENUM_Steuern_Regeln, sizeof(ENUM_Steuern_Regeln), (int)bitRead(receiveBuffer[6], 2), 1, parameterTable, 8);
      printEnum(STR_9D_9, ENUM_Steuern_Regeln, sizeof(ENUM_Steuern_Regeln), (int)bitRead(receiveBuffer[6], 3), 1, parameterTable, 9);
      printEnum(STR_9D_10, ENUM_Steuern_Regeln, sizeof(ENUM_Steuern_Regeln), (int)bitRead(receiveBuffer[6], 4), 1, parameterTable, 10);
      printEnum(STR_9D_11, ENUM_Positiv_Negativ, sizeof(ENUM_Positiv_Negativ), (int)bitRead(receiveBuffer[7], 0), 1, parameterTable, 11);
      printEnum(STR_9D_12, ENUM_Positiv_Negativ, sizeof(ENUM_Positiv_Negativ), (int)bitRead(receiveBuffer[7], 1), 1, parameterTable, 12);
      printEnum(STR_9D_13, ENUM_Positiv_Negativ, sizeof(ENUM_Positiv_Negativ), (int)bitRead(receiveBuffer[7], 2), 1, parameterTable, 13);
      printEnum(STR_9D_14, ENUM_Positiv_Negativ, sizeof(ENUM_Positiv_Negativ), (int)bitRead(receiveBuffer[7], 3), 1, parameterTable, 14);
      printEnum(STR_9D_15, ENUM_Positiv_Negativ, sizeof(ENUM_Positiv_Negativ), (int)bitRead(receiveBuffer[7], 4), 1, parameterTable, 15);
      printInt(STR_9D_16, (int)receiveBuffer[8], STR_PERCENT, parameterTable, 16);
      printInt(STR_9D_17, (int)receiveBuffer[9], STR_PERCENT, parameterTable, 17);
      printInt(STR_9D_18, (int)receiveBuffer[10], STR_PERCENT, parameterTable, 18);
      printInt(STR_9D_19, (int)receiveBuffer[11], STR_PERCENT, parameterTable, 19);
      printInt(STR_9D_20, (int)receiveBuffer[12], STR_PERCENT, parameterTable, 20);
      printInt(STR_9D_21, (int)receiveBuffer[13], STR_PERCENT, parameterTable, 21);
      printInt(STR_9D_22, (int)receiveBuffer[14], STR_PERCENT, parameterTable, 22);
      printInt(STR_9D_23, (int)receiveBuffer[15], STR_PERCENT, parameterTable, 23);
      printInt(STR_9D_24, (int)receiveBuffer[16], STR_PERCENT, parameterTable, 24);
      printInt(STR_9D_25, (int)receiveBuffer[17], STR_PERCENT, parameterTable, 25);
      printInt(STR_9D_26, (int)receiveBuffer[18], STR_PERCENT, parameterTable, 26);
      printInt(STR_9D_27, (int)receiveBuffer[19], STR_PERCENT, parameterTable, 27);
      printInt(STR_9D_28, (int)receiveBuffer[20], STR_PERCENT, parameterTable, 28);
      printInt(STR_9D_29, (int)receiveBuffer[21], STR_PERCENT, parameterTable, 29);
      printInt(STR_9D_30, (int)receiveBuffer[22], STR_PERCENT, parameterTable, 30);
      printEnum(STR_9D_31, ENUM_9D_19, sizeof(ENUM_9D_19), (int)receiveBuffer[23], 1, parameterTable, 31);
      return true;
    }

    // Query (time) dealys
    if (commandId == 0xC9)
    {
      printProgStr(STR_C9_0, parameterTable, 0);
      printInt(STR_C9_1, (int)receiveBuffer[5], STR_MINUTE, parameterTable, 1);
      printInt(STR_C9_2, (int)receiveBuffer[6], STR_MINUTE, parameterTable, 2);
      printInt(STR_C9_3, (int)receiveBuffer[7], STR_MINUTE, parameterTable, 3);
      printInt(STR_C9_4, (int)receiveBuffer[8], STR_MINUTE, parameterTable, 4);
      printInt(STR_C9_5, (int)receiveBuffer[9], STR_WEEKS, parameterTable, 5);
      printInt(STR_C9_6, (int)receiveBuffer[10], STR_MINUTE, parameterTable, 6);
      printInt(STR_C9_7, (int)receiveBuffer[11], STR_MINUTE, parameterTable, 7);
      printInt(STR_C9_8, (int)receiveBuffer[12], STR_MINUTE, parameterTable, 8);
      return true;
    }

    // Query ventilation level
    if (commandId == 0xCD)
    {
      printProgStr(STR_CD_0, parameterTable, 0);
      printInt(STR_CD_1, (int)receiveBuffer[5], STR_PERCENT, parameterTable, 1);
      printInt(STR_CD_2, (int)receiveBuffer[6], STR_PERCENT, parameterTable, 2);
      printInt(STR_CD_3, (int)receiveBuffer[7], STR_PERCENT, parameterTable, 3);
      printInt(STR_CD_4, (int)receiveBuffer[8], STR_PERCENT, parameterTable, 4);
      printInt(STR_CD_5, (int)receiveBuffer[9], STR_PERCENT, parameterTable, 5);
      printInt(STR_CD_6, (int)receiveBuffer[10], STR_PERCENT, parameterTable, 6);
      printInt(STR_CD_7, (int)receiveBuffer[11], STR_PERCENT, parameterTable, 7);
      printInt(STR_CD_8, (int)receiveBuffer[12], STR_PERCENT, parameterTable, 8);
      printEnum(STR_CD_9, ENUM_CD_9, sizeof(ENUM_CD_9), (int)receiveBuffer[13], 1, parameterTable, 9);
      printEnum(STR_CD_10, ENUM_Inaktiv_Aktiv, sizeof(ENUM_Inaktiv_Aktiv), (int)receiveBuffer[14], 1, parameterTable, 10);
      printInt(STR_CD_11, (int)receiveBuffer[15], STR_PERCENT, parameterTable, 11);
      printInt(STR_CD_12, (int)receiveBuffer[16], STR_PERCENT, parameterTable, 12);

      ventilationLevel = (int)receiveBuffer[13];
      return true;
    }

    // Query temperatures
    if (commandId == 0xD1)
    {
      printProgStr(STR_D1_0, parameterTable, 0);
      printTemp(STR_D1_1, (float)receiveBuffer[5], STR_DEGREE, parameterTable, 1);
      printTemp(STR_D1_2, (float)receiveBuffer[6], STR_DEGREE, parameterTable, 2);
      printTemp(STR_D1_3, (float)receiveBuffer[7], STR_DEGREE, parameterTable, 3);
      printTemp(STR_D1_4, (float)receiveBuffer[8], STR_DEGREE, parameterTable, 4);
      printTemp(STR_D1_5, (float)receiveBuffer[9], STR_DEGREE, parameterTable, 5);
      printEnum(STR_D1_6, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[10], 0), 1, parameterTable, 6);
      printEnum(STR_D1_7, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[10], 1), 1, parameterTable, 7);
      printEnum(STR_D1_8, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[10], 2), 1, parameterTable, 8);
      printEnum(STR_D1_9, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[10], 3), 1, parameterTable, 9);
      printEnum(STR_D1_10, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[10], 4), 1, parameterTable, 10);
      printEnum(STR_D1_11, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[10], 5), 1, parameterTable, 11);
      printEnum(STR_D1_12, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[10], 6), 1, parameterTable, 12);
      printTemp(STR_D1_13, (float)receiveBuffer[11], STR_DEGREE, parameterTable, 13);
      printTemp(STR_D1_14, (float)receiveBuffer[12], STR_DEGREE, parameterTable, 14);
      printTemp(STR_D1_15, (float)receiveBuffer[13], STR_DEGREE, parameterTable, 15);
      return true;
    }

    // Query states
    if (commandId == 0xD5)
    {
      printProgStr(STR_D5_0, parameterTable, 0);
      printEnum(STR_D5_1, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)receiveBuffer[5], 1, parameterTable, 1);
      printEnum(STR_D5_2, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)receiveBuffer[6], 1, parameterTable, 2);
      printEnum(STR_D5_3, ENUM_Links_Rechts, sizeof(ENUM_Links_Rechts), (int)receiveBuffer[7], 1, parameterTable, 3);
      printEnum(STR_D5_4, ENUM_Gross_Klein, sizeof(ENUM_Gross_Klein), (int)receiveBuffer[8], 1, parameterTable, 4);
      printEnum(STR_D5_5, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[9], 0), 1, parameterTable, 5);
      printEnum(STR_D5_6, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[9], 1), 1, parameterTable, 6);
      printEnum(STR_D5_7, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[9], 2), 1, parameterTable, 7);
      printEnum(STR_D5_8, ENUM_Abwesend_Anwesend, sizeof(ENUM_Abwesend_Anwesend), (int)bitRead(receiveBuffer[9], 3), 1, parameterTable, 8);
      // 0x00
      printEnum(STR_D5_9, ENUM_D5_7, sizeof(ENUM_D5_7), (int)receiveBuffer[11], 1, parameterTable, 9);
      printEnum(STR_D5_10, ENUM_D5_8, sizeof(ENUM_D5_8), (int)receiveBuffer[12], 1, parameterTable, 10);
      printEnum(STR_D5_11, ENUM_D5_9, sizeof(ENUM_D5_9), (int)receiveBuffer[13], 1, parameterTable, 11);
      printEnum(STR_D5_12, ENUM_D5_10, sizeof(ENUM_D5_10), (int)receiveBuffer[14], 1, parameterTable, 12);
      printEnum(STR_D5_13, ENUM_D5_11, sizeof(ENUM_D5_11), (int)receiveBuffer[15], 1, parameterTable, 13);
      return true;
    }

    // Query malfunctions
    if (commandId == 0xD9)
    {
      printProgStr(STR_D9_0, parameterTable, 0);
      printEnum(STR_D9_1, ENUM_D9_1, sizeof(ENUM_D9_1), (int)receiveBuffer[5], 1, parameterTable, 1);
      printEnum(STR_D9_2, ENUM_D9_2, sizeof(ENUM_D9_2), (int)receiveBuffer[6], 1, parameterTable, 2);
      printEnum(STR_D9_3, ENUM_D9_1, sizeof(ENUM_D9_1), (int)receiveBuffer[7], 1, parameterTable, 3);
      printEnum(STR_D9_4, ENUM_D9_2, sizeof(ENUM_D9_2), (int)receiveBuffer[8], 1, parameterTable, 4);
      printEnum(STR_D9_5, ENUM_D9_1, sizeof(ENUM_D9_1), (int)receiveBuffer[9], 1, parameterTable, 5);
      printEnum(STR_D9_6, ENUM_D9_2, sizeof(ENUM_D9_2), (int)receiveBuffer[10], 1, parameterTable, 6);
      printEnum(STR_D9_7, ENUM_D9_1, sizeof(ENUM_D9_1), (int)receiveBuffer[11], 1, parameterTable, 7);
      printEnum(STR_D9_8, ENUM_D9_2, sizeof(ENUM_D9_2), (int)receiveBuffer[12], 1, parameterTable, 8);
      printEnum(STR_D9_9, ENUM_D9_9, sizeof(ENUM_D9_9), (int)receiveBuffer[13], 1, parameterTable, 9);
      printEnum(STR_D9_10, ENUM_D9_10, sizeof(ENUM_D9_10), (int)receiveBuffer[14], 1, parameterTable, 10);
      printEnum(STR_D9_11, ENUM_D9_10, sizeof(ENUM_D9_10), (int)receiveBuffer[15], 1, parameterTable, 11);
      printEnum(STR_D9_12, ENUM_D9_10, sizeof(ENUM_D9_10), (int)receiveBuffer[16], 1, parameterTable, 12);
      printEnum(STR_D9_13, ENUM_D9_10, sizeof(ENUM_D9_10), (int)receiveBuffer[17], 1, parameterTable, 13);
      printEnum(STR_D9_14, ENUM_D9_14, sizeof(ENUM_D9_14), (int)receiveBuffer[18], 1, parameterTable, 14);
      printEnum(STR_D9_15, ENUM_D9_14, sizeof(ENUM_D9_14), (int)receiveBuffer[19], 1, parameterTable, 15);
      printEnum(STR_D9_16, ENUM_D9_14, sizeof(ENUM_D9_14), (int)receiveBuffer[20], 1, parameterTable, 16);
      printEnum(STR_D9_17, ENUM_D9_14, sizeof(ENUM_D9_14), (int)receiveBuffer[21], 1, parameterTable, 17);
      return true;
    }

    // Query operating hours
    if (commandId == 0xDD)
    {
      printProgStr(STR_DD_0, parameterTable, 0);
      printLInt(STR_DD_1, receiveBuffer[5], receiveBuffer[6], receiveBuffer[7], STR_HOUR, parameterTable, 1);
      printLInt(STR_DD_2, receiveBuffer[8], receiveBuffer[9], receiveBuffer[10], STR_HOUR, parameterTable, 2);
      printLInt(STR_DD_3, receiveBuffer[11], receiveBuffer[12], receiveBuffer[13], STR_HOUR, parameterTable, 3);
      printDInt(STR_DD_4, receiveBuffer[14], receiveBuffer[15], STR_HOUR, parameterTable, 4);
      printDInt(STR_DD_5, receiveBuffer[16], receiveBuffer[17], STR_HOUR, parameterTable, 5);
      printDInt(STR_DD_6, receiveBuffer[18], receiveBuffer[19], STR_HOUR, parameterTable, 6);
      printDInt(STR_DD_7, receiveBuffer[20], receiveBuffer[21], STR_HOUR, parameterTable, 7);
      printLInt(STR_DD_8, receiveBuffer[22], receiveBuffer[23], receiveBuffer[24], STR_HOUR, parameterTable, 8);
      return true;
    }

    // Query bypass control state
    if (commandId == 0xDF)
    {
      printProgStr(STR_DF_0, parameterTable, 0);
      printInt(STR_DF_1, (int)receiveBuffer[7], STR_BLANK, parameterTable, 1);
      printInt(STR_DF_2, (int)receiveBuffer[8], STR_BLANK, parameterTable, 2);
      printInt(STR_DF_3, (int)receiveBuffer[9], STR_BLANK, parameterTable, 3);
      printEnum(STR_DF_4, ENUM_Nein_Ja, sizeof(ENUM_Nein_Ja), (int)receiveBuffer[11], 1, parameterTable, 4);
      return true;
    }

    // Query preheating state
    if (commandId == 0xE1)
    {
      printProgStr(STR_E1_0, parameterTable, 0);
      printEnum(STR_E1_1, ENUM_Geschlossen_Offen, sizeof(ENUM_Geschlossen_Offen), (int)receiveBuffer[5], 1, parameterTable, 1);
      printEnum(STR_E1_2, ENUM_Inaktiv_Aktiv, sizeof(ENUM_Inaktiv_Aktiv), (int)receiveBuffer[6], 1, parameterTable, 2);
      printEnum(STR_E1_3, ENUM_Inaktiv_Aktiv, sizeof(ENUM_Inaktiv_Aktiv), (int)receiveBuffer[7], 1, parameterTable, 3);
      printDInt(STR_E1_4, receiveBuffer[8], receiveBuffer[9], STR_MINUTE, parameterTable, 4);
      printEnum(STR_E1_5, ENUM_E1_6, sizeof(ENUM_E1_6), (int)receiveBuffer[10], 1, parameterTable, 5);
      return true;
    }

    // Query RF state
    if (commandId == 0xE5)
    {
      printProgStr(STR_E5_0, parameterTable, 0);
      printInt(STR_E5_1, (int)receiveBuffer[5], STR_BLANK, parameterTable, 1);
      printInt(STR_E5_2, (int)receiveBuffer[6], STR_BLANK, parameterTable, 2);
      printInt(STR_E5_3, (int)receiveBuffer[7], STR_BLANK, parameterTable, 3);
      printInt(STR_E5_4, (int)receiveBuffer[8], STR_BLANK, parameterTable, 4);
      printInt(STR_E5_5, (int)receiveBuffer[9], STR_BLANK, parameterTable, 5);
      printInt(STR_E5_6, (int)receiveBuffer[10], STR_BLANK, parameterTable, 6);
      printInt(STR_E5_7, (int)receiveBuffer[11], STR_BLANK, parameterTable, 7);
      return true;
    }

    // Query last 8 times preheating
    if (commandId == 0xE9)
    {
      printProgStr(STR_E9_0, parameterTable, 0);
      printInt(STR_E9_1, (int)receiveBuffer[5], STR_DEGREE, parameterTable, 1);
      printInt(STR_E9_2, (int)receiveBuffer[6], STR_DEGREE, parameterTable, 2);
      printInt(STR_E9_3, (int)receiveBuffer[7], STR_DEGREE, parameterTable, 3);
      printInt(STR_E9_4, (int)receiveBuffer[8], STR_DEGREE, parameterTable, 4);
      printInt(STR_E9_5, (int)receiveBuffer[9], STR_DEGREE, parameterTable, 5);
      printInt(STR_E9_6, (int)receiveBuffer[10], STR_DEGREE, parameterTable, 6);
      printInt(STR_E9_7, (int)receiveBuffer[11], STR_DEGREE, parameterTable, 7);
      printInt(STR_E9_8, (int)receiveBuffer[12], STR_DEGREE, parameterTable, 8);
      return true;
    }

    // Query earth heat exchanger / reheating
    if (commandId == 0xEB)
    {
      printProgStr(STR_EB_0, parameterTable, 0);
      printTemp(STR_EB_1, (float)receiveBuffer[5], STR_DEGREE, parameterTable, 1);
      printTemp(STR_EB_2, (float)receiveBuffer[6], STR_DEGREE, parameterTable, 2);
      printInt(STR_EB_3, (int)receiveBuffer[7], STR_PERCENT, parameterTable, 3);
      printInt(STR_EB_4, (int)receiveBuffer[8], STR_PERCENT, parameterTable, 4);
      printInt(STR_EB_5, (int)receiveBuffer[9], STR_BLANK, parameterTable, 5);
      printInt(STR_EB_6, (int)receiveBuffer[10], STR_BLANK, parameterTable, 6);
      printTemp(STR_EB_7, (float)receiveBuffer[11], STR_DEGREE, parameterTable, 7);
      return true;
    }

    // Query bootloader version
    if (commandId == 0x67)
    {
      printProgStr(STR_67_0, parameterTable, 0);
      printVersionString(STR_67_1, (int)receiveBuffer[5], (int)receiveBuffer[6], (int)receiveBuffer[7], STR_DOT, parameterTable, 1);
      printString(STR_67_2, receiveBuffer, 8, 10, parameterTable, 2);
      return true;
    }

    // Query firmware version
    if (commandId == 0x69)
    {
      printProgStr(STR_69_0, parameterTable, 0);
      printVersionString(STR_69_1, (int)receiveBuffer[5], (int)receiveBuffer[6], (int)receiveBuffer[7], STR_DOT, parameterTable, 1);
      printString(STR_69_2, receiveBuffer, 8, 10, parameterTable, 2);
      return true;
    }

    // Query connector board version
    if (commandId == 0xA1)
    {
      printProgStr(STR_A1_0, parameterTable, 0);
      printVersionString(STR_A1_1, (int)receiveBuffer[5], (int)receiveBuffer[6], 0, STR_DOT, parameterTable, 1);
      printString(STR_A1_2, receiveBuffer, 7, 10, parameterTable, 2);
      printInt(STR_A1_3, (int)receiveBuffer[17], "", parameterTable, 3);
      printInt(STR_A1_4, (int)receiveBuffer[18], "", parameterTable, 4);
      return true;
    }

    // Query air quality sensor
    if (commandId == 0x98)
    {
      uint16_t airQualityPredict = 0;
      uint8_t airQualityStatus = 0;
      uint32_t airQualityResistance = 0;
      uint16_t airQualityTvoc = 0;
      airQualitySensorRequest(airQualityPredict, airQualityStatus, airQualityResistance, airQualityTvoc);
      printProgStr(STR_98_0, parameterTable, 0);
      printEnum(STR_98_1, ENUM_airQualitySensor, sizeof(ENUM_airQualitySensor), airQualityStatus, 1, parameterTable, 1);
      printUInt(STR_98_2, airQualityPredict, "", parameterTable, 2);
      printUInt(STR_98_3, airQualityTvoc, "", parameterTable, 3);
      printLInt(STR_98_4, airQualityResistance, "", parameterTable, 4);
      return true;
    }
  }
  return false;
}

// Prepare get command, send to device and evaluate response
bool lueftSendGetCommand(byte sendCommandId)
{
  byte sendAck[2] = { 0x07, 0xF3 };

  lueftGetCommand[3] = sendCommandId;
  lueftGetCommand[5] = (int)sendCommandId + 173;

  if (verbose)
    Serial.print(F("Sending command: "));

  for (byte i = 0; i < sizeof(lueftGetCommand); i++)
  {
    Serial1.write(lueftGetCommand[i]);
    if (verbose)
    {
      Serial.print(" ");
      SerialPrintHex(lueftGetCommand[i]);
    }
  }
  if (verbose)
    Serial.println("");

  memset(receiveBuffer, 0x00, sizeof(receiveBuffer));

  delay(500);
  if (Serial1.available() > 0)
  {
    if (verbose)
    {
      Serial.print(F("Serial available: "));
      Serial.println(Serial1.available());
    }

    bool inTelegram = false;
    delay(500);
    int i = 0;
    do {
      byte currentByte = Serial1.read();
      byte nextByte = Serial1.peek();

      if (currentByte == 0x07 && nextByte == 0xF0)
      {
        inTelegram = true;
      }
      else if (currentByte == 0x07 && nextByte == 0x0F)
      {
        inTelegram = false;
      }
      else if (currentByte == 0x07 && nextByte == 0x07)
      {
        continue;
      }

      if (inTelegram == true)
      {
        receiveBuffer[i] = currentByte;
        if (verbose)
        {
          Serial.print(" ");
          SerialPrintHex(receiveBuffer[i]);
        }
        i++;
      }
    } while (Serial1.available() > 0);

    Serial1.write(sendAck, 2);
  }

  if (receiveBuffer[3] == lueftGetCommand[3] + 1)
  {
    if (verbose)
      Serial.println(F("Answer received"));
    return true;
  }
  else if (receiveBuffer[3] != 0x00)
  {
    Serial.print(F("Answer command does not fit to requested data - awaited: "));
    Serial.print(lueftGetCommand[3] + 1, HEX);
    Serial.print(F(" - received: "));
    Serial.print(receiveBuffer[3], HEX);
  }
  else Serial.println(F("Receive buffer empty."));

  return false;
}

// Call send function and evaluate response
void lueftungSetCommand(byte commandId, byte commandData[], int commandDataLength)
{
  bool commandFound = false;
  String parameterTable[1][3];

  // Set level
  if (commandId == 0x99)
  {
    printProgStr(STR_99_0, parameterTable, 0);
    // Check data
    if (commandDataLength != 1)
    {
      printDataError(STR_DATALENGTHERROR, commandDataLength, STR_INSTEADOF, '1', " ", ' ', parameterTable, 0);
      ethClient.println(parameterTable[0][ParameterName] + F(" ") + parameterTable[0][ParameterValue]);
      ethClient.println(outBuf);
      return;
    }
    else if (/*(int)commandData[0] < 0 ||*/ (int)commandData[0] > 4)
    {
      printDataError(STR_DATAERROR, (int)commandData[0], STR_INSTEADOF, '0', STR_MINUS, '4', parameterTable, 0);
      ethClient.println(parameterTable[0][ParameterName] + F(" ") + parameterTable[0][ParameterValue]);
      ethClient.println(outBuf);
      return;
    }

    printEnum(STR_VALUE, ENUM_CD_9, sizeof(ENUM_CD_9), (int)commandData[0], 1, parameterTable, 0);
    ethClient.println(parameterTable[0][ParameterName] + F(": ") + parameterTable[0][ParameterValue]);
    outBufLen += sprintf(outBuf + outBufLen, "</p>");
    commandFound = true;
  }

  if (commandFound == false)
  {
    Serial.println("No command was found");
    return;
  }

  bool answerReceived = lueftSendSetCommand(commandId, commandData, commandDataLength);

  if (answerReceived == true)
  {
    String parameterTable[ParameterTableSize][3];
    // Set level
    if (commandId == 0x99)
    {
      lueftungGetCommand(0xCD, parameterTable);
      ethClient.println("</br>Response:</br>");
      printParameterTableWeb(parameterTable, sizeof(parameterTable) / (3 * sizeof(String)));
    }
  }
}

// Prepare set command, send to device and evaluate response
bool lueftSendSetCommand(byte sendCommandId, byte commandData[], int commandDataLength)
{
  lueftSetCommand[0] = 0x07;
  lueftSetCommand[1] = 0xF0;
  lueftSetCommand[3] = sendCommandId;
  lueftSetCommand[4] = commandDataLength;
  for (int i = 0; i < commandDataLength; i++)
  {
    lueftSetCommand[5 + i] = commandData[i];
  }
  lueftSetCommand[5 + commandDataLength] = lueftCalcChecksum(sendCommandId, commandDataLength, commandData);
  lueftSetCommand[6 + commandDataLength] = 0x07;
  lueftSetCommand[7 + commandDataLength] = 0x0F;

  if (verbose)
    Serial.print(F("Sending command: "));

  for (int i = 0; i < 8 + commandDataLength; i++)
  {
    Serial1.write(lueftSetCommand[i]);
    if (verbose)
    {
      Serial.print(" ");
      SerialPrintHex(lueftSetCommand[i]);
    }
  }

  memset(receiveBuffer, 0x00, sizeof(receiveBuffer));

  delay(500);
  if (Serial1.available() > 0)
  {
    if (verbose)
    {
      Serial.print(F("Serial available: "));
      Serial.println(Serial1.available());
    }

    delay(500);
    int i = 0;
    do {
      receiveBuffer[i] = Serial1.read();
      if (verbose)
      {
        Serial.print(" ");
        SerialPrintHex(receiveBuffer[i]);
      }
      i++;
    } while (Serial1.available() > 0);

    if ( receiveBuffer[0] == 0x07 && receiveBuffer[1] == 0xF3)
    {
      if (verbose)
        Serial.println(F("ACK was received."));
      return true;
    }
    else Serial.println(F("No ACK received."));
  }
  return false;
}

// Query air quality sensor and get current values
void airQualitySensorRequest(uint16_t &sensorPredict, uint8_t &sensorStatus, uint32_t &sensorResistance, uint16_t &sensorTvoc)
{
  byte airQualityPredict1;
  byte airQualityPredict2;

  Wire.requestFrom(iaqaddress, 9);
  airQualityPredict1 = Wire.read();
  airQualityPredict2 = Wire.read();
  sensorPredict = (airQualityPredict1 << 8 | airQualityPredict2);

  sensorStatus = Wire.read();

  sensorResistance = Wire.read();
  sensorResistance = (sensorResistance << 8) | Wire.read();
  sensorResistance = (sensorResistance << 8) | Wire.read();
  sensorResistance = (sensorResistance << 8) | Wire.read();

  sensorTvoc = (Wire.read() << 8 | Wire.read());
}

// Print results from air quality sensor
void printAirQualitySensor()
{
  outBufclear();

  uint16_t airQualityPredict = 0;
  uint8_t airQualityStatus = 0;
  uint32_t airQualityResistance = 0;
  uint16_t airQualityTvoc = 0;
  airQualitySensorRequest(airQualityPredict, airQualityStatus, airQualityResistance, airQualityTvoc);

  String parameterTable[5][3];

  lueftungGetCommand(0x98, parameterTable);
  printParameterTableWeb(parameterTable, sizeof(parameterTable) / (3 * sizeof(String)));

  ethClient.print("</br>&nbsp;</br>");
  ethClient.print("airQualityLevelSlow: ");
  ethClient.print(airQualityLevelSlow);
  ethClient.print("</br> airQualityLevelStop: ");
  ethClient.print(airQualityLevelStop);
}

// Check air regularly and change ventilation level as needed
void airCheck()
{
  byte command = 0x99;
  byte cmdData[1] = { 0x03 };
  airLoopCount++;

  uint16_t airQualityPredict = 0;
  uint8_t airQualityStatus = 0;
  uint32_t airQualityResistance = 0;
  uint16_t airQualityTvoc = 0;
  airQualitySensorRequest(airQualityPredict, airQualityStatus, airQualityResistance, airQualityTvoc);

  if (verbose)
  {
    Serial.print("Air loop count: ");
    Serial.println(airLoopCount);
    Serial.print("airQualityPredict: ");
    Serial.println(airQualityPredict);
    Serial.print("Ventilation Level: ");
    Serial.println(ventilationLevel);
    Serial.print("AirQualityCheck Loops: ");
    Serial.println(airQualityLoops);
    Serial.print("AirQualityLevel Slow: ");
    Serial.println(airQualityLevelSlow);
    Serial.print("AirQualityLevel Stop: ");
    Serial.println(airQualityLevelStop);
  }

  // If air quality is worse than first warning level, set ventilation to level 2
  // If ventilation level is 1 (stopped) for quite a while (airQualityLoops * 4), set ventilation level to 2 to provide new air for measurement
  if (airQualityPredict > airQualityLevelSlow && ((ventilationLevel >= 3 && airLoopCount > airQualityLoops)
      || (ventilationLevel == 1 && airLoopCount > airQualityLoops * 4)))
  {
    if (badAir == false)
    {
      savedVentilationLevel = (byte)ventilationLevel;
    }
    airLoopCount = 0;
    cmdData[0] = 0x02;
    lueftungSetCommand(command, cmdData, sizeof(cmdData));
    badAir = true;
  }

  if (airQualityPredict > airQualityLevelStop && ventilationLevel >= 2  && airLoopCount > airQualityLoops)
  {
    if (badAir == false)
    {
      savedVentilationLevel = (byte)ventilationLevel;
    }
    airLoopCount = 0;
    cmdData[0] = 0x01;
    lueftungSetCommand(command, cmdData, sizeof(cmdData));
    badAir = true;
  }

  if (airQualityPredict < airQualityLevelSlow && badAir == true && airLoopCount > airQualityLoops)
  {
    airLoopCount = 0;
    cmdData[0] = savedVentilationLevel;
    lueftungSetCommand(command, cmdData, sizeof(cmdData));
    badAir = false;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.print(F("Start"));

  Serial1.begin(9600);
  Wire.begin();

  // Start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();

#ifdef MQTTBrokerIP

#ifdef MQTTUsername
  const char MQTTUser[] = MQTTUsername;
#else
  const char* MQTTUser = NULL;
#endif
#ifdef MQTTPassword
  const char MQTTPass[] = MQTTPassword;
#else
  const char* MQTTPass = NULL;
#endif

  if (!MQTTClient.connected()) {
    MQTTClient.setServer(MQTTBroker, 1883);
    int retries = 0;
    while (!MQTTClient.connected() && retries < 3) {
      MQTTClient.connect("ComfoD-LAN", MQTTUser, MQTTPass);
      retries++;
      if (!MQTTClient.connected()) {
        delay(1000);
        Serial.println(F("Failed to connect to MQTT broker, retrying..."));
      }
      else {
        Serial.println(F("MQTTClient connected"));
        retries = 0;
        char topic[30] = "fhem/";
        sprintf(topic, "fhem/%s/#", MQTTTopicPrefix);
        while (!MQTTClient.subscribe(topic) && retries < 3) {
          retries++;
          Serial.println(F("Failed to subscribe to MQTT topic ("));
          Serial.print(topic);
          Serial.print(F("), retrying..."));
        }
        if (retries < 3)
        {
          Serial.println(F("Subscribed to MQTT topic: "));
          Serial.print(topic);
        }

        MQTTClient.setCallback(subscribeReceive);
      }
    }
  }
#endif
  Serial.print(F("Setup finished"));
}

void loop() {
  char c;
  const byte MaxArrayElement = 252;
  char  cLineBuffer[MaxArrayElement];
  byte  bPlaceInBuffer;                // index into buffer

  MQTTClient.loop();

  if (millis() - lastAirQualityTime >= (airQualityInterval * 1000))
  {
    airCheck();
    lastAirQualityTime = millis();
  }

  // Check if ENC28J60 is still available
  if (millis() - lastEthernetCheckTime >= (5000))
  {
    // Enc28J60 is Enc28J60Network class that defined in Enc28J60Network.h
    // ENC28J60 ignore all incoming packets if ECON1.RXEN is not set
    if  (! (Enc28J60.readReg((uint8_t) NET_ENC28J60_ECON1) & NET_ENC28J60_ECON1_RXEN)) {
      Serial.println ("ENC28J60 reinit");
      Enc28J60.init(mac);
    }
    lastEthernetCheckTime = millis();
  }

  // Listen for incoming clients
  ethClient = server.available();
  if (ethClient) {
    // Read characters from client and assemble them in cLineBuffer
    bPlaceInBuffer = 0;          // index into cLineBuffer
    while (ethClient.connected()) {
      if (ethClient.available()) {
        c = ethClient.read();       // read one character
        Serial.print(c);            // and send it to hardware UART

        if ((c != '\n') && (c != '\r') && (bPlaceInBuffer < MaxArrayElement)) {
          cLineBuffer[bPlaceInBuffer++] = c;
          continue;
        }
        // Got an EOL character
        Serial.println();

        // Flush any remaining bytes from the client buffer
        ethClient.flush();
        // GET / HTTP/1.1 (anforderung website)
        // GET /710 HTTP/1.0 (befehlseingabe)
        String urlString = String(cLineBuffer);
        urlString = urlString.substring(urlString.indexOf('/'), urlString.indexOf(' ', urlString.indexOf('/')));
        Serial.println(urlString);
        urlString.toCharArray(cLineBuffer, MaxArrayElement);

        // Air quality sensor
        if (urlString == "/airQualitySensor") {
          webPrintHeader();
          printAirQualitySensor();
          webPrintFooter();
          break;
        }

        // Set up a pointer to cLineBuffer
        char *p = cLineBuffer;
#ifdef PASSKEY
        // Check for valid passkey
        p = strchr(cLineBuffer + 1, '/');
        if (p == NULL) { // no match
          break;
        }
        *p = '\0';   // mark end of string
        if (strcmp(cLineBuffer + 1, PASSKEY)) {
          Serial.println(F("no matching passkey"));
          Serial.println(cLineBuffer + 1);
          Serial.println(PASSKEY);
          webPrintHeader();
          webPrintFooter();
          break;
        }
        *p = '/';
#endif
        // Simply print the website
        if (!strcmp(p, "/")) {
          webPrintSite();
          break;
        }

        // Answer to unknown requests
        if (!isdigit(p[1]) && strchr("KVSG", p[1]) == NULL) {
          webPrintHeader();
          webPrintFooter();
          break;
        }
        // Set verbosity level
        if (p[1] == 'V') {
          p += 2;
          verbose = atoi(p);
          webPrintHeader();
          if (verbose > 0) {
            ethClient.println(F("verbose mode activated<br>"));
          } else {
            ethClient.println(F("verbose mode deactivated<br>"));
          }
          ethClient.println(F("only serial output is affected"));
          webPrintFooter();
          break;
        }

        // Send a SET command or an information message
        // char * cLineBuffer has the following structure:
        // p[1]          'S'
        // p[3]          ProgNr (digits, any (!) length)
        // p[3+length]   '='
        // p[3+length+1] Value, any (!) length
        // There is only the buffer size which limits to the
        // permissible lengths, no sanity checks
        if ( p[1] == 'S')     // SET information message
        {
          webPrintHeader();
          outBufclear();
          byte command;

          p += 2;             // third position in cLineBuffer
          if (!isdigit(*p)) { // now we check for digits - nice
            webPrintHeader();
            ethClient.println(F("ERROR: invalid parameter line"));
            webPrintFooter();
            break;
          }
          command = atoi(p);     // convert until non-digit char is found

          p = strchr(p, '='); // search for '=' sign
          if (p == NULL) {    // no match
            webPrintHeader();
            ethClient.println(F("ERROR: invalid parameter val"));
            webPrintFooter();
            break;
          }
          p++;                   // position pointer past the '=' sign
          Serial.print("set ProgNr ");
          Serial.print(command, HEX);    // the ProgNr
          Serial.print(" = ");
          Serial.println(p);     // the value

          byte tmpCmdData[20];
          int i = 0;

          while (true) {
            tmpCmdData[i] = atoi(p);
            Serial.println(tmpCmdData[i]);
            p = strchr(p, ';'); // search for ';' sign
            i++;
            if (p == NULL)
              break;
            p++;
          } ;

          byte cmdData[i];
          memcpy(cmdData, tmpCmdData, i);

          ethClient.println("<p>");
          outBufLen += sprintf(outBuf + outBufLen, "SET Cmd: %02X - ", command);
          ethClient.println(outBuf);
          outBufclear();

          // Now send it out to the bus
          lueftungSetCommand(command, cmdData, sizeof(cmdData));

          webPrintFooter();
          break;
        }
        // List categories
        if (p[1] == 'K' && !isdigit(p[2])) {
          // list categories
          webPrintHeader();

          for (int cat = 0; cat < CAT_UNKNOWN; cat++) {
            outBufclear();
            String category[10][3];
            printEnum(STR_CAT, ENUM_CAT, sizeof(ENUM_CAT), cat, 1, category, 0);

            for (unsigned int j = 0; j < sizeof(category) / (3 * sizeof(String)); j++)
            {
              bool foundEntry = false;
              if (category[j][ParameterName] != NULL && category[j][ParameterName] != "")
              {
                foundEntry = true;
                if (j == 0 && cat == 0)
                  ethClient.println("<b>" + category[j][ParameterName] + "</b></br>");
              }
              if (category[j][ParameterValue] != NULL && category[j][ParameterValue] != "")
              {
                foundEntry = true;
                ethClient.print("<a href='");
#ifdef PASSKEY
                ethClient.print("/" + String(PASSKEY));
#endif
                ethClient.print("/K" + String(cat) + "'>");
                ethClient.print(category[j][ParameterValue]);
                ethClient.print("</a>");
              }
              if (foundEntry)
                ethClient.println("</br>");
            }
          }
          webPrintFooter();
          break;
        }
        // Print queries
        webPrintHeader();
        char* range;
        char* line_start;
        char* line_end;
        int start = -1;
        int end = -1;
        range = strtok(p, "/");
        while (range != 0) {
          if (range[0] == 'G') { // handle gpio command
            uint8_t val;
            uint8_t pin;
            bool error = false;
            p = range + 1;
            if (!isdigit(*p)) { // now we check for digits
              ethClient.println(F("ERROR: invalid parameter line"));
              break;
            }
            pin = (uint8_t)atoi(p);     // convert until non-digit char is found
            int anz_ex_gpio = sizeof(exclude_GPIO) / sizeof(int);
            for (int i = 0; i < anz_ex_gpio; i++) {
              if (pin == exclude_GPIO[i]) {
                error = true;
              }
            }
            if (error == true) {
              ethClient.println(F("ERROR: protected GPIO pin"));
              break;
            }
            p = strchr(p, '='); // search for '=' sign
            if (p == NULL) {    // no match -> query value
              val = digitalRead(pin);
            } else { // set value
              p++;
              if (!strncasecmp(p, "on", 2) || !strncasecmp(p, "high", 2) || *p == '1') {
                val = HIGH;
              } else {
                val = LOW;
              }
              digitalWrite(pin, val);
              pinMode(pin, OUTPUT); // TODO: does this case a problem if already set as output?
            }
            ethClient.print(F("GPIO"));
            ethClient.print(pin);
            ethClient.print(F(": "));
            ethClient.print(val != LOW ? F("1") : F("0"));
          }
          else {
            if (range[0] == 'K') {
              uint8_t cat, search_cat;
              uint16_t line;
              int i;
              uint32_t c;
              i = 0;
              start = -1;
              end = -1;
              search_cat = atoi(&range[1]);
              c = pgm_read_dword(&cmdtbl[i].cmd);

              while (c != CMD_END) {
                cat = pgm_read_byte(&cmdtbl[i].category);
                if (cat == search_cat) {
                  if (start < 0) {
                    line = pgm_read_word(&cmdtbl[i].line);
                    start = line;
                  }
                } else {
                  if (start >= 0) {
                    line = pgm_read_word(&cmdtbl[i - 1].line);
                    end = line;
                    break;
                  }
                }
                i++;
                c = pgm_read_dword(&cmdtbl[i].cmd);
              }

              if (end < start) {
                end = start;
              }
            } else {
              // split range
              line_start = range;
              line_end = strchr(range, '-');
              if (line_end == NULL) {
                line_end = line_start;
              } else {
                *line_end = '\0';
                line_end++;
              }
              start = atoi(line_start);
              end = atoi(line_end);
            }

            if (start >= 0 || end >= 0)
              query(start, end, 0);
            else {
              Serial.println("Query failed");
              ethClient.println("Query failed");
            }
          }

          range = strtok(NULL, "/");
        } // endwhile
        webPrintFooter();
        break;
      } // endif, ethClient available
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    ethClient.stop();
  } // endif, ethClient


#ifdef MQTTBrokerIP

#ifdef MQTTUsername
  const char MQTTUser[] = MQTTUsername;
#else
  const char* MQTTUser = NULL;
#endif
#ifdef MQTTPassword
  const char MQTTPass[] = MQTTPassword;
#else
  const char* MQTTPass = NULL;
#endif

  String MQTTTopic = "";

  if (mqttFirst || (millis() - lastMQTTTime >= ((mqttInterval * 1000)) && (numMqttValues > 0)))
  {
    mqttFirst = false;
    lastMQTTTime = millis();

    if (!MQTTClient.connected()) {
      MQTTClient.setServer(MQTTBroker, 1883);
      int retries = 0;
      while (!MQTTClient.connected() && retries < 3) {
        MQTTClient.connect("ComfoD-LAN", MQTTUser, MQTTPass);
        retries++;
        if (!MQTTClient.connected()) {
          delay(1000);
          Serial.println(F("Failed to connect to MQTT broker, retrying..."));
        }
      }
    }

    for (int i = 0; i < numMqttValues; i++) {
      // Declare local variables and start building json if enabled
#ifdef MQTT_JSON
      MQTTPayload = "";
      // Build the json heading
      MQTTPayload.concat(F("{\""));
#ifdef MQTTDeviceID
      MQTTPayload.concat(MQTTDeviceID);
#else
      MQTTPayload.concat(F("ComfoD-LAN"));
#endif
      MQTTPayload.concat(F("\":{"));
#endif

      if (mqttParameters[i] > 0) {
        if (MQTTClient.connected()) {


#ifdef MQTTTopicPrefix
          MQTTTopic = MQTTTopicPrefix;
          MQTTTopic.concat(F("/"));
#else
          MQTTTopic = "ComfoD-LAN/";
#endif

          // use the sub-topic "json" if json output is enabled
#ifdef MQTT_JSON
          MQTTTopic.concat(F("Query"));
          MQTTTopic.concat(String(mqttParameters[i]));
          MQTTTopic.concat(F("/json"));
#endif
          String parameterTableMqtt[ParameterTableSize][3];

          if (mqttParameters[i] != CMD_UNKNOWN) { // send only valid command codes
            int retry = QUERY_RETRIES;

            while (retry) {
              if ( lueftungGetCommand(mqttParameters[i], parameterTableMqtt)) {
                break;   // success, break out of while loop
              } else {
                Serial.println("query failed");
                retry--;          // decrement number of attempts
              }
            } // endwhile, maximum number of retries reached

            String mqttTopicSaved;

            for (int j = 0; j < ParameterTableSize; j++)
            {
#ifndef MQTT_JSON
              MQTTPayload = "";
#endif
              if (parameterTableMqtt[j][ParameterName] != NULL && parameterTableMqtt[j][ParameterName] != "")
              {
                if (j == 0)
                {
#ifdef MQTT_JSON
                  MQTTPayload.concat("\"" + parameterTableMqtt[j][ParameterName] + "\":{");
#else
                  MQTTTopic.concat(parameterTableMqtt[j][ParameterName] + F("/"));
                  mqttTopicSaved = MQTTTopic;
#endif
                }
#ifdef MQTT_JSON
                else if (j == 1)
                  MQTTPayload.concat("\"" + parameterTableMqtt[j][ParameterName] + "\":");
                else MQTTPayload.concat(",\"" + parameterTableMqtt[j][ParameterName] + "\":");
#else
                MQTTTopic = mqttTopicSaved + String(parameterTableMqtt[j][ParameterName]);
#endif
              }
              if (parameterTableMqtt[j][ParameterValue] != NULL && parameterTableMqtt[j][ParameterValue] != "")
              {
#ifdef MQTT_JSON
                MQTTPayload.concat("\"" + parameterTableMqtt[j][ParameterValue]);
#else
                MQTTPayload = String(parameterTableMqtt[j][ParameterValue]);
#endif
              }
#ifdef MQTT_UNIT
              if (parameterTableMqtt[j][ParameterUnit] != NULL && parameterTableMqtt[j][ParameterUnit] != "")
                MQTTPayload.concat(" " + parameterTableMqtt[j][ParameterUnit]);
#endif
#ifdef MQTT_JSON
              if (parameterTableMqtt[j][ParameterValue] != NULL && parameterTableMqtt[j][ParameterValue] != "")
                MQTTPayload.concat("\"");
#else
              // Publish MQTT topic if not in JSON-format
              if (MQTTPayload.length() > 0)
              {
                MQTTTopic.replace(" ", "_");
                Serial.print(F("Output topic: "));
                Serial.println(MQTTTopic.c_str());
                Serial.print(F("Payload Output: "));
                Serial.println(MQTTPayload.c_str());
                MQTTClient.publish(MQTTTopic.c_str(), MQTTPayload.c_str());
              }
#endif
            }
            MQTTPayload.concat(F("}"));
          }
        }
      }
      // End of mqtt if loop so close off the json and publish
#ifdef MQTT_JSON
      // Close the json doc off
      MQTTPayload.concat(F("}}"));
#ifdef MQTTStatus
      MQTTPayload.concat(F("}"));
#endif

      Serial.print(F("Output topic: "));
      Serial.println(MQTTTopic.c_str());
      Serial.print(F("Payload Output: "));
      Serial.println(MQTTPayload.c_str());
      // Now publish the json payload only once
      MQTTClient.publish(MQTTTopic.c_str(), MQTTPayload.c_str());
#endif
    }
    Serial.println("Finished");
  }
#endif
}

#ifdef MQTTBrokerIP

// Recieve MQTT messages from the subscribed topic and evaluate
void subscribeReceive(char* topic, byte* payload, unsigned int length)
{
  if (String(topic) == "fhem/Comfo-LAN/Lueftung/Stufe")
  {
    int level = atoi(payload);
    Serial.print("Command received with payload: ");
    Serial.print(topic);
    Serial.println("Set to level: ");
    Serial.print(level);
    if (level > 0 && level < 5)
    {
      byte command = 0x99;
      byte cmdData[1] = { 0x03 };
      cmdData[0] = byte(level);
      lueftungSetCommand(command, cmdData, sizeof(cmdData));
    }
    else
    {
      Serial.println("Wrong payload for topic: ");
      Serial.println(topic);
    }
  }
  else {
    Serial.println("Topic received - no command found");
    // Print the topic
    Serial.print("Topic: ");
    Serial.println(topic);

    // Print the message
    Serial.print("Message: ");
    for (unsigned int i = 0; i < length; i ++)
    {
      Serial.print(char(payload[i]));
    }
    Serial.println();
    Serial.print("Length: ");
    Serial.println(length);
  }

  // Print a newline
  Serial.println("");
}
#endif
