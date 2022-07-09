/**
 * @file CAN_Setup.c
 * @author Leo Liu
 * @brief 
 * @version 1.0
 * @date 2022/07/08
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "CAN_Setup.h"

void CAN_IT_Init(CAN_HandleTypeDef *hcanx, uint8_t Can_type);
void CAN_RxMessage_Export_Data(CAN_HandleTypeDef *hcanx, osMessageQId CANx_Handle, uint8_t Can_type);
void CAN_SendData(osMessageQId CANx_Handle, CAN_HandleTypeDef *CANx, uint8_t id_type, uint32_t id, uint8_t data[8]);
void CAN_0x1FF_Send_Data(CAN_HandleTypeDef *hcanx, int16_t Output_1, int16_t Output_2, int16_t Output_3, int16_t Output_4);
void CAN_0x200_Send_Data(CAN_HandleTypeDef *hcanx, int16_t Output_1, int16_t Output_2, int16_t Output_3, int16_t Output_4);
void CAN_0x2FF_Send_Data(CAN_HandleTypeDef *hcanx, int16_t Output_1, int16_t Output_2, int16_t Output_3, int16_t Output_4);
void CAN_0x210_Send_Data(CAN_HandleTypeDef *hcanx, int16_t Target_Power);
void Check_CAN(void);

CAN_Func_t CAN_Func = CAN_Func_GroundInit;
#undef CAN_Func_GroundInit

CAN_Data_t CAN_Data[2] = CAN_Data_GroundInit;
#undef CAN_Data_GroundInit

CAN_Devices_t *Monitor_CAN[] = {&Monitor_CAN1, &Monitor_CAN2};
CAN_Devices_t Monitor_CAN1, Monitor_CAN2;
/*******************************************************************************/

/**
  * @Data   2021-03-24
  * @brief  CANɸѡ����ʼ��
  * @param  CAN_FilterTypeDef *CAN_Filter, CAN_HandleTypeDef *hcanx
  * @retval void
  */
static void CAN_FILTER_Init(CAN_FilterTypeDef *CAN_Filter, CAN_HandleTypeDef *hcanx)
{
    CAN_Filter->FilterFIFOAssignment = CAN_FILTER_FIFO0; //ɸѡ����������FIFO0
    CAN_Filter->FilterBank = 0;                          //ɸѡ����0
    CAN_Filter->SlaveStartFilterBank = 0;
    CAN_Filter->FilterMode = CAN_FILTERMODE_IDMASK;   //������ID����ģʽ
    CAN_Filter->FilterScale = CAN_FILTERSCALE_32BIT;  //ɸѡ��λ��Ϊ����32λ��
    CAN_Filter->FilterActivation = CAN_FILTER_ENABLE; //ʹ��ɸѡ��
                                                      /* ʹ��ɸѡ�������ձ�־�����ݽ��бȶ�ɸѡ����չID�������µľ����������ǵĻ��������FIFO0�� */
    CAN_Filter->FilterIdHigh = 0x0000;                //Ҫɸѡ��ID��λ
    CAN_Filter->FilterIdLow = 0x0000;                 //Ҫɸѡ��ID��λ
    CAN_Filter->FilterMaskIdHigh = 0x0000;            //ɸѡ����16λÿλ����ƥ��
    CAN_Filter->FilterMaskIdLow = 0x0000;             //ɸѡ����16λÿλ����ƥ��
    HAL_CAN_ConfigFilter(hcanx, CAN_Filter);
}

/**
  * @Data   2021-03-24
  * @brief   canx�ж�����
  * @param   CAN_HandleTypeDef *hcanx, uint8_t Can_type
  * @retval  void
  */
void CAN_IT_Init(CAN_HandleTypeDef *hcanx, uint8_t Can_type)
{
    uint8_t Canx_type = Can_type - 1;
    /*ʹ���˲���*/
    CAN_FILTER_Init(&CAN_Data[Canx_type].CAN_FilterTypedef.CAN_Filter, hcanx);
    /*����CAN*/
    HAL_CAN_Start(hcanx);
    /*ʹ��CAN��IT�ж�*/
    __HAL_CAN_ENABLE_IT(hcanx, CAN_IT_RX_FIFO0_MSG_PENDING); //  CAN_IT_FMP0
}

/**
  * @Data   2021-03-27
  * @brief  canx�жϽ���
  * @param  CAN_HandleTypeDef *hcanx, osMessageQId CANx_Handle, uint8_t Can_type
  * @retval void
  */
void CAN_RxMessage_Export_Data(CAN_HandleTypeDef *hcanx, osMessageQId CANx_Handle, uint8_t Can_type)
{
    CAN_Export_Data_t CAN_Export_Data[2];
    uint8_t Canx_type = Can_type - 1;
    HAL_CAN_GetRxMessage(hcanx, CAN_RX_FIFO0,&CAN_Data[Canx_type].CAN_RxTypedef.CANx_RxHeader,CAN_Data[Canx_type].CAN_RxTypedef.CAN_RxMessage);

    CAN_Export_Data[Canx_type].CAN_RxHeader = CAN_Data[Canx_type].CAN_RxTypedef.CANx_RxHeader;
    memcpy(&CAN_Export_Data[Canx_type].CANx_Export_RxMessage,CAN_Data[Canx_type].CAN_RxTypedef.CAN_RxMessage,sizeof(uint8_t[8]));

    xQueueSendToBackFromISR(CANx_Handle, &CAN_Export_Data[Canx_type], 0); //�ѽ������ݷ������ն���
}

/**
  * @brief  CAN��������
  * @param  CANx 		CAN���
  * 		id_type ��	id���� CAN_ID_STD�� CAN_ID_EXT
  *			id			id��
  * 		data[8]		8������
  * @retval None
  */
void CAN_SendData(osMessageQId CANx_Handle, CAN_HandleTypeDef *CANx, uint8_t id_type, uint32_t id, uint8_t data[8])
{
    CAN_Send_Data_t CAN_Send_Data;

    CAN_Send_Data.Canx = CANx;
    if (id_type == CAN_ID_STD)
    {
        CAN_Send_Data.CAN_TxHeader.StdId = id;
    }
    else
    {
        CAN_Send_Data.CAN_TxHeader.ExtId = id; //ID��
    }

    CAN_Send_Data.CAN_TxHeader.IDE = id_type;      //ID����
    CAN_Send_Data.CAN_TxHeader.RTR = CAN_RTR_DATA; //���͵�Ϊ����
    CAN_Send_Data.CAN_TxHeader.DLC = 0x08;         //���ݳ���Ϊ8�ֽ�
    CAN_Send_Data.CAN_TxHeader.TransmitGlobalTime = DISABLE;

    memcpy(CAN_Send_Data.CANx_Send_RxMessage,data,sizeof(uint8_t[8]));

    xQueueSend(CANx_Handle, &CAN_Send_Data, 0);
}

void CAN_0x1FF_Send_Data(CAN_HandleTypeDef *hcanx, int16_t Output_1, int16_t Output_2, int16_t Output_3, int16_t Output_4)
{
	uint8_t data[8];

	data[0] = Output_1 >> 8;
	data[1] = Output_1;
	data[2] = Output_2 >> 8;
	data[3] = Output_2;
	data[4] = Output_3 >> 8;
	data[5] = Output_3;
	data[6] = Output_4 >> 8;
	data[7] = Output_4;

	CAN_SendData(CAN_SendHandle, hcanx, CAN_ID_STD, 0x1FF, data);
}

void CAN_0x200_Send_Data(CAN_HandleTypeDef *hcanx, int16_t Output_1, int16_t Output_2, int16_t Output_3, int16_t Output_4)
{
	uint8_t data[8];

	data[0] = Output_1 >> 8;
	data[1] = Output_1;
	data[2] = Output_2 >> 8;
	data[3] = Output_2;
	data[4] = Output_3 >> 8;
	data[5] = Output_3;
	data[6] = Output_4 >> 8;
	data[7] = Output_4;

	CAN_SendData(CAN_SendHandle, hcanx, CAN_ID_STD, 0x200, data);
}
void CAN_0x2FF_Send_Data(CAN_HandleTypeDef *hcanx, int16_t Output_1, int16_t Output_2, int16_t Output_3, int16_t Output_4)
{
	uint8_t data[8];

	data[0] = Output_1 >> 8;
	data[1] = Output_1;
	data[2] = Output_2 >> 8;
	data[3] = Output_2;
	data[4] = Output_3 >> 8;
	data[5] = Output_3;
	data[6] = Output_4 >> 8;
	data[7] = Output_4;

	CAN_SendData(CAN_SendHandle, hcanx, CAN_ID_STD, 0x2FF, data);
}

void CAN_0x210_Send_Data(CAN_HandleTypeDef *hcanx, int16_t Target_Power)
{
	uint8_t data[8];

	data[0] = Target_Power >> 8;
	data[1] = Target_Power;

	CAN_SendData(CAN_SendHandle, hcanx, CAN_ID_STD, 0x210, data);
}

void Check_CAN(void)
{
    for (uint8_t i = 0; i < 2; i++)
    {
        if (Monitor_CAN[i]->Info_Update_Frame < 1)
        {
            Monitor_CAN[i]->Offline_Flag = 1;
        }
        else
        {
            Monitor_CAN[i]->Offline_Flag = 0;
        }
        Monitor_CAN[i]->Info_Update_Frame = 0;
    }
}
