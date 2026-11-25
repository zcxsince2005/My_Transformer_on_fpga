// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XGPT_ACCEL_H
#define XGPT_ACCEL_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xgpt_accel_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
typedef struct {
#ifdef SDT
    char *Name;
#else
    u16 DeviceId;
#endif
    u64 Control_BaseAddress;
} XGpt_accel_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XGpt_accel;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XGpt_accel_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XGpt_accel_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XGpt_accel_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XGpt_accel_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
#ifdef SDT
int XGpt_accel_Initialize(XGpt_accel *InstancePtr, UINTPTR BaseAddress);
XGpt_accel_Config* XGpt_accel_LookupConfig(UINTPTR BaseAddress);
#else
int XGpt_accel_Initialize(XGpt_accel *InstancePtr, u16 DeviceId);
XGpt_accel_Config* XGpt_accel_LookupConfig(u16 DeviceId);
#endif
int XGpt_accel_CfgInitialize(XGpt_accel *InstancePtr, XGpt_accel_Config *ConfigPtr);
#else
int XGpt_accel_Initialize(XGpt_accel *InstancePtr, const char* InstanceName);
int XGpt_accel_Release(XGpt_accel *InstancePtr);
#endif

void XGpt_accel_Start(XGpt_accel *InstancePtr);
u32 XGpt_accel_IsDone(XGpt_accel *InstancePtr);
u32 XGpt_accel_IsIdle(XGpt_accel *InstancePtr);
u32 XGpt_accel_IsReady(XGpt_accel *InstancePtr);
void XGpt_accel_EnableAutoRestart(XGpt_accel *InstancePtr);
void XGpt_accel_DisableAutoRestart(XGpt_accel *InstancePtr);

void XGpt_accel_Set_in_a(XGpt_accel *InstancePtr, u64 Data);
u64 XGpt_accel_Get_in_a(XGpt_accel *InstancePtr);
void XGpt_accel_Set_in_b(XGpt_accel *InstancePtr, u64 Data);
u64 XGpt_accel_Get_in_b(XGpt_accel *InstancePtr);
void XGpt_accel_Set_in_c(XGpt_accel *InstancePtr, u64 Data);
u64 XGpt_accel_Get_in_c(XGpt_accel *InstancePtr);
void XGpt_accel_Set_out_r(XGpt_accel *InstancePtr, u64 Data);
u64 XGpt_accel_Get_out_r(XGpt_accel *InstancePtr);
void XGpt_accel_Set_rows(XGpt_accel *InstancePtr, u32 Data);
u32 XGpt_accel_Get_rows(XGpt_accel *InstancePtr);
void XGpt_accel_Set_cols(XGpt_accel *InstancePtr, u32 Data);
u32 XGpt_accel_Get_cols(XGpt_accel *InstancePtr);
void XGpt_accel_Set_inner(XGpt_accel *InstancePtr, u32 Data);
u32 XGpt_accel_Get_inner(XGpt_accel *InstancePtr);
void XGpt_accel_Set_mode(XGpt_accel *InstancePtr, u32 Data);
u32 XGpt_accel_Get_mode(XGpt_accel *InstancePtr);

void XGpt_accel_InterruptGlobalEnable(XGpt_accel *InstancePtr);
void XGpt_accel_InterruptGlobalDisable(XGpt_accel *InstancePtr);
void XGpt_accel_InterruptEnable(XGpt_accel *InstancePtr, u32 Mask);
void XGpt_accel_InterruptDisable(XGpt_accel *InstancePtr, u32 Mask);
void XGpt_accel_InterruptClear(XGpt_accel *InstancePtr, u32 Mask);
u32 XGpt_accel_InterruptGetEnabled(XGpt_accel *InstancePtr);
u32 XGpt_accel_InterruptGetStatus(XGpt_accel *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
