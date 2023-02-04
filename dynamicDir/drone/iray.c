
#include "iray.h"
#include "app-serial.h"

/////////////////////////////////////////////////////////
//	   Function : serial_calc_checksum
//	Description : 计算通信帧的checksum
//		  Input :
//		 Output :
U8 serial_calc_checksum(U8 * dat, int len)
{
	U8				crc;

	crc = 0;

	while (len-- > 0)
		crc += *dat++;

	return crc;
}


//serial 的端口号需要更改
void LGC6122_send_data(U8 * buffer, int len)
{
	SERIAL_SEND_DATA(4, buffer, len);
}


/////////////////////////////////////////////////////////
//	   Function : weapon_LGC6122_adjust(U8 flag)
//	Description : 红外控制协议：红外校正
//		  Input : mode: 校正模式
//		 Output :
void LGC6122_adjust(U8 mode)
{
	U8 buffer[9] =  {0};

	printf("---set ir = %d\n", mode);
	buffer[0]			= 0xAA;
	buffer[1]			= 0x05;
	buffer[2]			= 0x01;
	buffer[3]			= 0x11;
	buffer[4]			= 0x02;
	buffer[5]			= mode;
	buffer[6]			= mode == 0x00 ? 0xc3: 0xc4; //背景：C3，快门(c4):mode =0x01
	buffer[7]			= 0xEB;
	buffer[8]			= 0xAA;
	LGC6122_send_data(buffer, sizeof(buffer));
}


/////////////////////////////////////////////////////////
//	   Function : weapon_LGC6122_bright
//	Description : 红外亮度 10字节，操作字0x23
//		  Input : level：亮度值
//		 Output :
void LGC6122_bright(short level)
{
	U8 buffer[10]={0};

	buffer[0]			= 0xAA;
	buffer[1]			= 0x06;
	buffer[2]			= 0x01;
	buffer[3]			= 0x23;
	buffer[4]			= 0x01;
	memcpy(buffer + 5, &level, 2);
	buffer[7]			= serial_calc_checksum(buffer, 7);
	buffer[8]			= 0xEB;
	buffer[9]			= 0xAA;

	LGC6122_send_data(buffer, sizeof(buffer));
}


////////////////////////////////////////////////////////
//	   Function : weapon_LGC6122_operation_word_0x01
//	Description : 红外控制协议：操作字:0x01，命令字0：0x01、
//		  Input : cmd:命令字1，level:参数值
//		 Output : 无
void LGC6122_operation_word_0x01(U8 cmd, U8 level)
{
	U8 buffer[9] = {0};
	buffer[0]			= 0xAA;
	buffer[1]			= 0x05;
	buffer[2]			= 0x01; 					// command word 0:
	buffer[3]			= cmd;						// command word 1:
	buffer[4]			= 0x01; 					//operation word = 0x01:
	buffer[5]			= level;
	buffer[6]			= serial_calc_checksum(buffer, 6);
	printf("image_level = %x, crc = %x\n", level, buffer[6]);
	buffer[7]			= 0xEB;
	buffer[8]			= 0xAA;

	LGC6122_send_data(buffer, sizeof(buffer));
}


/////////////////////////////////////////////////////////
//	   Function : weapon_LGC6122_operation_word_0x02
//	Description : 红外控制协议：操作字:0x02，命令字0：0x01、
//		  Input : cmd:命令字1，level:参数值
//		 Output : 无
void LGC6122_operation_word_0x02(U8 cmd1, U8 level)
{
	U8 buffer[9] = {0};
	buffer[0]			= 0xAA;
	buffer[1]			= 0x05;
	buffer[2]			= 0x01; 					// command word 0:
	buffer[3]			= cmd1; 					// command word 1:
	buffer[4]			= 0x02; 					//operation word = 0x02;
	buffer[5]			= level;
	buffer[6]			= serial_calc_checksum(buffer, 6);
	printf("image_level = %x, crc = %x\n", level, buffer[6]);
	buffer[7]			= 0xEB;
	buffer[8]			= 0xAA;

	LGC6122_send_data(buffer, sizeof(buffer));
}


/////////////////////////////////////////////////////////
//	   Function : serial_ir_data_loop
//	Description :
//		  Input :
//		 Output :
void LGC6122_ir_do_serial_protocol( U8* p )
{
    int len= 0;
    len = sizeof( p );
    switch( p[2] )
    {
    case CORRECT_RESPONE:
        printf("-------CORRECT_RESPONE---------\n");
        break;
    case POLARITY_RESPONE:
        printf("-------POLARITY_RESPONE---------\n");
        break;
    case ZOOM_RESPONE:
        printf("-------ZOOM_RESPONE---------\n");
        break;
    case AUTOCHECK_RESPONE:
        printf("------AUTOCHECK_RESPONE----------\n");
        break;
    case 0x1f:
        printf("------switch agc mode ----------\n");
        break;
    default:
        break;
    }
    for(int i = 0; i < len; i++)
        printf("%x-",p[i]);
    printf("\r\n");

}

/////////////////////////////////////////////////////////
//	   Function : LGC6122_black_white(U8 flag)
//	Description : 红外控制协议：色板模式
//		  Input : flag：0x00（白热），0x01（黑热）
//		 Output :
void LGC6122_black_white(U8 flag)
{
	U8 buffer[9]={0};

	printf("---set ir = %d\n", flag);
	buffer[0]			= 0xAA;
	buffer[1]			= 0x05;
	buffer[2]			= 0x01;
	buffer[3]			= 0x42;
	buffer[4]			= 0x02;
	buffer[5]			= flag;
	buffer[6]			= flag == 0x00 ? 0xf4: 0xf5;
	buffer[7]			= 0xEB;
	buffer[8]			= 0xAA;

	LGC6122_send_data(buffer, sizeof(buffer));
}


void LGC6122_set_zoom(int zoom)
{
	short			x, y;
	U8 buffer[16] = {0};

	printf("---set ir zoom = %d\n", zoom);

	if (zoom <= 0)
		return;

	buffer[0]			= 0xAA;
	buffer[1]			= 0x0c;
	buffer[2]			= 0x01;
	buffer[3]			= 0x40;
	buffer[4]			= 0x02;
	x					= (short) (640 / 2 - 640 / (2 * zoom));
	y					= (short) (512 / 2 - 512 / (2 * zoom));
	printf("left_up[x,y] = (%d,%d)\n", x, y);
	memcpy(buffer + 5, (U8 *) &x, 2);
	memcpy(buffer + 7, (U8 *) &y, 2);
	printf("left_up1[x1,y1] = (%x,%x),left_up2[x2,y2] = (%x,%x)\n", buffer[5], buffer[6], buffer[7], buffer[8]);
	x					= (short) (640 / 2 + (640 - 1) / (2 * zoom));
	y					= (short) (512 / 2 + (512 - 1) / (2 * zoom));
	printf("right_down[x,y] = (%d,%d)\n", x, y);
	memcpy(buffer + 9, (U8 *) &x, 2);
	memcpy(buffer + 11, (U8 *) &y, 2);
	printf("right_down1[x1,y1] = (%x,%x),right_down2[x2,y2] = (%x,%x)\n", buffer[9], buffer[10], buffer[11], buffer[12]);
	buffer[13]			= serial_calc_checksum(buffer, 13);
	printf("crc = (%x)\n", buffer[13]);
	buffer[14]			= 0xEB;
	buffer[15]			= 0xAA;
	LGC6122_send_data(buffer, sizeof(buffer));


}


