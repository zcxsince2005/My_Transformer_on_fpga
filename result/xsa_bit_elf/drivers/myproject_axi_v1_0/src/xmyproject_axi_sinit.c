// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#ifdef SDT
#include "xparameters.h"
#endif
#include "xmyproject_axi.h"

extern XMyproject_axi_Config XMyproject_axi_ConfigTable[];

#ifdef SDT
XMyproject_axi_Config *XMyproject_axi_LookupConfig(UINTPTR BaseAddress) {
	XMyproject_axi_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XMyproject_axi_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XMyproject_axi_ConfigTable[Index].Control_BaseAddress == BaseAddress) {
			ConfigPtr = &XMyproject_axi_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XMyproject_axi_Initialize(XMyproject_axi *InstancePtr, UINTPTR BaseAddress) {
	XMyproject_axi_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XMyproject_axi_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XMyproject_axi_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XMyproject_axi_Config *XMyproject_axi_LookupConfig(u16 DeviceId) {
	XMyproject_axi_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XMYPROJECT_AXI_NUM_INSTANCES; Index++) {
		if (XMyproject_axi_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XMyproject_axi_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XMyproject_axi_Initialize(XMyproject_axi *InstancePtr, u16 DeviceId) {
	XMyproject_axi_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XMyproject_axi_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XMyproject_axi_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

