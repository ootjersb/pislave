#ifndef DATALOGPARSER_H
#define DATALOGPARSER_H

#include "field.h"

#define FIELD_COUNT_WARMTEPOMP 	28
#define FIELD_COUNT_AUTOTEMP 	25

#define DEVICE_ID_WARMTEPOMP 13
#define DEVICE_ID_AUTOTEMP   15

class DatalogParser
{
    public:
        int fieldCount;

        DatalogParser(int device);
        void Parse(unsigned char *buffer, int length);
		bool ParseWithHeader(unsigned char *buffer, int length);
        void Parse(char *hexstring);
        float ParseSignedIntDec2(unsigned char * buffer);
        int ParseByte(unsigned char *buffer);
        unsigned int ParseUnsignedInt(unsigned char *buffer);
		char *FieldValue(int index);
		char *FieldValue(string label);
		bool FieldChanged(int index);
		bool FieldChanged(string label);

    private:
		char errorMessage[100];
		char fieldValues[FIELD_COUNT_WARMTEPOMP][50];	// only the bigger of the two taken here
		char previousValues[FIELD_COUNT_WARMTEPOMP][50];	// only the bigger of the two taken here
        Field *config;
        Field warmtepompConfig[FIELD_COUNT_WARMTEPOMP] {{ 0, SignedIntDec2, "Buitentemperatuur"}, 
                                        { 2, SignedIntDec2, "Boiler laag"},
                                        { 4, SignedIntDec2, "Boiler hoog"},
                                        { 6, SignedIntDec2, "Verdamper temperatuur"},
                                        { 8, SignedIntDec2, "Zuiggas temperatuur"},
                                        { 10, SignedIntDec2, "Persgas temperatuur"},
                                        { 12, SignedIntDec2, "Vloeistof temperatuur"},
                                        { 14, SignedIntDec2, "Naar bron"},
                                        { 16, SignedIntDec2, "Van bron"},
                                        { 18, SignedIntDec2, "CV aanvoer"},
                                        { 20, SignedIntDec2, "CV retour"},
                                        { 22, SignedIntDec2, "Druksensor"},
                                        { 24, SignedIntDec2, "Stroom trafo 1"},
                                        { 26, SignedIntDec2, "Stroom trafo 2"},
                                        { 34, UnsignedInt, "Flow sensor bron"},
                                        { 37, Byte, "Snelheid cv pomp (%)"},
                                        { 38, Byte, "Snelheid bron pomp (%)"},
                                        { 39, Byte, "Snelheid boiler pomp (%)"},
                                        { 44, Byte, "Compressor aan/uit"},
                                        { 45, Byte, "Elektrisch element aan/uit"},
                                        { 46, Byte, "Trickle heating aan/uit"},
                                        { 47, Byte, "Fout aanwezig (0=J, 1=N)"},
                                        { 48, Byte, "Vrijkoelen actief (0=uit, 1=aan)"},
										{ 51, SignedIntDec2, "Kamertemperatuur"},
                                        { 56, Byte, "State (0=init,1=uit,2=CV,3=boiler,4=vrijkoel,5=ontluchten)"},
                                        { 57, Byte, "Substatus (255=geen)"},
                                        { 67, Byte, "Fout gevonden (foutcode)"},
                                        { 92, Byte, "Vrijkoelen interval (sec)"}};

        Field autotempConfig[FIELD_COUNT_AUTOTEMP] {{ 0, Byte, "Modus"}, 
                                                    { 1, Byte, "Toestand"},
                                                    { 2, Byte, "Status"},
                                                    { 3, Byte, "Warmtebron"},
                                                    { 4, Byte, "Foutcode"},
                                                    { 5, Byte, "Gewenst vermogen"},
                                                    { 6, SignedIntDec2, "Ruimte 1 temp"},
                                                    { 13, SignedIntDec2, "Ruimte 2 temp"},
                                                    { 20, SignedIntDec2, "Ruimte 3 temp"},
                                                    { 27, SignedIntDec2, "Ruimte 4 temp"},
                                                    { 34, SignedIntDec2, "Ruimte 5 temp"},
                                                    { 41, SignedIntDec2, "Ruimte 6 temp"},
                                                    { 90, Byte, "Toestand uit"},
                                                    { 91, Byte, "Toestand koel"},
                                                    { 92, Byte, "Toestand verwarmen"},
                                                    { 93, Byte, "Toestand hand"},
                                                    { 142, UnsignedInt, "Lege batterij (0=OK)"},
                                                    { 150, SignedIntDec2, "Buitentemperatuur"},
                                                    { 152, UnsignedInt, "Rest cyclustijd"},
                                                    { 156, UnsignedInt, "Comm Ruimte A"},
                                                    { 158, UnsignedInt, "Comm Ruimte B"},
                                                    { 160, UnsignedInt, "Comm Ruimte C"},
                                                    { 162, UnsignedInt, "Comm Ruimte D"},
                                                    { 164, UnsignedInt, "Comm Ruimte E"},
                                                    { 166, UnsignedInt, "Comm Ruimte F"}};
    int char2int(char input);
    void hex2bin(const char* src, unsigned char* target);
	int GetFieldIndex(string label);
	bool IsChecksumValid(unsigned char *buffer, int length);
};

#endif