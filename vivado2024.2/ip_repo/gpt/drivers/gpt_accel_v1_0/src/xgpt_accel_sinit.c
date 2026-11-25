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
#include "xgpt_accel.h"

extern XGpt_accel_Config XGpt_accel_ConfigTable[];

#ifdef SDT
XGpt_accel_Config *XGpt_accel_LookupConfig(UINTPTR BaseAddress) {
	XGpt_accel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XGpt_accel_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XGpt_accel_ConfigTable[Index].Control_BaseAddress == BaseAddress) {
			ConfigPtr = &XGpt_accel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XGpt_accel_Initialize(XGpt_accel *InstancePtr, UINTPTR BaseAddress) {
	XGpt_accel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XGpt_accel_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XGpt_accel_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XGpt_accel_Config *XGpt_accel_LookupConfig(u16 DeviceId) {
	XGpt_accel_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XGPT_ACCEL_NUM_INSTANCES; Index++) {
		if (XGpt_accel_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XGpt_accel_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XGpt_accel_Initialize(XGpt_accel *InstancePtr, u16 DeviceId) {
	XGpt_accel_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XGpt_accel_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XGpt_accel_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

