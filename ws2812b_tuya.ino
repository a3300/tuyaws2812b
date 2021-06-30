/*
 Name:		ws2812b_tuya.ino
 Created:	2021/6/28 22:21:49
 Author:	jb
*/
#include <TuyaWifi.h>
#include <SoftwareSerial.h>

#include <FastLED.h>
#include <stdlib.h>
#define DebugSerial Serial

//����LED������
#define DATA_PIN D1		

/* ������ť */
int key_pin = D2;

SoftwareSerial SoftSerial(D5/*RX*/,D6/*TX*/);

// Define the array of leds
CRGB * leds;




TuyaWifi my_device(&SoftSerial);

/* Current LED status */
unsigned char led_state = 0;


/*���ݵ㶨�� */
#define DPID_SWITCH_LED 20	//����DP��
#define DPID_WORK_MODE 21	//����ģʽDP��
#define DPID_BRIGHT_VALUE 22	//����DP��
#define DPID_COLOUR_DATA 24		//��ɫDP��
#define DPID_DREAMLIGHT_SCENE_MODE 51	//ר�����ڻòʵƴ�����
#define DPID_LIGHTPIXEL_NUMBER_SET 53	//�òʵƴ��ü�֮���������ó���


/* Stores all DPs and their types. PS: array[][0]:dpid, array[][1]:dp type.
 *                                     dp type(TuyaDefs.h) : DP_TYPE_RAW, DP_TYPE_BOOL, DP_TYPE_VALUE, DP_TYPE_STRING, DP_TYPE_ENUM, DP_TYPE_BITMAP
*/
unsigned char dp_array[][2] =
{
	{DPID_SWITCH_LED,				DP_TYPE_BOOL},
	{DPID_WORK_MODE,				DP_TYPE_ENUM},
	{DPID_BRIGHT_VALUE,				DP_TYPE_VALUE},
	{DPID_COLOUR_DATA,				DP_TYPE_STRING},
	{DPID_DREAMLIGHT_SCENE_MODE,	DP_TYPE_RAW},
	{DPID_LIGHTPIXEL_NUMBER_SET,	DP_TYPE_VALUE}
};

/*DP�����*/
bool switch_led_value = 0;	//����DP���ֵ

enum WORK_MODE
{
	white,colour,scene
}work_mode_value;	//����ģʽ ö��ֵ: white, colour, scene

int bright_value=20;	//���� ��ֵ��Χ: 0-255, ���: 1, ����: 0, ��λ:

unsigned char colour_data[13] = { 0 };
CHSV colour_value;	//	�ʹ� //���ͣ��ַ���Value: 000011112222��
								//0000��H��ɫ�ȣ�0 - 360��0X0000 - 0X0168����
								//1111��S(���ͣ�0 - 1000, 0X0000 - 0X03E8����	
								//2222��V(���ȣ�0 - 1000��0X0000 - 0X03E8)
unsigned char* scene_mode;	//�ò��龰 ͸���ͣ�Raw��ר�����ڻòʵƴ�����
int led_number_set=17;		//���������� 





unsigned char pid[] = { "avwj3awxulnrup4a" };
unsigned char mcu_ver[] = { "1.0.0" };

/* last time */
unsigned long last_time = 10;




// the setup function runs once when you press reset or power the board
void setup() {

	SoftSerial.begin(9600);
	Serial.begin(9600);
	//��ʼ��ָʾ��, turn off led.
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	//��ʼ��������ť.
	pinMode(key_pin, INPUT_PULLUP);

	my_device.init(pid, mcu_ver);
	//��������DP�����������飬DP����
	my_device.set_dp_cmd_total(dp_array, 6);
	//ע��DP���ػص�����
	my_device.dp_process_func_register(dp_process);
	//ע���ϴ�����DP��ص�����
	my_device.dp_update_all_func_register(dp_update_all);

	leds = new CRGB[led_number_set];
	DebugSerial.println(sizeof(leds));
	FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, led_number_set);

	
	last_time = millis();
}

// the loop function runs over and over again until power down or reset
void loop() {
	
	my_device.uart_service();

	//���°�ť��������ģʽ.
	if (digitalRead(key_pin) == LOW) {
		delay(80);
		if (digitalRead(key_pin) == LOW) {
			my_device.mcu_set_wifi_mode(SMART_CONFIG);
		}
	}

	/* LED��˸������ģ������״̬ */
	switch (my_device.mcu_get_wifi_work_state())
	{
	case SMART_AND_AP_STATE:
		//����2������ģʽ
	case SMART_CONFIG_STATE:
		//���� Smart ����״̬���� LED ����
		if (millis() - last_time >= 250) {
			last_time = millis();

			if (led_state == LOW) {
				led_state = HIGH;
			}
			else {
				led_state = LOW;
			}
			digitalWrite(LED_BUILTIN, led_state);
		}
		break;
	case AP_STATE:
		//���� AP ����״̬���� LED ����

		break;
	case WIFI_LOW_POWER:
		//�͹���ģʽ
	case WIFI_NOT_CONNECTED:
		//Wi-Fi ������ɣ���������·�������� LED ����
		digitalWrite(LED_BUILTIN, HIGH);
		break;
	case WIFI_CONN_CLOUD:
		//������Ϳѻ��
	case WIFI_CONNECTED:
		//·�������ӳɹ����� LED ����
		digitalWrite(LED_BUILTIN, LOW);
		break;
	default:break;
	}


	delay(10);
}




/**
 * @description: DP download callback function.
 * @param {unsigned char} dpid
 * @param {const unsigned char} value
 * @param {unsigned short} length
 * @return {unsigned char}
 */
unsigned char dp_process(unsigned char dpid, const unsigned char value[], unsigned short length)
{
	
	/*	{DPID_SWITCH_LED,				DP_TYPE_BOOL},1
	{DPID_WORK_MODE,				DP_TYPE_ENUM},1
	{DPID_BRIGHT_VALUE,				DP_TYPE_VALUE},1
	{DPID_COLOUR_DATA,				DP_TYPE_STRING},
	{DPID_DREAMLIGHT_SCENE_MODE,	DP_TYPE_RAW},
	{DPID_LIGHTPIXEL_NUMBER_SET,	DP_TYPE_VALUE}1
	
	bool switch_led_value;	//����DP���ֵ

	work_mode_value;	//����ģʽ ö��ֵ: white, colour, scene

	int bright_value;	//���� ��ֵ��Χ: 10-1000, ���: 1, ����: 0, ��λ:
	unsigned char* colour_data;	//	�ʹ� //���ͣ��ַ���Value: 000011112222��
									//0000��H��ɫ�ȣ�0 - 360��0X0000 - 0X0168����
									//1111��S(���ͣ�0 - 1000, 0X0000 - 0X03E8����	
									//2222��V(���ȣ�0 - 1000��0X0000 - 0X03E8)
	unsigned char* scene_mode;	//�ò��龰 ͸���ͣ�Raw��ר�����ڻòʵƴ�����
	int led_number_set = 17;		//���������� */
	switch (dpid) {
	//LED����
	case DPID_SWITCH_LED:	
		switch_led_value = my_device.mcu_get_dp_download_data(dpid, value, length);
		DebugSerial.print("switch SET:");
		DebugSerial.println(switch_led_value);
		if (switch_led_value) {
			//if(work_mode_value )
			//Turn on 
			for (int i = 0; i < led_number_set; i++) {
				leds[i] = CRGB(20, 20, 20);
			}
			FastLED.show();
		}
		else {
			//Turn off
			for (int i = 0; i < led_number_set; i++) {
				leds[i] = CRGB(0, 0, 0);
			}
			FastLED.show();
		}
		//Status changes should be reported.
		my_device.mcu_dp_update(dpid, switch_led_value, 1);
		break;

	//����ģʽ�޸�
	case DPID_WORK_MODE:	
		work_mode_value = WORK_MODE( my_device.mcu_get_dp_download_data(dpid, value, length));


		my_device.mcu_dp_update(dpid, work_mode_value, 1);
		break;

	//���ȵ���
	case DPID_BRIGHT_VALUE:	
		bright_value = my_device.mcu_get_dp_download_data(dpid, value, length);
		if (work_mode_value == white) {	//��ɫģʽ����
			DebugSerial.print("bright SET:");
			DebugSerial.println(bright_value);
			DebugSerial.print("leds=");
			DebugSerial.print(leds[0].r); DebugSerial.print(" ");
			DebugSerial.print(leds[0].g); DebugSerial.print(" ");
			DebugSerial.print(leds[0].b); DebugSerial.print(" ");
			DebugSerial.print("to ");
			DebugSerial.print(bright_value); DebugSerial.print(" ");
			DebugSerial.print(bright_value); DebugSerial.print(" ");
			DebugSerial.println(bright_value);

			for (int i = 0; i < led_number_set; i++) {
				leds[i] = CRGB(bright_value, bright_value, bright_value);
			}
		}
		else if (work_mode_value == colour){	//��ɫģʽ����
			CRGB c;
			//������ֵ����HSV�С�
			colour_value.v = bright_value;
			//ת��rgb
			hsv2rgb_rainbow(colour_value, c);
			for (int i = 0; i < led_number_set; i++) {
				leds[i]=c;
			}
		}
		FastLED.show();
		my_device.mcu_dp_update(dpid, bright_value, 1);
		break;

	//����LED����
	case DPID_LIGHTPIXEL_NUMBER_SET:	
		led_number_set = my_device.mcu_get_dp_download_data(dpid, value, length);
		delete[]leds;
		leds = new CRGB[led_number_set];
		my_device.mcu_dp_update(dpid, led_number_set, 1);
		break;

	//��ɫ,�ַ��ͣ����д���
	case DPID_COLOUR_DATA: {
		char temp_char1[5] = { 0 };
		char temp_char2[5] = { 0 };
		char temp_char3[5] = { 0 };
		int temp1, temp2, temp3;
		CRGB rgb_temp;

		strlcpy((char*)colour_data, (const char*)value, length);

		DebugSerial.print("colourLength:");
		DebugSerial.print(length);
		DebugSerial.print("   colourData:");

		//ֻ����Hɫ����S���ͣ����ȴ������ǹ̶�ֵ����Ҫ��һ���������ȿؼ�
		strlcpy(temp_char1, (const char*)colour_data, 5);
		strlcpy(temp_char2, (const char*)(&colour_data[4]), 5);

		DebugSerial.print(temp_char1); DebugSerial.print(" ");
		DebugSerial.print(temp_char2); DebugSerial.print(" ");
		DebugSerial.print(temp_char3);
		DebugSerial.println("");

		temp1 = htoi(temp_char1);
		temp2 = htoi(temp_char2);

		temp1 = map(temp1, 0, 360, 0, 255);
		temp2 = map(temp2, 0, 1000, 0, 255);

		//��hsvת��Ϊrgbɫ��
		colour_value = CHSV(temp1, temp2, bright_value);
		hsv2rgb_rainbow(colour_value, rgb_temp);

		for (int i = 0; i < led_number_set; i++) {
			leds[i] = rgb_temp;
		}

		FastLED.show();

		my_device.mcu_dp_update(dpid, colour_data, length);

		DebugSerial.print("str to hsv=");
		DebugSerial.print(colour_value.h); DebugSerial.print(" ");
		DebugSerial.print(colour_value.s); DebugSerial.print(" ");
		DebugSerial.print(colour_value.v); DebugSerial.print(" ");
		DebugSerial.print("  hsv to rgb=");
		DebugSerial.print(rgb_temp.r);		DebugSerial.print(" ");
		DebugSerial.print(rgb_temp.g);		DebugSerial.print(" ");
		DebugSerial.println(rgb_temp.b);	DebugSerial.println(" ");

	}
		break;

	case DPID_DREAMLIGHT_SCENE_MODE:	//����ģʽ��͸���ͣ����д���

		break;
	default:break;
	}
	return SUCCESS;
}


/**
* ���ص�ǰ�豸������DP״̬��
 * @description: Upload all DP status of the current device.
 * @param {*}
 * @return {*}
 * 
 */
void dp_update_all(void)
{ /*{DPID_SWITCH_LED, DP_TYPE_BOOL},
	{ DPID_WORK_MODE,				DP_TYPE_ENUM },
	{ DPID_BRIGHT_VALUE,				DP_TYPE_VALUE },
	{ DPID_COLOUR_DATA,				DP_TYPE_STRING },
	{ DPID_DREAMLIGHT_SCENE_MODE,	DP_TYPE_RAW },
	{ DPID_LIGHTPIXEL_NUMBER_SET,	DP_TYPE_VALUE }
	*/
	my_device.mcu_dp_update(DPID_SWITCH_LED, led_state, 1);
	my_device.mcu_dp_update(DPID_WORK_MODE, work_mode_value, 1);
	my_device.mcu_dp_update(DPID_BRIGHT_VALUE, bright_value, 1);
	my_device.mcu_dp_update(DPID_COLOUR_DATA, colour_data,sizeof(colour_data));
	my_device.mcu_dp_update(DPID_DREAMLIGHT_SCENE_MODE, scene_mode,sizeof(scene_mode));
	my_device.mcu_dp_update(DPID_LIGHTPIXEL_NUMBER_SET, led_number_set,1);
}

/*
* ��16�����ַ���ת��Ϊ����
* ֻ����4λ��16������
* @param {str:��Ҫת�����ַ���}
* @return {ת������}
*/
int htoi(char* s) {
	int totalNum = strlen(s);
	DebugSerial.print("s length:");
	DebugSerial.println(totalNum);
	DebugSerial.print("s=");
	DebugSerial.println(s);
	//int 
	int c;
	//int position;
	int lastChar;
	int num = 0;
	
	for(int position = 0; position <4; position++){
		c = s[position];
		DebugSerial.print("ctotalNum=");
		DebugSerial.print(totalNum);
		DebugSerial.print(" c=");
		DebugSerial.print(c);
		//��������4λ
		num = num * 16;
		//Сдת��д
		if (c >= 'a' && c <= 'z') {
			c -= 'a' - 'A';
		}
		DebugSerial.print(" CAPS=");
		DebugSerial.print(c);

		// ��дA��Fת����10-15,�����ַ�ת����0-9
		if (c >= 'A' && c <= 'F') {
			num += c - 'A' + 10;
		}
		else {
			num += c - '0';
		}
		DebugSerial.print(" position:");
		DebugSerial.print(position);
		DebugSerial.print(" num:");
		DebugSerial.println(num);
	}

	return num;
}