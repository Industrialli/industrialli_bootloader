#include <Arduino.h>
#include <STM32SD.h>
#include <FatFs.h>

#include "stm32h7xx_hal.h"

#define START_ADDRESS_APPLICATION 0x08020000

typedef void (*pFunction)(void);

HAL_StatusTypeDef erase_app_memory(uint32_t bank, uint32_t sector, uint32_t NbSectors);
HAL_StatusTypeDef Flash_write32B(uint8_t const *src, uint32_t dst);
static void goto_application();
bool update_firmware(const TCHAR* fwPath, uint32_t flashBank, uint32_t flashSector, uint32_t NbSectors, uint32_t fwFlashStartAdd);

void setup(){
	pinMode(PB10, OUTPUT);
	pinMode(PA15, INPUT);

	if(!digitalRead(PA15)){
		SdFatFs fatFs;
		fatFs.init();

		FATFS fs;
		uint8_t mountRes;

  		mountRes = f_mount(&fs, "/", 1);

		if (mountRes == FR_OK) {
			for(int i = 0; i < 25; i++){
				digitalToggle(PB10);
				delay(50);
			}

			update_firmware("firmware.bin", FLASH_BANK_1, FLASH_SECTOR_1, 3, START_ADDRESS_APPLICATION);

			f_mount(NULL, "/", 0);

			for(int i = 0; i < 25; i++){
				digitalToggle(PB10);
				delay(50);
			}
		}
	}
	
	goto_application();
}

void loop(){}

HAL_StatusTypeDef erase_app_memory(uint32_t bank, uint32_t sector, uint32_t NbSectors){
	
	HAL_StatusTypeDef ret;
	FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t SectorError;
	
	HAL_FLASH_Unlock();
	
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
	EraseInitStruct.Banks = bank;
	EraseInitStruct.Sector = sector;
	EraseInitStruct.NbSectors = NbSectors;
	EraseInitStruct.VoltageRange  = FLASH_VOLTAGE_RANGE_4;

	ret = HAL_FLASHEx_Erase( &EraseInitStruct, &SectorError );

	HAL_FLASH_Lock();
	return ret;
}


HAL_StatusTypeDef Flash_write32B(uint8_t const *src, uint32_t dst){
    uint32_t FlashAddress = dst;

    if (FlashAddress & (32-1)) {
        return HAL_ERROR;
    }
	
	HAL_StatusTypeDef ret;
	
	HAL_FLASH_Unlock();
	
	ret = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, FlashAddress, (uint32_t)src);

	HAL_FLASH_Lock();

	return ret;
	
}

static void goto_application(){
	typedef void(*pMainApp)(void);
	pMainApp mainApplication;

	uint32_t mainAppStack = (uint32_t)*((uint32_t*)START_ADDRESS_APPLICATION);
	mainApplication = (pMainApp)*(uint32_t*)(START_ADDRESS_APPLICATION + 4);

	SCB->VTOR = START_ADDRESS_APPLICATION;

	mainApplication();
}

bool update_firmware(const TCHAR* fwPath, uint32_t flashBank, uint32_t flashSector, uint32_t NbSectors, uint32_t fwFlashStartAdd){
	uint8_t readBytes[512];
	UINT bytesRead;
	FSIZE_t file_size;
	FIL file;
	uint32_t flashAdd, addCNTR;

	if (f_open(&file, fwPath, FA_READ) == FR_OK) {

		file_size = f_size(&file);

		erase_app_memory(flashBank, flashSector, NbSectors);

		flashAdd = fwFlashStartAdd;
		addCNTR  = 0;

		while (f_read(&file, readBytes, 512, &bytesRead) == FR_OK && bytesRead > 0) {
			for(uint32_t i = 0; i < 16; i++){
				Flash_write32B(readBytes + ( i * 32), flashAdd + addCNTR);
				addCNTR += 32;
			}
			memset(readBytes, 0xFF, sizeof(readBytes));
		}
		f_close(&file);
	}

	return true;
}