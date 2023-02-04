#ifndef _IRAY_H_
#define _IRAY_H_

#include "stdio.h"
#include "sys-service.h"
#include "sys-core.h"
#include "string.h"

void LGC6122_adjust(U8 mode);
void LGC6122_bright(short level);
void LGC6122_operation_word_0x01(U8 cmd,U8 level);
void LGC6122_operation_word_0x02(U8 cmd1,U8 level);
void LGC6122_black_white(U8 flag);
void weapon_LGC6122_zoom(int zoom);
void LGC6122_ir_do_serial_protocol(U8 * p);



#define  CORRECT_RESPONE      0x33
#define  POLARITY_RESPONE     0x2D
#define  ZOOM_RESPONE         0x2A
#define  AUTOCHECK_RESPONE    0x00

#endif
