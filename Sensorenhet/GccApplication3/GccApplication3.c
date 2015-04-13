/*
 * GccApplication3.c
 *
 * Created: 3/26/2015 1:17:55 PM
 *  Author: rasvi146
 */ 

#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
//#include <time.h>

int tempReading;

int sensor1[5]; // De 5 senaste avläsningarna för varje sensor
int sensor2[5];
int sensor3[5];
int sensor4[5];
int sensor5[5];
int sensor6[5];
int sensor7[5];
int sensor8[5];

int averageDistance1; // Medelvärdesavbildad sensoravläsning
int averageDistance2;
int averageDistance3;
int averageDistance4;
int averageDistance5;
int averageDistance6;
int averageDistance7;
int averageDistance8;


int sideAngle1; 
int sideAngle2;
int sideAngle3;
int sideAngle4;

int totalAngle; // Robotens vinkel

// Sidornas avstånd på roboten
int sideDistance1; // Beror på sensor 1 och 2
int sideDistance2; // Beror på sensor 3 och 3
int sideDistance3; // Beror på sensor 5 och 6
int sideDistance4; // Beror på sensor 7 och 8


//Tabell för att omvandla A/d-omvandlat värde till avstånd
// Måste skapa en per sensor
//										   00  01  02  03  04  05  06  07  08  09  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96  97  98  99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160
	const int distanceTable1[] PROGMEM = {800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,780,730,720,710,670,660,640,620,560,550,530,520,500,480,470,450,440,420,410,400,390,370,360,350,340,330,320,315,310,300,297,292,286,275,268,264,262,254,251,247,237,231,230,227,225,219,216,213,205,203,199,198,196,192,190,188,183,180,178,176,172,171,170,168,166,162,160,159,157,155,154,152,151,147,146,145,143,142,141,139,137,135,134,133,132,130,129,128,127,126,124,123,122,121,120,118,118,116,115,114,113,112,111,110,109,107,106,105,105,104,104,103,101,100, 99, 99, 98, 97, 96, 96, 94, 93, 93, 92, 91, 91, 90, 90, 88, 87, 86, 85, 85, 84, 83, 83, 83, 83}; // Minus en konstant, (14mm för mycket i tabellerna)
									//	   00  01  02  03  04  05  06  07  08  09  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96  97  98  99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160
	const int distanceTable2[] PROGMEM = {800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,790,780,740,720,700,640,615,595,585,560,530,520,510,480,460,445,440,430,415,400,390,380,365,350,340,330,320,315,310,302,293,283,280,277,266,264,260,245,242,239,238,233,227,226,224,215,212,206,205,203,198,195,193,188,184,182,180,177,175,173,172,170,166,164,162,161,158,157,156,154,151,149,147,145,144,142,141,138,137,135,134,133,132,131,130,128,126,125,124,123,122,121,120,119,117,115,114,114,113,112,111,110,108,106,106,105,104,103,102,101,100, 99, 99, 98, 96, 96, 95, 94, 93, 92, 92, 91, 90, 90, 89, 88, 86, 85, 85, 84, 82, 81, 80, 78, 78}; // Minus en konstant, (14mm för mycket i tabellerna)         
									//	   00  01  02  03  04  05  06  07  08  09  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96  97  98  99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160
	const int distanceTable3[] PROGMEM = {800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,780,760,750,730,710,690,670,620,600,580,560,540,520,510,500,460,450,430,425,420,390,385,380,370,350,340,335,330,320,310,305,297,284,279,275,270,263,259,254,239,237,234,230,227,224,222,220,211,208,204,203,200,195,193,190,183,181,179,177,174,172,171,169,167,163,160,159,158,155,154,153,152,148,146,145,142,141,140,139,136,135,134,133,132,130,129,127,126,125,123,122,121,120,119,118,117,115,114,114,113,112,110,109,109,108,107,106,105,104,103,102,101,100, 99, 98, 98, 97, 96, 95, 95, 94, 94, 93, 92, 91, 91, 90, 90, 89, 88, 87, 86, 85, 84, 83, 83, 83}; // Minus en konstant, (14mm för mycket i tabellerna)
									//	   00  01  02  03  04  05  06  07  08  09  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96  97  98  99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160
	const int distanceTable4[] PROGMEM = {800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,780,760,750,700,670,660,600,590,560,545,530,500,490,480,450,440,420,415,410,390,380,370,360,350,340,330,320,310,308,305,298,286,278,275,271,262,259,255,242,240,238,236,231,227,226,222,215,213,209,207,205,199,197,194,188,186,184,182,179,177,175,174,172,168,165,164,163,160,159,157,156,153,151,150,147,146,145,144,141,140,139,138,137,136,135,133,132,130,128,127,126,125,124,123,122,122,119,118,117,116,115,114,113,111,110,109,108,107,106,105,104,103,102,101,100,100, 99, 98, 96, 95, 94, 93, 93, 92, 91, 91, 90, 89, 88, 87, 87, 85, 84, 83, 83, 83}; // Minus en konstant, (14mm för mycket i tabellerna)
									//	   00  01  02  03  04  05  06  07  08  09  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96  97  98  99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160
	const int distanceTable5[] PROGMEM = {800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,760,750,740,700,685,670,610,600,570,565,540,520,500,490,470,450,430,420,405,390,385,380,365,350,340,330,320,310,305,301,297,286,277,275,272,266,261,255,240,235,234,232,227,223,219,216,209,206,203,200,197,194,192,189,183,182,180,177,175,174,172,169,166,163,160,158,157,155,154,152,151,147,145,144,143,141,140,138,136,134,133,132,132,131,129,127,126,124,122,121,120,120,119,118,117,114,113,112,111,110,109,108,107,106,105,104,103,103,102,101,100, 99, 98, 97, 97, 95, 94, 93, 92, 91, 91, 90, 89, 88, 88, 87, 86, 85, 84, 84, 83, 82, 82, 82, 82, 82}; // Minus en konstant, (14mm för mycket i tabellerna)
									//	   00  01  02  03  04  05  06  07  08  09  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96  97  98  99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160
	const int distanceTable6[] PROGMEM = {800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,760,750,720,690,670,650,600,580,560,550,520,500,490,480,450,440,430,420,410,390,380,370,360,350,340,330,320,310,305,302,296,284,278,275,273,263,260,258,241,240,239,235,232,228,225,221,214,211,208,207,205,200,198,197,189,187,185,183,180,178,176,175,173,170,166,165,164,160,159,158,157,154,151,149,148,146,145,144,141,140,139,137,136,135,134,133,132,129,128,127,126,125,124,123,122,119,118,118,117,116,114,113,112,111,110,109,107,106,105,104,104,103,102,101,100, 99, 98, 97, 96, 95, 95, 94, 94, 92, 91, 91, 90, 89, 88, 87, 87, 85, 85, 84, 84, 84}; // Minus en konstant, (14mm för mycket i tabellerna)
									//	   00  01  02  03  04  05  06  07  08  09  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96  97  98  99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160
	const int distanceTable7[] PROGMEM = {800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,790,770,750,690,675,660,610,590,570,550,530,510,500,490,460,440,430,420,410,390,380,375,370,350,340,335,330,320,310,306,300,286,282,277,272,263,250,257,244,241,239,235,231,227,225,224,215,213,207,205,204,199,196,193,188,185,184,182,179,176,175,173,171,168,165,163,161,160,158,157,155,151,150,148,146,145,144,143,140,139,138,137,136,134,133,132,131,129,127,126,125,124,123,122,121,119,118,117,117,115,114,113,112,111,110,109,108,107,106,105,103,102,101,101,100,100, 99, 98, 96, 95, 94, 94, 93, 92, 91, 91, 90, 89, 88, 87, 87, 86, 84, 83, 83, 83}; // Minus en konstant, (14mm för mycket i tabellerna)
									//	   00  01  02  03  04  05  06  07  08  09  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96  97  98  99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160
	const int distanceTable8[] PROGMEM = {800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,780,750,735,720,690,670,650,600,580,560,540,520,510,500,490,460,440,430,420,410,390,380,370,360,350,340,330,320,310,304,297,291,278,272,270,268,258,255,252,238,135,232,228,225,223,220,215,208,205,203,201,199,194,192,190,184,182,180,178,175,173,171,169,167,164,161,160,158,157,155,153,152,149,147,145,143,141,140,139,137,135,134,133,132,130,129,128,127,125,123,122,121,120,119,119,118,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,101,100, 99, 98, 97, 96, 95, 94, 94, 93, 92, 91, 91, 90, 90, 89, 89, 87, 86, 86, 85, 85, 84, 83, 82, 82}; // Minus en konstant, (14mm för mycket i tabellerna)

//-------------------------------
//-------------------------------
// Ställ in rätt sidoavstånd!!!!!
//-------------------------------
//-------------------------------

// Tabell för att omvandla differens i sensoravstånd till vinkel	
//int angleTable[] = {0, (180/3.141592)*asin(1/(sqrt(1 + 15*15))), (180/3.141592)*asin(2/(sqrt(4 + 15*15))), (180/3.141592)*asin(3/(sqrt(9 + 15*15))), (180/3.141592)*asin(4/(sqrt(16 + 15*15))), (180/3.141592)*asin(5/(sqrt(25 + 15*15))), (180/3.141592)*asin(6/(sqrt(36 + 15*15))), (180/3.141592)*asin(7/(sqrt(49 + 15*15))), (180/3.141592)*asin(8/(sqrt(64 + 15*15))), (180/3.141592)*asin(9/(sqrt(81 + 15*15))), (180/3.141592)*asin(10/(sqrt(100 + 15*15))), (180/3.141592)*asin(11/(sqrt(121 + 15*15))), (180/3.141592)*asin(12/(sqrt(144 + 15*15))), (180/3.141592)*asin(13/(sqrt(169 + 15*15))), (180/3.141592)*asin(14/(sqrt(196 + 15*15)))};

#define differentAngles 150 // Hur många vinklar som finns i look-up-table
#define sidelenght	175		// Sidan mellan två sensorer, 175 millimeter
#define displayE	PORTB1	// E på displayen
#define displayRS	PORTB0	// RS på displayen
#define ADC_notComplete	!(ADCSRA & (1<<ADIF)) // Boolvariabel för ADC complete
#define leakBit		PORTC1	// Output hos IR-mottagaren
#define pi			3.141592

int angleTable[differentAngles];

void makeAngleTable() // Skapar angleTable med 150 element som motsvarar differens (millimeter), lägger på varje plats motsvarande vinkel
{
	int i;
	for (i=0; i<differentAngles; i++)
	{
		angleTable[i]=10*(180/pi)*asin(i/(sqrt(i*i+sidelenght*sidelenght))); // En decimal
	}
}

void writeDisplay(int asciiCode)
{
	PORTB = (1<< displayE) | (1<< displayRS);
	PORTD = asciiCode; // Matar in asciikoden i displayen
	PORTB = (0<< displayE);
	_delay_ms(1); // Väntar på att instruktion utförs
}

void initDisplay()
{
	DDRD = 0xFF; // Sätter PD till outport, tror jag
	DDRB = 0xFF; // Sätter PB 0 och 1 till ut PB0= RS, PB1 = E
	
	_delay_ms(30);
	PORTB = (1<<displayE);
	PORTD = (1<<PORTD5) | (1<<PORTD4) | (1<<PORTD3) | (0<<PORTD2); // Instruktion "Funktion Set"
	PORTB = (0<<displayE);
	_delay_ms(1);
	
	PORTB = (1<<displayE);
	PORTD = (0<<PORTD5) | (0<<PORTD4) | (1<<PORTD3) | (1<<PORTD2) | (0<<PORTD1) | (0<<PORTD0); // Instruktion "Display ON/OFF" med blink och cursor av
	PORTB = (0<<displayE);
	_delay_ms(1);
	
	
	PORTB = (1<<displayE);
	PORTD = (0<<PORTD3) | (0<<PORTD2) | (1<<PORTD0); // DB0 till 1 för intstruktion "Clear Display"
	PORTB = (0<<displayE);
	_delay_ms(2);      // Väntar på att instruktion utförs
	
	
	PORTB = (1<<displayE);
	PORTD = (0<<PORTD0) | (1<<PORTD1) | (1<<PORTD2); // Instruktion  "Entry Mode Set" med "Increment"
	PORTB = (0<<displayE);
	_delay_ms(1);
	
	PORTB = (1<<displayE);
	PORTD = (1<<PORTD1); // Return Home
	PORTB = (0<<displayE);
	_delay_ms(2);
	
	
};

void writeSensor(int whatSensor)
{
		int number1;
		int number2;
		int number3;
		if (whatSensor >= 0)
		{
			writeDisplay(20);
		}
		else
		{
			writeDisplay(45);
			
		}
		
		whatSensor = abs(whatSensor);
		
		number3 = (whatSensor % 100) % 10; // Entalet
		number2 = ((whatSensor % 100) - number3) / 10; // Tiotalet
		number1 = (whatSensor - (number2 * 10) - number3) / 100; // Hundratalet
		writeDisplay(number1 + 48); // Plussar på 48 för att omvandla till ascii
	 	writeDisplay(number2 + 48);
		writeDisplay(number3 + 48);
}

int compareFunction (const void * firstValue, const void * secondValue)
{
   return ( *(int*)firstValue - *(int*)secondValue ); // Sorterar i stigande ordning
}



int median(int medianSensor[5])
{
	qsort(medianSensor, 5, sizeof(int), compareFunction); // Sorterar alla sensorvärden
	return medianSensor[2]; // Returnerar medianen
}

int average(int averageSensor[5])
{
	qsort(averageSensor, 5, sizeof(int), compareFunction); // Sorterar alla sensorvärden
	int medianValue = averageSensor[2]; 
	if (abs(averageSensor[1] - medianValue) > abs(averageSensor[4] - medianValue)) // Om näst minsta värdet avviker mer än största ska de två minsta slängas bort
	{
		return (averageSensor[2] + averageSensor[3] + averageSensor[4])/3; // Tar medelvärdet av resterande
	}
	else if (abs(averageSensor[3] - medianValue) > abs(averageSensor[0] - medianValue)) // Om näst största värdet avviker mer än minsta ska de två största slängas bort
	{
		return (averageSensor[0] + averageSensor[1] + averageSensor[2])/3;
	}
	else // Annars ska största och minsta slängas bort
	{
		return (averageSensor[1] + averageSensor[2] + averageSensor[3])/3;		
	}
}

void startAD(int muxBit2, int muxBit1, int muxBit0)
{
	ADMUX = (muxBit2<<MUX2) | (muxBit1<<MUX1) | (muxBit0<<MUX0); // Väljer port som ska AD-omvandlas
	ADCSRA = (1<<ADEN) | (1<<ADSC) | (1<<ADPS1) | (1<<ADPS0); // Startar AD-omvandling
}


int sideValue(int firstSensor, int secondSensor)
{
	if (abs(firstSensor - secondSensor) > 10) // Om differensen mellan sensorerna är större än 10 returneras kortaste avståndet
	{
		if (firstSensor < secondSensor)
		{
			return firstSensor;
		}
		else
		{
			return secondSensor;
		}
	}	
	else
	{
		return (firstSensor + secondSensor)/2; // Annars returneras medelvärdet av avstånden
	}
}


int getAngle(int firstLength, int secondLength)
{
	int diffLength = firstLength - secondLength;
	if (firstLength < 300 && secondLength < 300 && abs(diffLength) < 150)
	{
		if (diffLength >= 0)
		{
			return angleTable[diffLength];
		}
		else
		{
			return -angleTable[abs(diffLength)];
		}
	}
	else
	{
		return 0;
	}
}

void waitForConversionComplete()
{
	while (ADC_notComplete) {} // Loopar tills ADIF=1 dvs ADC klar
	
	ADMUX = (1<<ADLAR); // Tar omv. värde från ADLAR1
	tempReading = ADCH; // Sparar omvandlade värdet i tempreading
	ADCSRA = (1<<ADIF); // Clearar ADIF
}

void calculateAvarageDistance() // Räknar ut medelvärdet av 5 senaste mätningarna och avståndet från varje sida
{
			averageDistance1 = pgm_read_word(&(distanceTable1[average(sensor1)]));
			averageDistance2 = pgm_read_word(&(distanceTable2[average(sensor2)]));
			averageDistance3 = pgm_read_word(&(distanceTable1[average(sensor3)]));
			averageDistance4 = pgm_read_word(&(distanceTable1[average(sensor4)]));
			averageDistance5 = pgm_read_word(&(distanceTable1[average(sensor5)]));
			averageDistance6 = pgm_read_word(&(distanceTable1[average(sensor6)]));
			averageDistance7 = pgm_read_word(&(distanceTable1[average(sensor7)]));
			averageDistance8 = pgm_read_word(&(distanceTable1[average(sensor8)]));
			
			sideDistance1 = sideValue(averageDistance1, averageDistance2);
			sideDistance2 = sideValue(averageDistance3, averageDistance4);
			sideDistance3 = sideValue(averageDistance5, averageDistance6);
			sideDistance4 = sideValue(averageDistance7, averageDistance8);
}

void calculateAngle() // Räknar ut vinklar hos varje sida, och ett genomsnitt av nollskillda vinklar
{
			int angleDivisor = 0;
			
			sideAngle1 = getAngle(averageDistance2, averageDistance1);
			sideAngle2 = getAngle(averageDistance4, averageDistance3);
			sideAngle3 = getAngle(averageDistance6, averageDistance5);
			sideAngle4 = getAngle(averageDistance8, averageDistance7);
			
			if (sideAngle1 != 0)
			{
				angleDivisor ++;
			}
			
			if (sideAngle2 != 0)
			{
				angleDivisor ++;
			}
			
			if (sideAngle3 != 0)
			{
				angleDivisor ++;
			}
			
			if (sideAngle4 != 0)
			{
				angleDivisor ++;
			}
			
			if (angleDivisor != 0)
			{
				totalAngle = (sideAngle1 + sideAngle2 + sideAngle3 + sideAngle4)/angleDivisor;
			}
			else
			{
				totalAngle = 0;
			}
}

/*
void leakAlert()
{
	
}
*/

int main(void)
{	
	initDisplay();
	
  	int leakFound = 0; // "Bool" 1=true, 0=false
	int iteration = 0; // Iterator för vilken avläsning på sensorn som görs
	
	makeAngleTable();
	
    while(1)
    {
		leakFound = leakBit;
				
		if (iteration >= 4) // iteration används så att det görs 5 mätningar per sensor
		{
			iteration = 0;
		}
		else
		{
			iteration = iteration + 1;
		}
		
		startAD(0,0,0);
		waitForConversionComplete();
		sensor1[iteration] = tempReading;
		
		startAD(0,0,1);
		waitForConversionComplete();
		sensor2[iteration] = tempReading;
		
		startAD(0,1,0);
		waitForConversionComplete();
		sensor3[iteration] = tempReading;
		
		startAD(0,1,1);
		waitForConversionComplete();
		sensor4[iteration] = tempReading;		
		
		startAD(1,0,0);
		waitForConversionComplete();
		sensor5[iteration] = tempReading;
		
		startAD(1,0,1);
		waitForConversionComplete();
		sensor6[iteration] = tempReading;
		
		startAD(1,1,0);
		waitForConversionComplete();
		sensor7[iteration] = tempReading;

		startAD(1,1,1);
		waitForConversionComplete();
		sensor8[iteration] = tempReading;				
		
		if (iteration == 4)
		{
			//clock_t beginCalculations = clock(); // Startar klocka för att beräkna tiden uträkning och utskrift tar
			
			calculateAvarageDistance();
			
			calculateAngle();
			writeSensor(sideDistance1); // Skriver ut alla sensorvärden
			writeSensor(sideDistance2);
			writeSensor(sideDistance3);
			writeSensor(sideDistance4);
			
			PORTB = (1<<displayE);
			PORTD = (1<<PORTD6) | (1<<PORTD7); // Rad 2
			PORTB = (0<<displayE);
			_delay_ms(1);
			
			writeSensor(totalAngle);
			writeSensor(leakFound);
			writeSensor(sideAngle1);
			writeSensor(sideAngle2);
		
			PORTB = (1<<displayE);
			PORTD = (1<<PORTD7); // Rad 1
			PORTB = (0<<displayE);
			_delay_ms(1);

			//clock_t calculationsComplete = clock(); // Avslutar klocka, drar av denna tid från väntetid

			_delay_ms(40 /*- (beginCalculations - calculationsComplete)*/); // Väntar på att sensorerna uppdateras
		}
		else
		{
			_delay_ms(40); // Väntar på att sensorerna uppdateras
		}
	}
}