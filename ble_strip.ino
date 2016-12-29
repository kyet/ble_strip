/* ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <kyet@me.com> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.         Isaac K. Ko
 * ----------------------------------------------------------------------------
 */

/* References
 * DemoReel100 by mark Kriegsman
 */

#include <stdarg.h>
#include <SoftwareSerial.h>  // https://www.arduino.cc/en/Reference/SoftwareSerial
#include <PacketSerial.h>    // https://github.com/bakercp/PacketSerial
#include <FastLED.h>

#if FASTLED_VERSION < 3001000
#error "Requires FastLED 3.1 or later; check github for latest code."
#endif

/* device type */
#define SWITCH       0x01
#define DIMMER       0x02
#define STRIP        0x03
#define STRIP2       0x04
#define DEV_TYPE     (STRIP2)

// undefine for release
//#define __DEBUG__

/* data type */
#define TYPE_GET         0x00
#define TYPE_RAW         0x01
#define TYPE_DIMMING1    0x02
#define TYPE_STRIP1      0x03

/* for bluetooth */
#define BLE_RX           11    // serial pin: connect reverse order
#define BLE_TX           10
#define BLE_DATA_MAX     20    // payload size

SoftwareSerial bleSerial(BLE_RX, BLE_TX);
PacketSerial   pktSerial;

/* number of strip and their pin */
#define STRIP_PIN1  2
#define STRIP_PIN2  3

/* fast led */
#define CHIPSET          NEOPIXEL
#if (DEV_TYPE == STRIP)
const int maxPwr = 2000; // 2A
const int nLed  = 37;
CRGB leds1[nLed];
CRGB leds2[nLed];
#elif (DEV_TYPE == STRIP2)
const int maxPwr = 4000; // 4A
const int nLed  = 160;
CRGB leds1[nLed];
#endif
const byte ledFpsDefault    = 25;
const byte ledHueFpsDefault = 50;
typedef struct {
	// common
	byte mode;
	byte brightness;
	byte fps;

	// effect specific
	union {
		struct {
			byte param[7];
		} generic;
		struct {
			byte huefps;
			byte dir;
			byte chance;
		} rainbow;
		struct {
			byte hue1;
			byte hue2;
			byte hue3;
		} chromoSaturation;
		struct {
			byte huefps;
			byte chance; // unused yet..
		} confetti;
		struct {
			byte huefps;
			byte hue;
			byte range;	
			byte chance; // unused yet..
		} aurora;
	} x; // eXtended parameter
} stripValue;
stripValue strip;

byte saturation   = 255;

/* strip mode */
void stripEffectOff();
void stripEffectDaylight();
void stripEffectChromoSaturation();
void stripEffectRainbow();
void stripEffectRainbowGlitter();
void stripEffectConfetti();
void stripEffectAurora();

typedef void (*stripEffectFunc)();
stripEffectFunc stripEffect[] = {
	stripEffectOff,
	stripEffectDaylight,
	stripEffectChromoSaturation,
	stripEffectRainbow,
	stripEffectRainbowGlitter,
	stripEffectConfetti,
	stripEffectAurora
};

const byte nEffect = sizeof(stripEffect) / sizeof(stripEffect[0]);

void setup() 
{
#if defined(HAVE_HWSERIAL0)
	Serial.begin(9600);

	// wait for serial port to connect
	// needed for native USB port only
	while (!Serial) { ; }
#endif


	bleSerial.begin(9600);

	pktSerial.setPacketHandler(&bleParser);
	pktSerial.begin(&bleSerial);

	if (DEV_TYPE == STRIP || DEV_TYPE == STRIP2)
	{
		delay(3000); // power-up safety delay

		strip.mode        = 6; // aurora 
		strip.brightness  = 255;
		strip.fps         = ledFpsDefault;

		// aqua family - 110~130 (best:125)
		// purple/pink family - 200~230 (best:200/220)
		strip.x.aurora.hue   = 195;
		strip.x.aurora.range = 40;
//		strip.x.aurora.hue   = 100;
//		strip.x.aurora.range = 40;

		FastLED.addLeds<CHIPSET, STRIP_PIN1>(leds1, nLed).setCorrection(TypicalSMD5050);
#if (DEV_TYPE == STRIP)
		FastLED.addLeds<CHIPSET, STRIP_PIN2>(leds2, nLed).setCorrection(TypicalSMD5050);
#endif
		FastLED.setBrightness(strip.brightness);
//		FastLED.setTemperature(Candle);
		set_max_power_in_volts_and_milliamps(5, maxPwr); // deprecated.. 
		//FastLED.setMaxPowerInVoltsAndMilliamps(5, maxPwr);  // FastLED 3.1.1
	}
}

void refreshStrip()
{
	if (strip.mode >= nEffect){ return; }

	stripEffect[strip.mode]();
	FastLED.show();
}

void stripEffectOff()
{
	fill_solid(leds1, nLed, CRGB::Black);
#if (DEV_TYPE == STRIP)
	fill_solid(leds2, nLed, CRGB::Black);
#endif

	// FIXME: below not work..
#if 0
	for (int i=0; i<nLed; i++)
		leds1[i].fadeToBlackBy(1);
#endif
}

void stripEffectDaylight()
{
	fill_solid(leds1, nLed, CRGB::FairyLight);
#if (DEV_TYPE == STRIP)
	fill_solid(leds2, nLed, CRGB::FairyLight);
#endif
}

void chromoIndex(byte *n1, byte *n2, byte *n3)
{
	byte r;

	*n1 = *n2 = *n3 = (nLed / 3);

	r = nLed % 3;
	if (r == 1)
	{
		(*n2)++;
	}
	if (r == 2)
	{
		(*n1)++;
		(*n3)++;
	}
}

void stripEffectChromoSaturation()
{
	byte n1, n2, n3;
	chromoIndex(&n1, &n2, &n3);

	fill_solid(leds1,           n1, CHSV(strip.x.chromoSaturation.hue1, saturation, strip.brightness));
	fill_solid(leds1 + n1,      n2, CHSV(strip.x.chromoSaturation.hue2, saturation, strip.brightness));
	fill_solid(leds1 + n1 + n2, n3, CHSV(strip.x.chromoSaturation.hue3, saturation, strip.brightness));

#if (DEV_TYPE == STRIP)
	fill_solid(leds2,           n1, CHSV(strip.x.chromoSaturation.hue1, saturation, strip.brightness));
	fill_solid(leds2 + n1,      n2, CHSV(strip.x.chromoSaturation.hue2, saturation, strip.brightness));
	fill_solid(leds2 + n1 + n2, n3, CHSV(strip.x.chromoSaturation.hue3, saturation, strip.brightness));
#endif
}

void stripEffectRainbow()
{
	byte static hue = ledHueFpsDefault;

	if (strip.x.rainbow.huefps > 0)
	{
		EVERY_N_MILLISECONDS(1000/strip.x.rainbow.huefps) { hue++; }
	}

	fill_rainbow(leds1, nLed, hue, 255/nLed);
#if (DEV_TYPE == STRIP)
	if (strip.x.rainbow.dir == 0)
		fill_rainbow(leds2, nLed, hue, 255/nLed);
	else // reverse direction
		fill_rainbow(leds2, nLed, (255-hue), 255/nLed);
#endif
}

void rainbowGlitter(fract8 chance)
{
	// add glitter
	if(random8() < chance)
		leds1[random8(nLed)] += CRGB::White;
}

void stripEffectRainbowGlitter()
{
	stripEffectRainbow();
	rainbowGlitter(strip.x.rainbow.chance);
}

void stripEffectConfetti()
{
	byte static hue = ledHueFpsDefault;
	uint8_t pos = random8(nLed);

	if (strip.x.confetti.huefps > 0)
	{
		EVERY_N_MILLISECONDS(1000/strip.x.confetti.huefps) { hue++; }
	}

	fadeToBlackBy(leds1, nLed, 10);
#if (DEV_TYPE == STRIP)
	fadeToBlackBy(leds2, nLed, 10);
#endif

	leds1[pos] += CHSV(hue + random8(64), 200, 255);
#if (DEV_TYPE == STRIP)
	leds2[pos] += CHSV(hue + random8(64), 200, 255);
#endif
}

void stripEffectAurora()
{
	static uint8_t delta = 0;
	uint8_t i, v, r;

	for(i=0; i<nLed; i++)
	{
		v = cubicwave8(i + delta);
		v = map(v, 0, 255, 50, 255);

		r = random8(strip.x.aurora.range);
		leds1[i] = CHSV(strip.x.aurora.hue + r, 255, v);
	}

	delta++;
}

/* Payload (data) 
 * .------------------------------------------------.
 * | mode | brightness | fps | param1 | .. | param7 |
 * |------+------------+-----+--------+----+--------|
 * |    1 |          1 |   1 |      1 | .. |      1 |
 * '------------------------------------------------'
 */
void bleStrip1(const byte *data, const byte sz)
{
	const byte stripSize = sizeof(stripValue);
	stripValue *dv = NULL;

	// sanity check
	if (sz > stripSize) { return; }
	if (DEV_TYPE != STRIP && DEV_TYPE != STRIP2) { return; }

	dumpPkt(data, sz);

	dv = (stripValue *)data;

	syslog("STRIP mode(%d) brightness(%d) fps(%d)", 
		dv->mode, dv->brightness, dv->fps);

	memcpy(&strip, data, sz);
}

/* Payload (data) 
 * .--------------------------.
 * | type | len | on/off | .. |
 * |------+-----+--------+----|
 * |    1 |   1 |      1 | .. |
 * '--------------------------'
 */
void bleGetState()
{
	// save 1 byte for COBS encoding
	byte datagram[BLE_DATA_MAX-1] = {0};
	byte idx = 0;

	datagram[idx++] = DEV_TYPE;
	datagram[idx++] = 0; // len. fill later

	switch (DEV_TYPE)
	{
		case SWITCH:
			datagram[idx++] = 0; //FIXME
			break;
			
		case DIMMER:
			datagram[idx++] = 0; //FIXME
			break;

		case STRIP:
		case STRIP2:
			if (strip.mode == 0){ datagram[idx++] = 0; }
			else                { datagram[idx++] = 1; }

			memcpy(datagram + idx, &strip, sizeof(strip));
			idx += sizeof(strip);
			break;
	}

	datagram[1] = idx; // len
	pktSerial.send(datagram, idx);
}

/* Datagram (buffer)
   .----------------------------------.
   | Type | Length | Payload          |
   |------+--------+------------------|
   |    1 |      1 | BLE_DATA_MAX - 2 |
   '----------------------------------'
*/
void bleParser(const byte* buffer, size_t size)
{
	byte datagram[BLE_DATA_MAX] = {0};
	byte type, sz;
	char buf[128];

	if (buffer == NULL)
		return; 

	if (size > BLE_DATA_MAX)
		return; 

	dumpPkt(buffer, size);
	memcpy(datagram, buffer, size); 

	// parse header
	type = datagram[0];
	sz   = datagram[1];

	if (sz < 2 || sz > BLE_DATA_MAX)
		return;

	switch(type)
	{
		case TYPE_RAW:
//			bleRaw(datagram + 2, sz - 2);
			break;
		case TYPE_DIMMING1:
//			if (sz < 3) { break; }
//			bleDimming1(datagram + 3, sz - 3, *(datagram + 2));
			break;
		case TYPE_STRIP1:
			bleStrip1(datagram + 2, sz - 2);
			break;
		case TYPE_GET:
			bleGetState();
			break;
		default:
			break;
	}
}

void loop() 
{
	pktSerial.update();

	/*  30 fps -> 33.3 ms
	    60 fps -> 16.6 ms
	   120 fps ->  9.3 ms */
	if (strip.fps == 0)
	{
		strip.fps = ledFpsDefault;
	}

	EVERY_N_MILLISECONDS(1000/strip.fps) { refreshStrip(); }
}

void dumpPkt(const byte* packet, size_t size)
{
#if defined(HAVE_HWSERIAL0) && defined(__DEBUG__)
	char buf[10] = "";

	sprintf(buf, "DUMP(%d) ", size);
	Serial.print(buf);

	for(int i=0; i<size; i++)
	{
		sprintf(buf, "[%02X] ", packet[i]);
		Serial.print(buf);
	}

	Serial.println();
#endif
}

void syslog(char *fmt, ... )
{
#if defined(HAVE_HWSERIAL0) && defined(__DEBUG__)
	char buf[128];
	va_list args;
	
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), (const char *)fmt, args);
	va_end(args);

	Serial.println(buf);
#endif
}
