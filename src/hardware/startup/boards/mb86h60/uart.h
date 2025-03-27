
typedef struct
{
	uint32_t index; //0
	GPIO_Params txPin; //4
	GPIO_Params rxPin; //16
	//28?
} Uart_Init_Params;


typedef struct
{
	volatile unsigned int FREG_UART_DR; //0
	int fill_4[5]; //4
	volatile unsigned int FREG_UART_FR; //24
	volatile unsigned int fill_28[2]; //28
	volatile unsigned int FREG_UART_IBRD; //0x24
	volatile unsigned int FREG_UART_FBRD; //0x28
	volatile unsigned int FREG_UART_LCRH; //0x2c
	volatile unsigned int FREG_UART_CR; //0x30
	int fill_0x34; //0x34
	volatile unsigned int UART_IMSC; //0x38

} Uart_Regs;

typedef struct
{
	char inUse; //0
	Uart_Regs* regs; //4
	//8
} Uart_Module;

extern int uart_setup(void);
extern char uart_init(Uart_Init_Params* pParams, Uart_Module** ppModule);
extern int uart_write_byte(Uart_Module* r0, unsigned char ch);
extern unsigned char uart_read_byte(Uart_Module* r0);
