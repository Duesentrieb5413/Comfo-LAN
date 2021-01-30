/****************************************************/
/* DEFINITIONS and TYPEDEFS                         */
/****************************************************/

/* special command ids */
#define CMD_UNKNOWN 0x00000000u
#define CMD_END     0xffffffffu

const int ResponseTableSize = 32;
const int ResponseTableChars = 70;

/****************************************************/
/* TABLES and STRINGS                       */
/****************************************************/

/* Menue Strings */

const char STR_PERCENT[]  = "%";
const char STR_DEGREE[]   = "°C";
const char STR_RPM[]      = "U pro min";
const char STR_VOLT[]     = "V";
const char STR_MINUTE[]   = "min";
const char STR_HOUR[]     = "h";
const char STR_WEEKS[]    = "Wochen";
const char STR_DOT[]      = ".";
const char STR_MINUS[]    = "-";
const char STR_BLANK[]    = " ";

const char STR_INSTEADOF[] = "anstatt";
const char STR_VALUE[]     PROGMEM = "Wert";
const char STR_DATALENGTHERROR[] PROGMEM = "Daten haben die falsche Laenge";
const char STR_DATAERROR[] PROGMEM = "Daten sind nicht im erlaubten Bereich";

const char STR_03_0[]  PROGMEM = "Eingaenge";
const char STR_03_1[]  PROGMEM = "Stufenschalter L1";
const char STR_03_2[]  PROGMEM = "Stufenschalter L2";
const char STR_03_3[]  PROGMEM = "Schalteingang Badezimmer";
const char STR_03_4[]  PROGMEM = "Schalteingang Kuechenhaube";
const char STR_03_5[]  PROGMEM = "Schalteingang Externer Filter";
const char STR_03_6[]  PROGMEM = "Schalteingang Waermerueckgewinnung (WTW)";
const char STR_03_7[]  PROGMEM = "Schalteingang Badezimmerschalter 2 (luxe)";

const char STR_0B_0[]  PROGMEM = "Ventilatorstatus";
const char STR_0B_1[]  PROGMEM = "Zuluft";
const char STR_0B_2[]  PROGMEM = "Abluft";
const char STR_0B_3[]  PROGMEM = "Drehzahl Zuluft";
const char STR_0B_4[]  PROGMEM = "Drehzahl Abluft";

const char STR_0D_0[]  PROGMEM = "Klappenstatus";
const char STR_0D_1[]  PROGMEM = "Bypass";
const char STR_0D_2[]  PROGMEM = "Vorheizung";
const char STR_0D_3[]  PROGMEM = "Bypass Motorstrom";
const char STR_0D_4[]  PROGMEM = "Vorheizung Motorstrom";

const char STR_0F_0[]  PROGMEM = "Temperaturstatus";
const char STR_0F_1[]  PROGMEM = "T1 - Aussenluft";
const char STR_0F_2[]  PROGMEM = "T2 - Zuluft";
const char STR_0F_3[]  PROGMEM = "T3 - Abluft";
const char STR_0F_4[]  PROGMEM = "T4 - Fortluft";

const char STR_11_0[]  PROGMEM = "Tastenstatus";
const char STR_11_1[]  PROGMEM = "Tastenstatus";

const char STR_13_0[]  PROGMEM = "Analoge Eingaenge";
const char STR_13_1[]  PROGMEM = "Analog 1";
const char STR_13_2[]  PROGMEM = "Analog 2";
const char STR_13_3[]  PROGMEM = "Analog 3";
const char STR_13_4[]  PROGMEM = "Analog 4";

const char STR_97_0[]  PROGMEM = "Sensordaten";
const char STR_97_1[]  PROGMEM = "Enthalpie Sensor Temperatur";
const char STR_97_2[]  PROGMEM = "Enthalpie Sensor Feuchtigkeit";
const char STR_97_3[]  PROGMEM = "Analog 1";
const char STR_97_4[]  PROGMEM = "Analog 2";
const char STR_97_5[]  PROGMEM = "Enthalpie Koeffizient";
const char STR_97_6[]  PROGMEM = "Enthalpie Timer";
// 0x00
const char STR_97_8[]  PROGMEM = "Analog 1 zu gewuenscht";
const char STR_97_9[]  PROGMEM = "Analog 1 ab gewuenscht";
const char STR_97_10[] PROGMEM = "Analog 2 zu gewuenscht";
const char STR_97_11[] PROGMEM = "Analog 2 ab gewuenscht";
const char STR_97_12[] PROGMEM = "Analog 3";
const char STR_97_13[] PROGMEM = "Analog 4";
const char STR_97_14[] PROGMEM = "Analog 3 zu gewuenscht";
const char STR_97_15[] PROGMEM = "Analog 3 ab gewuenscht";
const char STR_97_16[] PROGMEM = "Analog 4 zu gewuenscht";
const char STR_97_17[] PROGMEM = "Analog 4 ab gewuenscht";

const char STR_9D_0[]  PROGMEM = "Analogwerte";
const char STR_9D_1[]  PROGMEM = "Analog anwesend - Analog 1";
const char STR_9D_2[]  PROGMEM = "Analog anwesend - Analog 2";
const char STR_9D_3[]  PROGMEM = "Analog anwesend - Analog 3";
const char STR_9D_4[]  PROGMEM = "Analog anwesend - Analog 4";
const char STR_9D_5[]  PROGMEM = "Analog anwesend - RF";
const char STR_9D_6[]  PROGMEM = "Analog regeln-steuern - Analog 1";
const char STR_9D_7[]  PROGMEM = "Analog regeln-steuern - Analog 2";
const char STR_9D_8[]  PROGMEM = "Analog regeln-steuern - Analog 3";
const char STR_9D_9[]  PROGMEM = "Analog regeln-steuern - Analog 4";
const char STR_9D_10[] PROGMEM = "Analog regeln-steuern - RF";
const char STR_9D_11[] PROGMEM = "Analog positiv-negativ - Analog 1";
const char STR_9D_12[] PROGMEM = "Analog positiv-negativ - Analog 2";
const char STR_9D_13[] PROGMEM = "Analog positiv-negativ - Analog 3";
const char STR_9D_14[] PROGMEM = "Analog positiv-negativ - Analog 4";
const char STR_9D_15[] PROGMEM = "Analog positiv-negativ - RF";
const char STR_9D_16[] PROGMEM = "Analog 1 Min. Einstellung";
const char STR_9D_17[] PROGMEM = "Analog 1 Max. Einstellung";
const char STR_9D_18[] PROGMEM = "Analog 1 Sollwert";
const char STR_9D_19[] PROGMEM = "Analog 2 Min. Einstellung";
const char STR_9D_20[] PROGMEM = "Analog 2 Max. Einstellung";
const char STR_9D_21[] PROGMEM = "Analog 2 Sollwert";
const char STR_9D_22[] PROGMEM = "Analog 3 Min. Einstellung";
const char STR_9D_23[] PROGMEM = "Analog 3 Max. Einstellung";
const char STR_9D_24[] PROGMEM = "Analog 3 Sollwert";
const char STR_9D_25[] PROGMEM = "Analog 4 Min. Einstellung";
const char STR_9D_26[] PROGMEM = "Analog 4 Max. Einstellung";
const char STR_9D_27[] PROGMEM = "Analog 4 Sollwert";
const char STR_9D_28[] PROGMEM = "Analog RF Min. Einstellung";
const char STR_9D_29[] PROGMEM = "Analog RF Max. Einstellung";
const char STR_9D_30[] PROGMEM = "Analog RF Sollwert";
const char STR_9D_31[] PROGMEM = "Prioritaet Regelung";

const char STR_C9_0[]  PROGMEM = "Zeitverzoegerung";
const char STR_C9_1[]  PROGMEM = "Badezimmerschalter Einschaltverzoegerung";
const char STR_C9_2[]  PROGMEM = "Badezimmerschalter Ausschaltverzoegerung";
const char STR_C9_3[]  PROGMEM = "L1 Ausschaltverzoegerung";
const char STR_C9_4[]  PROGMEM = "Stosslueftung";
const char STR_C9_5[]  PROGMEM = "Filter Zaehler";
const char STR_C9_6[]  PROGMEM = "RF hoch Zeit kurz";
const char STR_C9_7[]  PROGMEM = "RF hoch Zeit lang";
const char STR_C9_8[]  PROGMEM = "Kuechenhaube Ausschaltverzoegerung";

const char STR_CD_0[]  PROGMEM = "Ventilationsstufen";
const char STR_CD_1[]  PROGMEM = "Abluft abwesend - Stufe 0";
const char STR_CD_2[]  PROGMEM = "Abluft niedrig - Stufe 1";
const char STR_CD_3[]  PROGMEM = "Abluft mittel - Stufe 2";
const char STR_CD_4[]  PROGMEM = "Zuluft abwesend - Stufe 0";
const char STR_CD_5[]  PROGMEM = "Zuluft niedrig - Stufe 1";
const char STR_CD_6[]  PROGMEM = "Zuluft mittel - Stufe 2";
const char STR_CD_7[]  PROGMEM = "Abluft aktuell";
const char STR_CD_8[]  PROGMEM = "Zuluft aktuell";
const char STR_CD_9[]  PROGMEM = "Stufe aktuell";
const char STR_CD_10[] PROGMEM = "Zuluft Ventilator aktiv";
const char STR_CD_11[] PROGMEM = "Abluft hoch - Stufe 3";
const char STR_CD_12[] PROGMEM = "Zuluft hoch - Stufe 3";

const char STR_D1_0[]  PROGMEM = "Temperaturen";
const char STR_D1_1[]  PROGMEM = "Komfort Temperatur";
const char STR_D1_2[]  PROGMEM = "T1 - Aussenluft";
const char STR_D1_3[]  PROGMEM = "T2 - Zuluft";
const char STR_D1_4[]  PROGMEM = "T3 - Abluft";
const char STR_D1_5[]  PROGMEM = "T4 - Fortluft";
const char STR_D1_6[]  PROGMEM = "Fuehler anwesend - T1 - Aussenluft";
const char STR_D1_7[]  PROGMEM = "Fuehler anwesend - T2 - Zuluft";
const char STR_D1_8[]  PROGMEM = "Fuehler anwesend - T3 - Abluft";
const char STR_D1_9[]  PROGMEM = "Fuehler anwesend - T4 - Fortluft";
const char STR_D1_10[] PROGMEM = "Fuehler anwesend - EWT";
const char STR_D1_11[] PROGMEM = "Fuehler anwesend - Nachheizung";
const char STR_D1_12[] PROGMEM = "Fuehler anwesend - Kuechenhaube";
const char STR_D1_13[] PROGMEM = "Temperatur EWT";
const char STR_D1_14[] PROGMEM = "Temperatur Nachheizung";
const char STR_D1_15[] PROGMEM = "Temperatur Kuechenhaube";

const char STR_D5_0[]  PROGMEM = "Status";
const char STR_D5_1[]  PROGMEM = "Vorheizung anwesend";
const char STR_D5_2[]  PROGMEM = "Bypass anwesend";
const char STR_D5_3[]  PROGMEM = "Typ";
const char STR_D5_4[]  PROGMEM = "Groesse";
const char STR_D5_5[]  PROGMEM = "Optionen - Feuerstaette";
const char STR_D5_6[]  PROGMEM = "Optionen - Kuechenhaube";
const char STR_D5_7[]  PROGMEM = "Optionen - Nachheizung";
const char STR_D5_8[]  PROGMEM = "Optionen - Nachheizung PWM Modus";
// 0x00
const char STR_D5_9[]  PROGMEM = "Aktiv Status 1";
const char STR_D5_10[] PROGMEM = "Aktiv Status 2";
const char STR_D5_11[] PROGMEM = "Aktiv Status 3";
const char STR_D5_12[] PROGMEM = "Enthalpie anwesend";
const char STR_D5_13[] PROGMEM = "EWT anwesend";

const char STR_D9_0[]  PROGMEM = "Stoerungen";
const char STR_D9_1[]  PROGMEM = "Aktueller Fehler A";
const char STR_D9_2[]  PROGMEM = "Aktueller Fehler E";
const char STR_D9_3[]  PROGMEM = "Letzter Fehler A";
const char STR_D9_4[]  PROGMEM = "Letzter Fehler E";
const char STR_D9_5[]  PROGMEM = "Vorletzter Fehler A";
const char STR_D9_6[]  PROGMEM = "Vorletzter Fehler E";
const char STR_D9_7[]  PROGMEM = "Vorvorletzter Fehler A";
const char STR_D9_8[]  PROGMEM = "Vorvorletzter Fehler E";
const char STR_D9_9[]  PROGMEM = "Filterstatus";
const char STR_D9_10[] PROGMEM = "Aktueller Fehler EA";
const char STR_D9_11[] PROGMEM = "Letzter Fehler EA";
const char STR_D9_12[] PROGMEM = "Vorletzter Fehler EA";
const char STR_D9_13[] PROGMEM = "Vorvorletzter Fehler EA";
const char STR_D9_14[] PROGMEM = "Aktueller Fehler A (high)";
const char STR_D9_15[] PROGMEM = "Letzter Fehler A (high)";
const char STR_D9_16[] PROGMEM = "Vorletzter Fehler A (high)";
const char STR_D9_17[] PROGMEM = "Vorvorletzter Fehler A (high)";

const char STR_DD_0[]  PROGMEM = "Betriebsstunden";
const char STR_DD_1[]  PROGMEM = "Stufe 0";
const char STR_DD_2[]  PROGMEM = "Stufe 1";
const char STR_DD_3[]  PROGMEM = "Stufe 2";
const char STR_DD_4[]  PROGMEM = "Frostschutz";
const char STR_DD_5[]  PROGMEM = "Vorheizung";
const char STR_DD_6[]  PROGMEM = "Bypass offen";
const char STR_DD_7[]  PROGMEM = "Filter";
const char STR_DD_8[]  PROGMEM = "Stufe 3";

const char STR_DF_0[]  PROGMEM = "Bypassregelung";
// 0x00
// 0x00
const char STR_DF_1[]  PROGMEM = "Faktor";
const char STR_DF_2[]  PROGMEM = "Stufe";
const char STR_DF_3[]  PROGMEM = "Korrektur";
// 0x00
const char STR_DF_4[]  PROGMEM = "Sommermodus";

const char STR_E1_0[]  PROGMEM = "Status Vorheizung";
const char STR_E1_1[]  PROGMEM = "Status Klappe";
const char STR_E1_2[]  PROGMEM = "Frostschutz";
const char STR_E1_3[]  PROGMEM = "Vorheizung";
const char STR_E1_4[]  PROGMEM = "Frostminuten";
const char STR_E1_5[]  PROGMEM = "Frostsicherheit";

const char STR_E5_0[]  PROGMEM = "RF Status";
const char STR_E5_1[]  PROGMEM = "RF Adresse 4";
const char STR_E5_2[]  PROGMEM = "RF Adresse 3";
const char STR_E5_3[]  PROGMEM = "RF Adresse 2";
const char STR_E5_4[]  PROGMEM = "RF Adresse 1";
const char STR_E5_5[]  PROGMEM = "RF ID";
const char STR_E5_6[]  PROGMEM = "Modul anwesend";
const char STR_E5_7[]  PROGMEM = "Selbstlernender Modus aktiv";

const char STR_E9_0[]  PROGMEM = "Letzte 8 mal Vorheizung";
const char STR_E9_1[]  PROGMEM = "Aeltester Wert";
const char STR_E9_2[]  PROGMEM = "Siebtletzter Wert";
const char STR_E9_3[]  PROGMEM = "Sechtsletzter Wert";
const char STR_E9_4[]  PROGMEM = "Fuenftletzter Wert";
const char STR_E9_5[]  PROGMEM = "Viertletzter Wert";
const char STR_E9_6[]  PROGMEM = "Vorvorletzter Wert";
const char STR_E9_7[]  PROGMEM = "Vorletzter Wert";
const char STR_E9_8[]  PROGMEM = "Letzter Wert";

const char STR_EB_0[]  PROGMEM = "EWT - Nachheizung";
const char STR_EB_1[]  PROGMEM = "EWT niedrig";
const char STR_EB_2[]  PROGMEM = "EWT hoch";
const char STR_EB_3[]  PROGMEM = "EWT speed up";
const char STR_EB_4[]  PROGMEM = "Kuechenhaube speed up";
const char STR_EB_5[]  PROGMEM = "Nachheizung Leistung";
const char STR_EB_6[]  PROGMEM = "Nachheizung Leistung I-Parameter";
const char STR_EB_7[]  PROGMEM = "Nachheizung T gewuenscht";

const char STR_67_0[]  PROGMEM = "Bootloader Version";
const char STR_67_1[]  PROGMEM = "Version";
const char STR_67_2[]  PROGMEM = "Geraetename";

const char STR_69_0[]  PROGMEM = "Firmware Version";
#define STR_69_1 STR_67_1
#define STR_69_2 STR_67_2

const char STR_A1_0[]  PROGMEM = "Konnektorplatine Version";
#define STR_A1_1 STR_67_1
#define STR_A1_2 STR_67_2
const char STR_A1_3[]  PROGMEM = "Version CC-Ease";
const char STR_A1_4[]  PROGMEM = "Version CC-Luxe";

const char STR_EF_0[]  PROGMEM = "Air Quality Sensor";
const char STR_EF_1[]  PROGMEM = "Sensor Status";
const char STR_EF_2[]  PROGMEM = "CO2";
const char STR_EF_3[]  PROGMEM = "TVoCs";
const char STR_EF_4[]  PROGMEM = "Widerstand";

const char STR_99_0[]  PROGMEM = "Stufe setzen";
const char STR_9B_0[]  PROGMEM = "RS232 Modus setzen";

/* ENUM tables */
const char ENUM_Inaktiv_Aktiv[] PROGMEM = {
  "\x00 Inaktiv\0"
  "\x01 Aktiv"
};

const char ENUM_Abwesend_Anwesend[] PROGMEM = {
  "\x00 Abwesend\0"
  "\x01 Anwesend"
};

const char ENUM_Nein_Ja[] PROGMEM = {
  "\x00 Nein\0"
  "\x01 Ja"
};

const char ENUM_Steuern_Regeln[] PROGMEM = {
  "\x00 Steuern\0"
  "\x01 Regeln"
};

const char ENUM_Positiv_Negativ[] PROGMEM = {
  "\x00 Positiv\0"
  "\x01 Negativ"
};

const char ENUM_Links_Rechts[] PROGMEM = {
  "\x01 Links\0"
  "\x02 Rechts"
};

const char ENUM_Gross_Klein[] PROGMEM = {
  "\x01 Gross\0"
  "\x02 Klein"
};

// Vorheizung
const char ENUM_Geschlossen_Offen[] PROGMEM = {
  "\x00 Geschlossen\0"
  "\x01 Offen\0"
  "\x02 Unbekannt"
};

// Tastenstatus
const char ENUM_11_1[] PROGMEM = {
  "\x00 Nichts gedrueckt\0"
  "\xFF Fehler"
};

// Priorität Regelung
const char ENUM_9D_19[] PROGMEM = {
  "\x00 Analoge Eingaenge\0"
  "\x01 Wochenprogramm"
};

// Aktuelle Stufe
const char ENUM_CD_9[] PROGMEM = {
  "\x00 Auto (4)\0"
  "\x01 Abwesend (0)\0"
  "\x02 Niedrig (1)\0"
  "\x03 Mittel (2)\0"
  "\x04 Hoch (3)"
};

// Aktiv Status 1
const char ENUM_D5_7[] PROGMEM = {
  "\x00 -\0"
  "\x01 P10\0"
  "\x02 P11\0"
  "\x04 P12\0"
  "\x08 P13\0"
  "\x10 P14\0"
  "\x20 P15\0"
  "\x40 P16\0"
  "\x80 P17"
};

// Aktiv Status 2
const char ENUM_D5_8[] PROGMEM = {
  "\x00 -\0"
  "\x01 P18\0"
  "\x02 P19"
};

// Aktiv Status 3
const char ENUM_D5_9[] PROGMEM = {
  "\x00 -\0"
  "\x01 P90\0"
  "\x02 P91\0"
  "\x04 P92\0"
  "\x08 P93\0"
  "\x10 P94\0"
  "\x20 P95\0"
  "\x40 P96"
};

// Enthalpie anwesend
const char ENUM_D5_10[] PROGMEM = {
  "\x00 Abwesend\0"
  "\x01 Anwesend\0"
  "\x02 ohne Fuehler"
};

// EWT anwesend
const char ENUM_D5_11[] PROGMEM = {
  "\x00 Abwesend\0"
  "\x01 Geregelt\0"
  "\x02 ungeregelt"
};

// Fehler A
const char ENUM_D9_1[] PROGMEM = {
  "\x00 -\0"
  "\x01 A1\0"
  "\x02 A2\0"
  "\x04 A3\0"
  "\x08 A4\0"
  "\x10 A5\0"
  "\x20 A6\0"
  "\x40 A7\0"
  "\x80 A8"
};

// Fehler E
const char ENUM_D9_2[] PROGMEM = {
  "\x00 -\0"
  "\x01 E1\0"
  "\x02 E2\0"
  "\x04 E3\0"
  "\x08 E4\0"
  "\x10 E5\0"
  "\x20 E6\0"
  "\x40 E7\0"
  "\x80 E8"
};

// Filter Status
const char ENUM_D9_9[] PROGMEM = {
  "\x00 Filter OK\0"
  "\x01 Filter voll"
};

// Fehler EA
const char ENUM_D9_10[] PROGMEM = {
  "\x00 -\0"
  "\x01 EA1\0"
  "\x02 EA2\0"
  "\x04 EA3\0"
  "\x08 EA4\0"
  "\x10 EA5\0"
  "\x20 EA6\0"
  "\x40 EA7\0"
  "\x80 EA8"
};

// Fehler E
const char ENUM_D9_14[] PROGMEM = {
  "\x00 -\0"
  "\x01 A9\0"
  "\x02 A10\0"
  "\x04 A11\0"
  "\x08 A12\0"
  "\x10 A13\0"
  "\x20 A14\0"
  "\x40 A15\0"
  "\x80 A0"
};

// Frostsicherheit
const char ENUM_E1_6[] PROGMEM = {
  "\x01 Extra sicher\0"
  "\x04 Sicher"
};

const char ENUM_9B_1[] PROGMEM = {
  "\x00 Ohne Verbindung\0"
  "\x01 Nur PC\0"
  "\x02 Nur CC-Ease\0"
  "\x03 PC Master\0"
  "\x04 PC Logmodus"
};

const char ENUM_airQualitySensor[] PROGMEM = {
  "\x00 Ready\0"
  "\x01 Busy\0"
  "\x10 Warming up\0"
  "\x80 Error\0"
  "\x84 Ready\0"
  "\x85 Busy\0"
  "\x90 Warming up\0"
  "\xFF No Status, check module"
};

// Menu Categories
typedef enum {
  CAT_VENTILATION,
  CAT_TEMPERATURE,
  CAT_STATUS,
  CAT_ERROR,
  CAT_INOUTPUTS,
  CAT_VERSIONS,
  CAT_UNKNOWN
} category_t;

typedef struct {
  uint32_t    cmd;                 // the command or fieldID
  uint8_t     category;            // the menu category
  uint16_t    line;
  const char *desc;
} cmd_t;

const char STR_CAT[] PROGMEM = "Kategorieuebersicht";

// Menue Kategorien
const char ENUM_CAT[]  PROGMEM = {
  "\x00 Ventilatorstatus\0"
  "\x01 Temperaturen\0"
  "\x02 Status\0"
  "\x03 Fehler\0"
  "\x04 Ein-/Ausgaenge\0"
  "\x05 Versionen"
};

const cmd_t cmdtbl[] PROGMEM = {
  {0x0B,        CAT_VENTILATION,      1,     STR_0B_0},      // Ventilatorstatus abrufen
  {0x0D,        CAT_VENTILATION,      2,     STR_0D_0},      // Klappenstatus abrufen
  {0xC9,        CAT_VENTILATION,      3,     STR_C9_0},      // (Zeit) Verzögerung abrufen
  {0xCD,        CAT_VENTILATION,      4,     STR_CD_0},      // Ventilationsstufe abrufen

  {0x0F,        CAT_TEMPERATURE,     10,     STR_0F_0},      // Temperaturstatus abrufen
  {0xD1,        CAT_TEMPERATURE,     11,     STR_D1_0},      // Temperaturen abrufen

  {0xDD,        CAT_STATUS,          20,     STR_DD_0},      // Betriebsstunden abrufen
  {0xDF,        CAT_STATUS,          21,     STR_DF_0},      // Status Bypassregelung abrufen
  {0xE1,        CAT_STATUS,          22,     STR_E1_0},      // Status Vorheizung abrufen
  {0xE9,        CAT_STATUS,          23,     STR_E9_0},      // Letzte 8 mal Vorheizung
  {0xEB,        CAT_STATUS,          24,     STR_EB_0},      // EWT / Nachheizung abrufen

  {0xD9,        CAT_ERROR,           30,     STR_D9_0},      // Störungen abrufen

  {0x03,        CAT_INOUTPUTS,       40,     STR_03_0},      // Eingänge abrufen
  {0x11,        CAT_INOUTPUTS,       41,     STR_11_0},      // Tastenstatus abrufen
  {0x13,        CAT_INOUTPUTS,       42,     STR_13_0},      // Analoge Eingänge abrufen
  {0x97,        CAT_INOUTPUTS,       43,     STR_97_0},      // Sensordaten abrufen
  {0x9D,        CAT_INOUTPUTS,       44,     STR_9D_0},      // Analogwerte abrufen
  {0xD5,        CAT_INOUTPUTS,       45,     STR_D5_0},      // Status abrufen

  {0x67,        CAT_VERSIONS,        50,     STR_67_0},      // Bootloader Version abrufen
  {0x69,        CAT_VERSIONS,        51,     STR_69_0},      // Firmware Version abrufen
  {0xA1,        CAT_VERSIONS,        52,     STR_A1_0},      // Konnektorplatine Version abrufen
  {0xE5,        CAT_VERSIONS,        53,     STR_E5_0},      // RF Status abrufen

  {CMD_END,     CAT_UNKNOWN,          0,           ""},
};
