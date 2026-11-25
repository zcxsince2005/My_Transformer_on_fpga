// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xgpt_accel.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XGpt_accel_CfgInitialize(XGpt_accel *InstancePtr, XGpt_accel_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XGpt_accel_Start(XGpt_accel *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_AP_CTRL) & 0x80;
    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XGpt_accel_IsDone(XGpt_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XGpt_accel_IsIdle(XGpt_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XGpt_accel_IsReady(XGpt_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XGpt_accel_EnableAutoRestart(XGpt_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XGpt_accel_DisableAutoRestart(XGpt_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_AP_CTRL, 0);
}

void XGpt_accel_Set_in_a(XGpt_accel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_A_DATA, (u32)(Data));
    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_A_DATA + 4, (u32)(Data >> 32));
}

u64 XGpt_accel_Get_in_a(XGpt_accel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_A_DATA);
    Data += (u64)XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_A_DATA + 4) << 32;
    return Data;
}

void XGpt_accel_Set_in_b(XGpt_accel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_B_DATA, (u32)(Data));
    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_B_DATA + 4, (u32)(Data >> 32));
}

u64 XGpt_accel_Get_in_b(XGpt_accel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_B_DATA);
    Data += (u64)XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_B_DATA + 4) << 32;
    return Data;
}

void XGpt_accel_Set_in_c(XGpt_accel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_C_DATA, (u32)(Data));
    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_C_DATA + 4, (u32)(Data >> 32));
}

u64 XGpt_accel_Get_in_c(XGpt_accel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_C_DATA);
    Data += (u64)XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IN_C_DATA + 4) << 32;
    return Data;
}

void XGpt_accel_Set_out_r(XGpt_accel *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_OUT_R_DATA, (u32)(Data));
    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_OUT_R_DATA + 4, (u32)(Data >> 32));
}

u64 XGpt_accel_Get_out_r(XGpt_accel *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_OUT_R_DATA);
    Data += (u64)XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_OUT_R_DATA + 4) << 32;
    return Data;
}

void XGpt_accel_Set_rows(XGpt_accel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_ROWS_DATA, Data);
}

u32 XGpt_accel_Get_rows(XGpt_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_ROWS_DATA);
    return Data;
}

void XGpt_accel_Set_cols(XGpt_accel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_COLS_DATA, Data);
}

u32 XGpt_accel_Get_cols(XGpt_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_COLS_DATA);
    return Data;
}

void XGpt_accel_Set_inner(XGpt_accel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_INNER_DATA, Data);
}

u32 XGpt_accel_Get_inner(XGpt_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_INNER_DATA);
    return Data;
}

void XGpt_accel_Set_mode(XGpt_accel *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_MODE_DATA, Data);
}

u32 XGpt_accel_Get_mode(XGpt_accel *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_MODE_DATA);
    return Data;
}

void XGpt_accel_InterruptGlobalEnable(XGpt_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_GIE, 1);
}

void XGpt_accel_InterruptGlobalDisable(XGpt_accel *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_GIE, 0);
}

void XGpt_accel_InterruptEnable(XGpt_accel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IER);
    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IER, Register | Mask);
}

void XGpt_accel_InterruptDisable(XGpt_accel *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IER);
    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IER, Register & (~Mask));
}

void XGpt_accel_InterruptClear(XGpt_accel *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XGpt_accel_WriteReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_ISR, Mask);
}

u32 XGpt_accel_InterruptGetEnabled(XGpt_accel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_IER);
}

u32 XGpt_accel_InterruptGetStatus(XGpt_accel *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XGpt_accel_ReadReg(InstancePtr->Control_BaseAddress, XGPT_ACCEL_CONTROL_ADDR_ISR);
}

