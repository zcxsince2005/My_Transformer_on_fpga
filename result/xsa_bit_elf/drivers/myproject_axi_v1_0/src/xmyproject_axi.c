// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xmyproject_axi.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XMyproject_axi_CfgInitialize(XMyproject_axi *InstancePtr, XMyproject_axi_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XMyproject_axi_Start(XMyproject_axi *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_AP_CTRL) & 0x80;
    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XMyproject_axi_IsDone(XMyproject_axi *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XMyproject_axi_IsIdle(XMyproject_axi *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XMyproject_axi_IsReady(XMyproject_axi *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XMyproject_axi_EnableAutoRestart(XMyproject_axi *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XMyproject_axi_DisableAutoRestart(XMyproject_axi *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_AP_CTRL, 0);
}

void XMyproject_axi_Set_in_ptr(XMyproject_axi *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_IN_PTR_DATA, (u32)(Data));
    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_IN_PTR_DATA + 4, (u32)(Data >> 32));
}

u64 XMyproject_axi_Get_in_ptr(XMyproject_axi *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_IN_PTR_DATA);
    Data += (u64)XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_IN_PTR_DATA + 4) << 32;
    return Data;
}

void XMyproject_axi_Set_out_ptr(XMyproject_axi *InstancePtr, u64 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_OUT_PTR_DATA, (u32)(Data));
    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_OUT_PTR_DATA + 4, (u32)(Data >> 32));
}

u64 XMyproject_axi_Get_out_ptr(XMyproject_axi *InstancePtr) {
    u64 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_OUT_PTR_DATA);
    Data += (u64)XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_OUT_PTR_DATA + 4) << 32;
    return Data;
}

void XMyproject_axi_InterruptGlobalEnable(XMyproject_axi *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_GIE, 1);
}

void XMyproject_axi_InterruptGlobalDisable(XMyproject_axi *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_GIE, 0);
}

void XMyproject_axi_InterruptEnable(XMyproject_axi *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_IER);
    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_IER, Register | Mask);
}

void XMyproject_axi_InterruptDisable(XMyproject_axi *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_IER);
    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_IER, Register & (~Mask));
}

void XMyproject_axi_InterruptClear(XMyproject_axi *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XMyproject_axi_WriteReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_ISR, Mask);
}

u32 XMyproject_axi_InterruptGetEnabled(XMyproject_axi *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_IER);
}

u32 XMyproject_axi_InterruptGetStatus(XMyproject_axi *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XMyproject_axi_ReadReg(InstancePtr->Control_BaseAddress, XMYPROJECT_AXI_CONTROL_ADDR_ISR);
}

