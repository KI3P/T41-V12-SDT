#ifndef RF_CONTROL_h
#define RF_CONTROL_h

void RFControlInit();
void RFControl_Enable_Prescaler(bool status);
void SetRF_InAtten(int attenInx2);
void SetRF_OutAtten(int attenOutx2);

#endif // RF_CONTROL_H