
#include "ff.h"
#include "diskio_blkdev.h"
#include "nrf_block_dev_sdc.h"
#include "nrf_drv_i2s.h"
#include "nrf_delay.h"
#include "sd_i2s.h"
#include "boards.h"

#define NRF_LOG_MODULE_NAME "APP"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#ifdef BOARD_PCA10040

#define SDC_SCK_PIN     ARDUINO_13_PIN  ///< SDC serial clock (SCK) pin.
#define SDC_MOSI_PIN    ARDUINO_11_PIN  ///< SDC serial data in (DI) pin.
#define SDC_MISO_PIN    ARDUINO_12_PIN  ///< SDC serial data out (DO) pin.
#define SDC_CS_PIN      ARDUINO_10_PIN  ///< SDC chip select (CS) pin.

#define I2S_SCK			I2S_CONFIG_SCK_PIN
#define I2S_LRCLK		I2S_CONFIG_LRCK_PIN
#define I2S_SDOUT		I2S_CONFIG_SDOUT_PIN
#define I2S_SDIN		255
#define I2S_MCK			255

#endif

NRF_BLOCK_DEV_SDC_DEFINE(
        m_block_dev_sdc,
        NRF_BLOCK_DEV_SDC_CONFIG(
                SDC_SECTOR_SIZE,
                APP_SDCARD_CONFIG(SDC_MOSI_PIN, SDC_MISO_PIN, SDC_SCK_PIN, SDC_CS_PIN)
         ),
         NFR_BLOCK_DEV_INFO_CONFIG("Nordic", "SDC", "1.00")
);

#define BUFFERED_SAMPLES 256

#define FATFS_BUFFER_SIZE BUFFERED_SAMPLES*2

static uint8_t fatfs_buffer[FATFS_BUFFER_SIZE];
static bool fatfs_data_ready;
static bool i2s_data_sent;
static bool playing;

#define I2S_BUFFER_SIZE     BUFFERED_SAMPLES*2

static uint32_t m_i2s_buffer_rx[I2S_BUFFER_SIZE];
static uint32_t m_i2s_buffer_tx[I2S_BUFFER_SIZE];

static FATFS fs;
static FIL file;

static uint32_t count;
static bool close_file = false;

void fatfs_init(void)
{
    FRESULT ff_result;
    DSTATUS disk_state = STA_NOINIT;

    // Initialize FATFS disk I/O interface by providing the block device.
    static diskio_blkdev_t drives[] =
    {
            DISKIO_BLOCKDEV_CONFIG(NRF_BLOCKDEV_BASE_ADDR(m_block_dev_sdc, block_dev), NULL)
    };

    diskio_blockdev_register(drives, ARRAY_SIZE(drives));

    NRF_LOG_INFO("Initializing disk 0 (SDC)...\r\n");
    for (uint32_t retries = 3; retries && disk_state; --retries)
    {
        disk_state = disk_initialize(0);
    }
    if (disk_state)
    {
        NRF_LOG_INFO("Disk initialization failed.\r\n");
        return;
    }
    
    uint32_t blocks_per_mb = (1024uL * 1024uL) / m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_size;
    uint32_t capacity = m_block_dev_sdc.block_dev.p_ops->geometry(&m_block_dev_sdc.block_dev)->blk_count / blocks_per_mb;
    NRF_LOG_INFO("Capacity: %d MB\r\n", capacity);

    NRF_LOG_INFO("Mounting volume...\r\n");
    ff_result = f_mount(&fs, "", 1);
    if (ff_result)
    {
        NRF_LOG_INFO("Mount failed.\r\n");
        return;
    }
    return;
}

uint8_t fatfs_list_directory(char *dir_list, uint16_t dir_list_len, uint16_t *indexes, uint8_t indexes_len)
{
	FRESULT ff_result;
	static DIR dir;
    static FILINFO fno;
	static uint8_t index_count = 0;
	static uint16_t str_len;
	
    ff_result = f_opendir(&dir, "/music");
    if (ff_result)
    {
        NRF_LOG_INFO("Directory listing failed!\r\n");
        return 0;
    }
    
    do
    {
        ff_result = f_readdir(&dir, &fno);
        if (ff_result != FR_OK)
        {
            NRF_LOG_INFO("Directory read failed.");
            return 0;
        }
        
        if (fno.fname[0])
        {
            if (!(fno.fattrib & AM_DIR))
            {
				str_len = strlen(dir_list) + strlen(fno.fname) + 1;
				if(str_len >= dir_list_len)
				{
					NRF_LOG_INFO("No more space in directory list string\r\n");
					return 0;
				}
				strcat(dir_list, fno.fname);
				strcat(dir_list, "\n");
				
				if(index_count >= indexes_len)
				{
					NRF_LOG_INFO("No more space in indexes array\r\n");
					return 0;
				}
				
				indexes[index_count] = str_len;
				index_count++;
				
                //NRF_LOG_RAW_INFO("%s\r\n", (uint32_t)fno.fname);
            }
        }
    }
    while (fno.fname[0]);
	dir_list[strlen(dir_list)-1] = '\0';
	return index_count;
}

bool fatfs_read()
{
	FRESULT ff_result;
	uint32_t bytes_read;
	
	ff_result = f_read(&file, fatfs_buffer, sizeof(fatfs_buffer), (UINT*)&bytes_read);
	
	if(ff_result != FR_OK)
	{
		NRF_LOG_INFO("Failed to read file\n");
		return false;
	}
	else if(bytes_read < sizeof(fatfs_buffer))
	{
		NRF_LOG_INFO("End of file\n");
		return false;
	}		
	return true;
}

static void prepare_i2s_data(uint32_t * p_buffer, uint16_t number_of_words)
{
	uint16_t i;
	
	for (i = 0; i < number_of_words; i++)
	{
		uint16_t sample_lr = (int32_t)(fatfs_buffer[i*2] + (fatfs_buffer[i*2+1]<<8));
		
		((uint16_t *)p_buffer)[2 * i]     = sample_lr;
		((uint16_t *)p_buffer)[2 * i + 1] = sample_lr;
	}
}

// This is the I2S data handler - all data exchange related to the I2S transfers
// is done here.
static void data_handler(uint32_t const * p_data_received,
                         uint32_t       * p_data_to_send,
                         uint16_t         number_of_words)
{
	// Non-NULL value in 'p_data_to_send' indicates that the driver needs
    // a new portion of data to send.
    if (p_data_to_send != NULL)
    {
        //if data is not ready from sd card we need to stop the streaming
		if(!fatfs_data_ready && (count > 2))
		{
			close_file = true;
			return;
		}
		
		prepare_i2s_data(p_data_to_send, number_of_words);
		
		i2s_data_sent = true;
		fatfs_data_ready = false;
		count++;
    }
}

void i2s_init(void)
{
	uint32_t err_code;
	
	 nrf_drv_i2s_config_t config = NRF_DRV_I2S_DEFAULT_CONFIG;
    
	config.sdin_pin  = I2S_SDIN;
    config.sdout_pin = I2S_SDOUT;
	config.sck_pin = I2S_SCK;
	config.mck_pin = I2S_MCK;
	config.lrck_pin = I2S_LRCLK;
	
    config.mck_setup = NRF_I2S_MCK_32MDIV23;
    config.ratio     = NRF_I2S_RATIO_32X;
    config.channels  = NRF_I2S_CHANNELS_STEREO;
    err_code = nrf_drv_i2s_init(&config, data_handler);
    APP_ERROR_CHECK(err_code);
}

bool play_song(char *song)
{
	if(!playing)
	{
		uint8_t header[44];
		FRESULT ff_result;
		uint32_t err_code, bytes_read;
		char file_name[50] = "music/";
		
		strcat(file_name, song);
		//strcat(file_name, ".wav");
		
		ff_result = f_open(&file, file_name, FA_READ | FA_OPEN_EXISTING);
		
		if (ff_result != FR_OK)
		{
			NRF_LOG_INFO("Unable to open file %s.\r\n", (uint32_t)file_name);
			return playing;
		}

		//read header (44 bytes)
		ff_result = f_read(&file, header, sizeof(header), (UINT*)&bytes_read);
		
		if((bytes_read < sizeof(header)) || (ff_result != FR_OK))
		{
			NRF_LOG_INFO("Failed to wav header\n");
			return playing;
		}

		uint16_t channels = header[22] + (header[23]<<8);
		uint32_t sample_rate = header[24] + (header[25]<<8) + (header[26]<<16) + (header[27]<<24);
		uint16_t bits_per_sample = header[34] + (header[35]<<8);
		
		NRF_LOG_INFO("channels: %d\n", channels);
		NRF_LOG_INFO("sample rate: %d\n", sample_rate);
		NRF_LOG_INFO("bits per sample: %d\n", bits_per_sample);
		
		prepare_i2s_data(m_i2s_buffer_tx, I2S_BUFFER_SIZE);
		fatfs_data_ready = true;
		
		err_code = nrf_drv_i2s_start(m_i2s_buffer_rx, m_i2s_buffer_tx,
				I2S_BUFFER_SIZE, 0);
		APP_ERROR_CHECK(err_code);
		
		playing = true;
		
	}
	else
	{
		stop_song();
	}
	return playing;
}

void stop_song(void)
{
	if(playing)
	{
		nrf_drv_i2s_stop();
		playing = false;
		NRF_LOG_INFO("Playing stopped by user\r\n");
		
		close_file = true;
	}
}

void stream_song(void)
{
	FRESULT ff_result;
	if(playing)
    {
		if(i2s_data_sent)
		{
			i2s_data_sent = false;
			fatfs_data_ready = fatfs_read();
		}
    }
	if(close_file)
	{
		
		NRF_LOG_INFO("stopped i2s streaming, count: %u\n", count);
		nrf_drv_i2s_stop();
		playing = false;
		
		//close file
		ff_result = f_close(&file);
		if (ff_result != FR_OK)
		{
			NRF_LOG_INFO("Unable to close file.\r\n");
			return;
		}
		
			
		NRF_LOG_INFO("File closed\r\n");
		close_file = false;
		count = 0;
	}
}

void billy_motion_init(void)
{
	nrf_gpio_cfg_output(BILLY_HEAD_PIN);
	nrf_gpio_pin_clear(BILLY_HEAD_PIN);
	//nrf_gpio_pin_set(BILLY_HEAD_PIN);
	
	nrf_gpio_cfg_output(BILLY_TAIL_PIN);
	nrf_gpio_pin_clear(BILLY_TAIL_PIN);
	//nrf_gpio_pin_set(BILLY_TAIL_PIN);
	
	nrf_gpio_cfg_output(BILLY_MOUTH_PIN);
	nrf_gpio_pin_clear(BILLY_MOUTH_PIN);
	//nrf_gpio_pin_set(BILLY_MOUTH_PIN);
}

void billy_head_toggle(void)
{
	nrf_gpio_pin_toggle(BILLY_HEAD_PIN);
}

void billy_tail_out(void)
{
	nrf_gpio_pin_set(BILLY_TAIL_PIN);
}

void billy_tail_in(void)
{
	nrf_gpio_pin_clear(BILLY_TAIL_PIN);
}

void billy_mouth_open(void)
{
	nrf_gpio_pin_set(BILLY_MOUTH_PIN);
}

void billy_mouth_close(void)
{
	nrf_gpio_pin_clear(BILLY_MOUTH_PIN);
}
