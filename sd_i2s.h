
#ifndef SD_I2S_H
#define SD_I2S_H

void fatfs_init(void);
uint8_t fatfs_list_directory(char *dir_list, uint16_t dir_list_len, uint16_t *indexes, uint8_t indexes_len);

void i2s_init(void);
bool play_song(char *song);
void stream_song(void);
void stop_song(void);

#endif //SD_I2S_H
