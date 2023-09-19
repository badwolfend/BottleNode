#include "Arduino.h"
#include "driver/i2s.h"
#include "driver/gpio.h"

static const i2s_port_t i2s_mic = I2S_NUM_1;
static const i2s_port_t i2s_spkr = I2S_NUM_0;

static const i2s_config_t i2s_mic_config = {
	.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
	.sample_rate = 8000,
	.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
	.channel_format = I2S_CHANNEL_FMT_ALL_LEFT,
	.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
	.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,       // high interrupt priority
	.dma_buf_count = 8,
	.dma_buf_len = 256,
	.use_apll=0,
	.tx_desc_auto_clear= true, 
	.fixed_mclk=-1    
};

static const i2s_config_t i2s_spkr_config = {
	.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
	.sample_rate = 8000,
	.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
	.channel_format = I2S_CHANNEL_FMT_ALL_LEFT,
	.communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
	.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,       // high interrupt priority
	.dma_buf_count = 8,
	.dma_buf_len = 256,
	.use_apll=0,
	.tx_desc_auto_clear= true, 
	.fixed_mclk=-1    
};

static const i2s_pin_config_t i2s_spkr_pins = {
	.bck_io_num = 5,
	.ws_io_num = 4,
	.data_out_num = 6,
	.data_in_num = I2S_PIN_NO_CHANGE
};

static const i2s_pin_config_t i2s_mic_pins = {
	.bck_io_num = 1,
	.ws_io_num = 2,
	.data_out_num = I2S_PIN_NO_CHANGE,
	.data_in_num = 3
};

#define POWER_PIN ((gpio_num_t)10)
#define OUTPUT_ENABLE_PIN ((gpio_num_t)7)

void setup() {
	Serial.begin(115200);

	pinMode(OUTPUT_ENABLE_PIN, INPUT_PULLUP);
	digitalWrite(POWER_PIN, 0);
	gpio_set_drive_capability(POWER_PIN, GPIO_DRIVE_CAP_3);
	pinMode(POWER_PIN, OUTPUT);

	sleep(1);

	digitalWrite(POWER_PIN, 1);

	Serial.println("Starting I2S audio test");

	pinMode(i2s_mic_pins.data_in_num, INPUT_PULLDOWN);

	i2s_driver_install(i2s_spkr, &i2s_spkr_config, 0, NULL);
	i2s_set_pin(i2s_spkr, &i2s_spkr_pins);

	i2s_driver_install(i2s_mic, &i2s_mic_config, 0, NULL);
	i2s_set_pin(i2s_mic, &i2s_mic_pins);

	gpio_set_drive_capability((gpio_num_t)i2s_spkr_pins.bck_io_num, GPIO_DRIVE_CAP_3);
	gpio_set_drive_capability((gpio_num_t)i2s_spkr_pins.ws_io_num, GPIO_DRIVE_CAP_3);
	gpio_set_drive_capability((gpio_num_t)i2s_spkr_pins.data_out_num, GPIO_DRIVE_CAP_3);
}

uint32_t count = 0;
volatile int next_sample()
{
	static uint32_t count;
	int32_t amplitude = 0x04000000;
	float time = (float)(count++ & 0x000FFFFF) / (float)i2s_spkr_config.sample_rate;
	return (int)((float)amplitude * sin(time * 3.14 * 523.25 / 2));
	// return count++;
}

float output_attenuation = 1.0;
float alpha = 0.9995;

void reset_ramp()
{
	output_attenuation = 1.0;
}

int scale_output(int v)
{
	int u = (1.0-output_attenuation) * (float)v;
	output_attenuation *= alpha;
	return u;
}

#define BUFLEN 16

void loop()
{    
	size_t bytes_written, bytes_read;
	uint32_t input[2];
	int32_t buf[BUFLEN];
	size_t buf_num_read;

	// Serial.print("About to read samples... ");
	i2s_read(i2s_mic, buf, sizeof(buf), &bytes_read, portMAX_DELAY);

	// Serial.printf("Read %d bytes: %08lx %08lx %08lx %08lx ...\n", bytes_read, buf[0], buf[1], buf[2], buf[3]);

	for (int i = 0; i < BUFLEN; i++) {
		// buf[i] = scale_output(0.1*buf[i] + next_sample()/4);
		buf[i] = scale_output(buf[i] / 1);
	}

	if (digitalRead(OUTPUT_ENABLE_PIN))
		i2s_write(i2s_spkr, buf, sizeof(buf), &bytes_written, portMAX_DELAY); 
}
