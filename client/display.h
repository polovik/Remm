/*
 * display.h
 */
#ifndef DISPLAY_H_
#define DISPLAY_H_

#define DISPLAY_WINDOW_NAME "Remm_control"

int init_display();
void display_frame(unsigned char *data, unsigned int length);
void release_display();

#endif /* DISPLAY_H_ */
