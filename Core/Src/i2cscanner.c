/*
 * i2cscanner.c
 *
 *  Created on: Aug 22, 2022
 *      Author: ralph
 */
//	char Buffer[25] = {0};
//	uint8_t Space[] = " - ";
//	uint8_t StartMSG[] = "Starting I2C Scanning: \r\n";
//	uint8_t EndMSG[] = "Done! \r\n\r\n";

//I2C addresss scan
////   HAL_UART_Transmit(&huart2, StartMSG, sizeof(StartMSG), 10000);
//    for(int i=1; i<128; i++)
//    {
//        int ret = HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i<<1), 3, 5);
//        if (ret != HAL_OK) /* No ACK Received At That Address */
//        {
//            HAL_UART_Transmit(&huart2, Space, sizeof(Space), 10000);
//        }
//        else if(ret == HAL_OK)
//        {
//            sprintf(Buffer, "0x%X", i);
//            HAL_UART_Transmit(&huart2, Buffer, sizeof(Buffer), 10000);
//        }
//    }
////    HAL_UART_Transmit(&huart2, EndMSG, sizeof(EndMSG), 10000);
////    /*--[ Scanning Done ]--*/
//    ssd1306_TestAll();
