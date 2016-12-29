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
#define DEV_TYPE     (STRIP)

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
const byte nOutlet = 2;

/* fast led */
#define CHIPSET          NEOPIXEL
const int nLed  = 37;
CRGB leds1[nLed], leds2[nLed];
const byte ledFpsDefault = 30;
const byte ledHueDefault = 50;
typedef struct {
	byte mode;
	byte brightness;
	byte fps;
	byte param1; /* rainbow+, confetti */
	byte param2; /* confetti */
	byte param3;
} stripValue;
stripValue strip;

byte saturation   = 255;
byte hue          = 0;

/* strip mode */
void stripEffectOff();
void stripEffectDaylight();
void stripEffectChromoSaturation();
void stripEffectChromoSaturation2();
void stripEffectRainbow();
void stripEffectRainbow2();
void stripEffectRainbowGlitter();
void stripEffectRainbowGlitter2();
void stripEffectConfetti();

typedef void (*stripEffectFunc)();
stripEffectFunc stripEffect[] = {
	stripEffectOff,
	stripEffectDaylight,
	stripEffectChromoSaturation,
	stripEffectRainbow,
	stripEffectRainbowGlitter,
	stripEffectConfetti,
	stripEffectChromoSaturation2,
	stripEffectRainbow2,
	stripEffectRainbowGlitter2,
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

	if (DEV_TYPE == STRIP)
	{
		delay(3000); // power-up safety delay

		strip.mode       = 2; // chromo saturation
		strip.brightness = 255;
		strip.fps        = ledFpsDefault;
		strip.param1     = ledHueDefault;
		strip.param2     = 0;
		strip.param3     = 0;

		FastLED.addLeds<CHIPSET, STRIP_PIN1>(leds1, nLed).setCorrection(TypicalSMD5050);
		FastLED.addLeds<CHIPSET, STRIP_PIN2>(leds2, nLed).setCorrection(TypicalSMD5050);
		FastLED.setBrightness(strip.brightness);
		FastLED.setTemperature(Candle);
		set_max_power_in_volts_and_milliamps(5, 2000); // deprecated.. 
		//FastLED.setMaxPowerInVoltsAndMilliamps(5, 2000);  // FastLED 3.1.1
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
	fill_solid(leds2, nLed, CRGB::Black);

	// FIXME: below not work..
#if 0
	for (int i=0; i<nLed; i++)
		leds1[i].fadeToBlackBy(1);
#endif
}

void stripEffectDaylight()
{
	fill_solid(leds1, nLed, CRGB::FairyLight);
	fill_solid(leds2, nLed, CRGB::FairyLight);
}

void stripEffectChromoSaturation()
{
	if (nLed != 37)
	{ 
		syslog("hey! modify hand crafted constant");
		return;
	}

	fill_solid(leds1 + 0,  12, CHSV(HUE_GREEN, saturation, strip.brightness));
	fill_solid(leds1 + 12, 13, CHSV(HUE_RED,   saturation, strip.brightness));
	fill_solid(leds1 + 25, 12, CHSV(HUE_BLUE,  saturation, strip.brightness));

	fill_solid(leds2 + 0,  12, CHSV(HUE_GREEN, saturation, strip.brightness));
	fill_solid(leds2 + 12, 13, CHSV(HUE_RED,   saturation, strip.brightness));
	fill_solid(leds2 + 25, 12, CHSV(HUE_BLUE,  saturation, strip.brightness));
}

/* leds2 have different color */
void stripEffectChromoSaturation2()
{
	if (nLed != 37)
	{ 
		syslog("hey! modify hand crafted constant");
		return;
	}

	fill_solid(leds1 + 0,  12, CHSV(HUE_GREEN,  saturation, strip.brightness));
	fill_solid(leds1 + 12, 13, CHSV(HUE_RED,    saturation, strip.brightness));
	fill_solid(leds1 + 25, 12, CHSV(HUE_BLUE,   saturation, strip.brightness));

	fill_solid(leds2 + 0,  12, CHSV(HUE_PINK,   saturation, strip.brightness));
	fill_solid(leds2 + 12, 13, CHSV(HUE_AQUA,   saturation, strip.brightness));
	fill_solid(leds2 + 25, 12, CHSV(HUE_YELLOW, saturation, strip.brightness));

	// other candidate:  HUE_ORANGE, HUE_PURPLE
}

void stripEffectRainbow()
{
	fill_rainbow(leds1, nLed, hue, 255/nLed);
	fill_rainbow(leds2, nLed, hue, 255/nLed);
}

/* leds2 have reverse order */
void stripEffectRainbow2()
{
	fill_rainbow(leds1, nLed, hue, 255/nLed);
	fill_rainbow(leds2, nLed, (255-hue), 255/nLed);
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
	rainbowGlitter(strip.param2);
}

void stripEffectRainbowGlitter2()
{
	stripEffectRainbow2();
	rainbowGlitter(strip.param2);
}

void stripEffectConfetti()
{
	uint8_t pos = random8(nLed);

	fadeToBlackBy(leds1, nLed, 10);
	fadeToBlackBy(leds2, nLed, 10);

	leds1[pos] += CHSV(hue + random8(64), 200, 255);
	leds2[pos] += CHSV(hue + random8(64), 200, 255);
}

/* Payload (data) 
 * .----------------------------------------------------.
 * | mode | brightness | fps | param1 | param2 | param3 |
 * |------+------------+-----+--------|--------|--------|
 * |    1 |          1 |   1 |     1  |      1 |      1 |
 * '----------------------------------------------------'
 */
void bleStrip1(const byte *data, const byte sz)
{
	const byte stripSize = sizeof(stripValue);
	stripValue *dv = NULL;

	// sanity check
	if (sz != stripSize)   { return; }
	if (DEV_TYPE != STRIP) { return; }

	dumpPkt(data, sz);

	dv = (stripValue *)data;

	syslog("STRIP mode(%d) brightness(%d) fps(%d)", 
		dv->mode, dv->brightness, dv->fps);

	memcpy(&strip, data, stripSize);
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
	static byte hueFps = strip.param1;
	pktSerial.update();

	/*  30 fps -> 33.3 ms
	    60 fps -> 16.6 ms
	   120 fps ->  9.3 ms */
	if (strip.fps == 0)
		strip.fps = ledFpsDefault;

	/* if hueFps is zero, don't change hue (stop moving rainbow) */
	if (hueFps != 0)
	{
		EVERY_N_MILLISECONDS(1000/hueFps) { hue++; }
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
