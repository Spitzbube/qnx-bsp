

typedef struct
{
	uint32_t dwOutFunction; //0
	uint32_t dwInFunction; //4
	uint32_t bPin; //8

} GPIO_Params;


typedef struct
{
	uint8_t bData_0;
	int fill_4; //4
	int Data_8; //8
	//12
} Struct_20611068;

extern int gpio_init(void);
extern int gpio_open(GPIO_Params* pParams, Struct_20611068** r5);
