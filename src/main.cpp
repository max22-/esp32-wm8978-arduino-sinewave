#include <Arduino.h>
#include "driver/i2s.h"
#include "WM8978.h"

// First : the boring stuff

#define I2C_SDA 19
#define I2C_SCL 18
#define I2S_BCK 33
#define I2S_WS 25
#define I2S_IN 27
#define I2S_OUT 26
#define I2S_PORT 0

#define SAMPLE_RATE 48000
#define BUFFER_SIZE 8
#define CHANNELS 2
#define BITS_PER_SAMPLE 32


int32_t buffer[CHANNELS*BUFFER_SIZE];


i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // high interrupt priority
    .dma_buf_count = 3,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false
};
   

i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_OUT,
    .data_in_num = I2S_IN
};


void setup() {
  Serial.begin(115200);

  WM8978 wm8978;
  wm8978.init();
  wm8978.addaCfg(1,1); 
  wm8978.inputCfg(1,0,0);     
  wm8978.outputCfg(1,0); 
  wm8978.micGain(30);
  wm8978.auxGain(0);
  wm8978.lineinGain(0);
  wm8978.spkVolSet(40);
  wm8978.hpVolSet(40,40);
  wm8978.i2sCfg(2,0);
  
  i2s_driver_install((i2s_port_t)0, &i2s_config, 0, nullptr);
  i2s_set_pin((i2s_port_t)0, &pin_config);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
  REG_WRITE(PIN_CTRL, 0xFFFFFFF0);

}

void loop() {

  // *************
  // Here comes the fun stuff : lets make some noise !
  // A basic sine wave, 440Hz.
  static double phase = 0;

  for(int i=0;i<BUFFER_SIZE;i++) {
    phase += 2 * PI * 440.0 / (double)SAMPLE_RATE;
    auto val = (pow(2, 31)-1) * sin(phase);   // the amplitude is pow(2, 31) - 1 because we use 32 bit SIGNED integers
    buffer[i*2] = (int32_t)val;     //  1st channel because we send stero
    buffer[i*2+1] = (int32_t)val;   //  2nd channel
                                    //  I don't know which one is left. Test yourself :)
  }

  // *************
  // Then we send the buffer to the WM8978
  size_t bytes_written = 0;
  i2s_write((i2s_port_t)0, &buffer, CHANNELS*(BITS_PER_SAMPLE/8)*BUFFER_SIZE, &bytes_written, portMAX_DELAY);

  // *************
  // And we print some stuff on the serial port, every second
  if(millis() % 1000 == 0)
    Serial.println(bytes_written);
}