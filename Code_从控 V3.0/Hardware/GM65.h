#ifndef __GM65_H_
#define __GM65_H_


extern u8 recv_ok ;       //接收完成标志
extern u8 uart_buf[32];  //用于保存串口数据

unsigned GM65_GetData(unsigned char *array);

#endif

