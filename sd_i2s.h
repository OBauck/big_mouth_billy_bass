
#ifndef SD_I2S_H
#define SD_I2S_H

void fatfs_init(void);
uint8_t fatfs_list_directory(char *dir_list, uint16_t dir_list_len, uint16_t *indexes, uint8_t indexes_len);

void i2s_init(void);
bool play_song(char *song);
void stream_song(void);
void stop_song(void);

void billy_motion_init(void);
void billy_head_toggle(void);
void billy_tail_out(void);
void billy_tail_in(void);
void billy_mouth_open(void);
void billy_mouth_close(void);

#endif //SD_I2S_H
