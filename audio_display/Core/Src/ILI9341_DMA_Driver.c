//--------------------------------------------
//	ILI9341 DMA Driver library for STM32 HAL
//	https://github.com/yuujiin/STM32_ILI9341_DMA_Driver
//--------------------------------------------


/* Includes ------------------------------------------------------------------*/
#include "ILI9341_DMA_Driver.h"

/* Local Variables ----------------------------------------------------------*/
static uint16_t Block_Width  = ILI_SCREEN_WIDTH;
static uint16_t Block_Height = ILI_SCREEN_HEIGHT;
static volatile uint32_t  DMA_SizeRemaining = 0;
static volatile uint16_t *DMA_BufRemaining;
static volatile uint16_t  DMA_BufFill __ALIGNED(32)	= BLACK;	// Aligned for DCache
static volatile uint8_t	  DMA_Busy = 0;

/* HARDWARE RESET */
void ILI_Reset(void) {
	HAL_GPIO_WritePin(ILI_RST_PORT, ILI_RST_PIN, GPIO_PIN_RESET);
	HAL_Delay(200);
	HAL_GPIO_WritePin(ILI_CS_PORT, ILI_CS_PIN, GPIO_PIN_SET);
	HAL_Delay(200);
	HAL_GPIO_WritePin(ILI_RST_PORT, ILI_RST_PIN, GPIO_PIN_SET);
}

/* Enable LCD display */
void ILI_Enable(void) {
	HAL_GPIO_WritePin(ILI_RST_PORT, ILI_RST_PIN, GPIO_PIN_SET);
}

/* Send command/data to LCD
 * (internal function) */
static inline HAL_StatusTypeDef ILI_SPI_Write(uint8_t Data, GPIO_PinState DC_PinState) {
	if (DMA_Busy) return HAL_BUSY;
	HAL_StatusTypeDef result;
	HAL_GPIO_WritePin(ILI_CS_PORT, ILI_CS_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(ILI_DC_PORT, ILI_DC_PIN, DC_PinState);
	result = HAL_SPI_Transmit(&ILI_SPI_HANDLE, &Data, 1, 1);
	HAL_GPIO_WritePin(ILI_CS_PORT, ILI_CS_PIN, GPIO_PIN_SET);
	return result;
}

/* Send command (char) to LCD */
HAL_StatusTypeDef ILI_Write_Command(uint8_t Command) {
	return ILI_SPI_Write(Command, GPIO_PIN_RESET);
}

/* Send data (char) to LCD */
HAL_StatusTypeDef ILI_Write_Data(uint8_t Data) {
	return ILI_SPI_Write(Data, GPIO_PIN_SET);
}

/* Set Address - Location block - to draw into
 * X, Y - start coordinates; W, H - block width and height */
void ILI_Set_Address(uint16_t X, uint16_t Y, uint16_t W, uint16_t H) {
	uint16_t X2 = X + W - 1;
	uint16_t Y2 = Y + H - 1;

	Block_Width  = W;
	Block_Height = H;

	ILI_Write_Command(0x2A);
	ILI_Write_Data(X>>8);
	ILI_Write_Data(X);
	ILI_Write_Data(X2>>8);
	ILI_Write_Data(X2);

	ILI_Write_Command(0x2B);
	ILI_Write_Data(Y>>8);
	ILI_Write_Data(Y);
	ILI_Write_Data(Y2>>8);
	ILI_Write_Data(Y2);

	ILI_Write_Command(0x2C);
}

/* Set rotation of the screen
 * Use ILI_Set_Address to set correct Width and Height afterwards */
void ILI_Set_Rotation(uint8_t Rotation) {
	uint8_t rotation_data;

	switch(Rotation) {
	case SCREEN_VERTICAL_1:
		rotation_data = 0x40|0x08;
		break;
	case SCREEN_HORIZONTAL_1:
		rotation_data = 0x20|0x08;
		break;
	case SCREEN_VERTICAL_2:
		rotation_data = 0x80|0x08;
		break;
	case SCREEN_HORIZONTAL_2:
		rotation_data = 0x40|0x80|0x20|0x08;
		break;
	default:
		//EXIT IF SCREEN ROTATION NOT VALID!
		break;
	}

	ILI_Write_Command(0x36);
	ILI_Write_Data(rotation_data);
}

/* Initialize LCD */
void ILI_Init(void)
{
	ILI_Enable();

	// SOFTWARE RESET
	ILI_Write_Command(0x01);
	HAL_Delay(20);	// min 5ms before next cmd

	// POWER CONTROL A
	ILI_Write_Command(0xCB);
	ILI_Write_Data(0x39);
	ILI_Write_Data(0x2C);
	ILI_Write_Data(0x00);
	ILI_Write_Data(0x34);
	ILI_Write_Data(0x02);

	// POWER CONTROL B
	ILI_Write_Command(0xCF);
	ILI_Write_Data(0x00);
	ILI_Write_Data(0xC1);
	ILI_Write_Data(0x30);

	// DRIVER TIMING CONTROL A
	ILI_Write_Command(0xE8);
	ILI_Write_Data(0x85);
	ILI_Write_Data(0x00);
	ILI_Write_Data(0x78);

	// DRIVER TIMING CONTROL B
	ILI_Write_Command(0xEA);
	ILI_Write_Data(0x00);
	ILI_Write_Data(0x00);

	// POWER ON SEQUENCE CONTROL
	ILI_Write_Command(0xED);
	ILI_Write_Data(0x64);
	ILI_Write_Data(0x03);
	ILI_Write_Data(0x12);
	ILI_Write_Data(0x81);

	// PUMP RATIO CONTROL
	ILI_Write_Command(0xF7);
	ILI_Write_Data(0x20);

	// POWER CONTROL,VRH[5:0]
	ILI_Write_Command(0xC0);
	ILI_Write_Data(0x23);

	// POWER CONTROL,SAP[2:0];BT[3:0]
	ILI_Write_Command(0xC1);
	ILI_Write_Data(0x10);

	// VCM CONTROL
	ILI_Write_Command(0xC5);
	ILI_Write_Data(0x3E);
	ILI_Write_Data(0x28);

	// VCM CONTROL 2
	ILI_Write_Command(0xC7);
	ILI_Write_Data(0x86);

	// MEMORY ACCESS CONTROL
	ILI_Write_Command(0x36);
	ILI_Write_Data(0x48);

	// PIXEL FORMAT
	ILI_Write_Command(0x3A);
	ILI_Write_Data(0x55);

	// FRAME RATIO CONTROL, STANDARD RGB COLOR
	ILI_Write_Command(0xB1);
	ILI_Write_Data(0x00);
	ILI_Write_Data(0x18);

	// DISPLAY FUNCTION CONTROL
	ILI_Write_Command(0xB6);
	ILI_Write_Data(0x08);
	ILI_Write_Data(0x82);
	ILI_Write_Data(0x27);

	// 3GAMMA FUNCTION DISABLE
	ILI_Write_Command(0xF2);
	ILI_Write_Data(0x00);

	// GAMMA CURVE SELECTED
	ILI_Write_Command(0x26);
	ILI_Write_Data(0x01);

	// POSITIVE GAMMA CORRECTION
	ILI_Write_Command(0xE0);
	ILI_Write_Data(0x0F);
	ILI_Write_Data(0x31);
	ILI_Write_Data(0x2B);
	ILI_Write_Data(0x0C);
	ILI_Write_Data(0x0E);
	ILI_Write_Data(0x08);
	ILI_Write_Data(0x4E);
	ILI_Write_Data(0xF1);
	ILI_Write_Data(0x37);
	ILI_Write_Data(0x07);
	ILI_Write_Data(0x10);
	ILI_Write_Data(0x03);
	ILI_Write_Data(0x0E);
	ILI_Write_Data(0x09);
	ILI_Write_Data(0x00);

	// NEGATIVE GAMMA CORRECTION
	ILI_Write_Command(0xE1);
	ILI_Write_Data(0x00);
	ILI_Write_Data(0x0E);
	ILI_Write_Data(0x14);
	ILI_Write_Data(0x03);
	ILI_Write_Data(0x11);
	ILI_Write_Data(0x07);
	ILI_Write_Data(0x31);
	ILI_Write_Data(0xC1);
	ILI_Write_Data(0x48);
	ILI_Write_Data(0x08);
	ILI_Write_Data(0x0F);
	ILI_Write_Data(0x0C);
	ILI_Write_Data(0x31);
	ILI_Write_Data(0x36);
	ILI_Write_Data(0x0F);

	// EXIT SLEEP
	ILI_Write_Command(0x11);
	HAL_Delay(120);	// min 120ms

	// TURN ON DISPLAY
	ILI_Write_Command(0x29);

	// STARTING ROTATION & ADDRESS
	ILI_Set_Rotation(ILI_ROTATION);
	ILI_Set_Address(0, 0, ILI_SCREEN_WIDTH, ILI_SCREEN_HEIGHT);
}

/* DMA Transfer functions -------------------------------- */
/* Switch SPI data size
 * (internal function)
 * Generates preprocessor error if not implemented for your target platform */
static inline void ILI_SPI_SetDataSize(uint32_t datasize) {
	if (ILI_SPI_HANDLE.Init.DataSize != datasize) {
		ILI_SPI_HANDLE.Init.DataSize =  datasize;
		/* Platform specific implementation: */
#ifdef STM32H7	// Tested on STM32H743 & STM32H753
		/* SPI_CFG1 reg, bits 4:0 DSIZE[4:0]
		 * 00111: 8-bit / 01111: 16-bit */
		CLEAR_BIT(ILI_SPI_HANDLE.Instance->CFG1, SPI_DATASIZE_32BIT);
		SET_BIT  (ILI_SPI_HANDLE.Instance->CFG1, datasize);
#elif defined STM32F7
		/* SPI_CR2 reg, bits 11:8 DS[3:0]
		 * 0111: 8-bit / 1111: 16-bit */
		CLEAR_BIT(ILI_SPI_HANDLE.Instance->CR2, SPI_DATASIZE_16BIT);
		SET_BIT  (ILI_SPI_HANDLE.Instance->CR2, datasize);
#elif defined STM32F4
		/* SPI_CR1 reg, bit 11 DFF, 0: 8-bit / 1: 16-bit
		 * This bit should be written only when SPI is disabled (SPE = ‘0’) for correct operation.
		 * SPI_CR1, bit 6 SPE */
		CLEAR_BIT(ILI_SPI_HANDLE.Instance->CR1, SPI_CR1_SPE);
		CLEAR_BIT(ILI_SPI_HANDLE.Instance->CR1, SPI_DATASIZE_16BIT);
		SET_BIT  (ILI_SPI_HANDLE.Instance->CR1, datasize);
		SET_BIT  (ILI_SPI_HANDLE.Instance->CR1, SPI_CR1_SPE);
#else
#error "SPI Data Size switching not implemented for target platform."
#endif
	}
}

/* Enable/disable DMA Memory Address Increment
 * (internal function)
 * Generates preprocessor error if not implemented for your target platform */
static inline void ILI_SPI_DMA_SetMemInc(uint32_t mode) {
	if (ILI_SPI_HANDLE.hdmatx->Init.MemInc != mode) {
		ILI_SPI_HANDLE.hdmatx->Init.MemInc =  mode;
		/* Platform specific implementation: */
#if defined STM32H7 || defined STM32F7 || defined STM32F4	// same on H7, F7, F4, and probably others
		/* DMA_SxCR reg, bit 10 MINC
		 * 1: increment enabled / 0: disabled (fixed pointer) */
		CLEAR_BIT(((DMA_Stream_TypeDef *)ILI_SPI_HANDLE.hdmatx->Instance)->CR, DMA_MINC_ENABLE);
		SET_BIT  (((DMA_Stream_TypeDef *)ILI_SPI_HANDLE.hdmatx->Instance)->CR, mode);
#else
#error "DMA Memory Increment Mode switching not implemented for target platform."
#endif
	}
}

/* Start DMA transfer
 * (internal function)
 * DMA module should be configured for half-word (16-bit) operation */
static HAL_StatusTypeDef ILI_DMA_Transfer(uint16_t *Buf, uint32_t MemInc) {
	if (DMA_Busy) return HAL_BUSY;

	// Check DMA data size
	// Abort in case of wrong configuration
	if (ILI_SPI_HANDLE.hdmatx->Init.PeriphDataAlignment != DMA_PDATAALIGN_HALFWORD) return HAL_ERROR;

	DMA_Busy = 1;

	HAL_GPIO_WritePin(ILI_DC_PORT, ILI_DC_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(ILI_CS_PORT, ILI_CS_PIN, GPIO_PIN_RESET);

	// Configure SPI & DMA peripherals
	ILI_SPI_SetDataSize(SPI_DATASIZE_16BIT);
	ILI_SPI_DMA_SetMemInc(MemInc);

	// Size set by ILI_Set_Address()
	uint32_t Size = Block_Width * Block_Height;

	// Clean DCache before DMA write operations
	#if defined __DCACHE_PRESENT && (__DCACHE_PRESENT == 1U) && defined ILI_DCACHE_ENABLED
	if (MemInc == DMA_MINC_ENABLE) {
		SCB_CleanDCache_by_Addr((uint32_t *)Buf, Size*2);	// size in bytes
	} else {
		SCB_CleanDCache_by_Addr((uint32_t *)Buf, 2);
	}
	#endif

	// Using callback function to start transfer
	DMA_SizeRemaining = Size;
	DMA_BufRemaining  = Buf;
	return ILI_DMA_Callback();
}

/* Start DMA transfer */
HAL_StatusTypeDef ILI_DMA_Load(uint16_t *Buf) {
	if (DMA_Busy) return HAL_BUSY;
	return ILI_DMA_Transfer(Buf, DMA_MINC_ENABLE);
}

/* Fill display with solid color */
HAL_StatusTypeDef ILI_DMA_Fill(uint16_t Color) {
	if (DMA_Busy) return HAL_BUSY;
	DMA_BufFill = Color;
	return ILI_DMA_Transfer((uint16_t *)&DMA_BufFill, DMA_MINC_DISABLE);
}

/* Continue/finish DMA transfer
 * To be called from HAL_SPI_TxCpltCallback */
HAL_StatusTypeDef ILI_DMA_Callback(void) {
	HAL_StatusTypeDef result = HAL_OK;

	if (DMA_SizeRemaining > 0) {	// Continue transfer
		uint16_t TransferSize;
		uint16_t *TransferBuf = (uint16_t *)DMA_BufRemaining;

		// Calculate DMA transfer size
		// (DMA module has 16-bit counter)
		if (DMA_SizeRemaining < UINT16_MAX) {
			TransferSize = DMA_SizeRemaining;
			DMA_SizeRemaining = 0;
		} else {
			TransferSize = UINT16_MAX - (UINT16_MAX % (Block_Width));
			DMA_SizeRemaining -= TransferSize;
			if (ILI_SPI_HANDLE.hdmatx->Init.MemInc != DMA_MINC_DISABLE) {	// Check if DMA_Fill in progress
				DMA_BufRemaining = &DMA_BufRemaining[TransferSize];
			}
		}

		result = HAL_SPI_Transmit_DMA(&ILI_SPI_HANDLE, (uint8_t *)TransferBuf, TransferSize);
	} else {
		// Restore 8-bit data size
		ILI_SPI_SetDataSize(SPI_DATASIZE_8BIT);
		// Restore MemInc
		ILI_SPI_DMA_SetMemInc(DMA_MINC_ENABLE);

		HAL_GPIO_WritePin(ILI_CS_PORT, ILI_CS_PIN, GPIO_PIN_SET);
		DMA_Busy = 0;
	}

	return result;
}

/* Returns non-zero value if transfer in progress */
uint8_t ILI_DMA_Busy(void) {
	return DMA_Busy;
}
