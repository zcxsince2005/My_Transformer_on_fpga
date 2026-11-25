// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.2 (64-bit)
// Tool Version Limit: 2024.11
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XMYPROJECT_AXI_H
#define XMYPROJECT_AXI_H

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
#include "xmyproject_axi_hw.h"

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
} XMyproject_axi_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XMyproject_axi;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XMyproject_axi_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XMyproject_axi_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XMyproject_axi_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XMyproject_axi_ReadReg(BaseAddress, RegOffset) \
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
int XMyproject_axi_Initialize(XMyproject_axi *InstancePtr, UINTPTR BaseAddress);
XMyproject_axi_Config* XMyproject_axi_LookupConfig(UINTPTR BaseAddress);
#else
int XMyproject_axi_Initialize(XMyproject_axi *InstancePtr, u16 DeviceId);
XMyproject_axi_Config* XMyproject_axi_LookupConfig(u16 DeviceId);
#endif
int XMyproject_axi_CfgInitialize(XMyproject_axi *InstancePtr, XMyproject_axi_Config *ConfigPtr);
#else
int XMyproject_axi_Initialize(XMyproject_axi *InstancePtr, const char* InstanceName);
int XMyproject_axi_Release(XMyproject_axi *InstancePtr);
#endif

void XMyproject_axi_Start(XMyproject_axi *InstancePtr);
u32 XMyproject_axi_IsDone(XMyproject_axi *InstancePtr);
u32 XMyproject_axi_IsIdle(XMyproject_axi *InstancePtr);
u32 XMyproject_axi_IsReady(XMyproject_axi *InstancePtr);
void XMyproject_axi_EnableAutoRestart(XMyproject_axi *InstancePtr);
void XMyproject_axi_DisableAutoRestart(XMyproject_axi *InstancePtr);

void XMyproject_axi_Set_in_ptr(XMyproject_axi *InstancePtr, u64 Data);
u64 XMyproject_axi_Get_in_ptr(XMyproject_axi *InstancePtr);
void XMyproject_axi_Set_out_ptr(XMyproject_axi *InstancePtr, u64 Data);
u64 XMyproject_axi_Get_out_ptr(XMyproject_axi *InstancePtr);

void XMyproject_axi_InterruptGlobalEnable(XMyproject_axi *InstancePtr);
void XMyproject_axi_InterruptGlobalDisable(XMyproject_axi *InstancePtr);
void XMyproject_axi_InterruptEnable(XMyproject_axi *InstancePtr, u32 Mask);
void XMyproject_axi_InterruptDisable(XMyproject_axi *InstancePtr, u32 Mask);
void XMyproject_axi_InterruptClear(XMyproject_axi *InstancePtr, u32 Mask);
u32 XMyproject_axi_InterruptGetEnabled(XMyproject_axi *InstancePtr);
u32 XMyproject_axi_InterruptGetStatus(XMyproject_axi *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
