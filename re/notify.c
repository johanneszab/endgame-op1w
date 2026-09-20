/* index

--- callers of 0x004030d0 (hid_read_timeout) ---
    hid_read                     @ 004031e0   (call site 004031f7)
    FUN_00416ad0                 @ 00416ad0   (call site 00416b19)
    total new: 2

--- callers of 0x00402eb0 (hid_open_path) ---
    hid_open                     @ 00402dd0   (call site 00402e81)
    total new: 1

--- callers of 0x004028e0 (hid_enumerate) ---
    hid_open                     @ 00402dd0   (call site 00402dea)
    total new: 0

*/

/* ======================================================
 * hid_read_timeout @ 004030d0
 * ====================================================== */

size_t __cdecl hid_read_timeout(undefined4 *param_1,void *param_2,DWORD param_3,DWORD param_4)

{
  HANDLE hEvent;
  char *_Src;
  BOOL BVar1;
  DWORD DVar2;
  DWORD _Size;
  DWORD local_8;
  
                    /* 0x30d0  15  hid_read_timeout */
  _Size = 0;
  hEvent = (HANDLE)param_1[0xc];
  local_8 = 0;
  if (param_1[6] == 0) {
    param_1[6] = 1;
    _memset((void *)param_1[7],0,param_1[3]);
    ResetEvent(hEvent);
    BVar1 = ReadFile((HANDLE)*param_1,(LPVOID)param_1[7],param_1[3],&local_8,
                     (LPOVERLAPPED)(param_1 + 8));
    if (BVar1 == 0) {
      DVar2 = GetLastError();
      if (DVar2 != 0x3e5) {
        CancelIo((HANDLE)*param_1);
        param_1[6] = 0;
        goto LAB_0040313c;
      }
    }
  }
  if (-1 < (int)param_4) {
    DVar2 = WaitForSingleObject(hEvent,param_4);
    if (DVar2 != 0) {
      return 0;
    }
  }
  BVar1 = GetOverlappedResult((HANDLE)*param_1,(LPOVERLAPPED)(param_1 + 8),&local_8,1);
  param_1[6] = 0;
  if (BVar1 != 0) {
    if (local_8 != 0) {
      _Src = (char *)param_1[7];
      if (*_Src != '\0') {
        if (local_8 < param_3) {
          param_3 = local_8;
        }
        FID_conflict__memcpy(param_2,_Src,param_3);
        return param_3;
      }
      local_8 = local_8 - 1;
      _Size = param_3;
      if (local_8 < param_3) {
        _Size = local_8;
      }
      FID_conflict__memcpy(param_2,_Src + 1,_Size);
    }
    return _Size;
  }
LAB_0040313c:
  FUN_004026c0();
  return 0xffffffff;
}



/* ======================================================
 * hid_read @ 004031e0
 * ====================================================== */

void __cdecl hid_read(undefined4 *param_1,void *param_2,DWORD param_3)

{
                    /* 0x31e0  14  hid_read */
  hid_read_timeout(param_1,param_2,param_3,-(uint)(param_1[1] != 0));
  return;
}



/* ======================================================
 * FUN_00416ad0 @ 00416ad0
 * ====================================================== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall FUN_00416ad0(uint3 param_1,int param_2)

{
  undefined3 uVar1;
  size_t sVar2;
  undefined4 uStack_8;
  
  _DAT_00587188 = 0;
  _DAT_0058718c = 0;
  DAT_00587191._3_1_ = 0;
  DAT_00587195 = 0;
  DAT_00587197 = 0;
  uStack_8 = (uint)param_1;
  DAT_00587191._0_3_ = 0;
  uVar1 = (undefined3)DAT_00587191;
  DAT_00587190 = 3;
  DAT_00587191._0_3_ = 0;
  if (param_2 != 0) {
    while ((uVar1 = (undefined3)DAT_00587191, DAT_00587180 != (undefined4 *)0x0 &&
           (sVar2 = hid_read_timeout(DAT_00587180,&DAT_00587190,8,-(uint)(DAT_00587180[1] != 0)),
           uVar1 = (undefined3)DAT_00587191, sVar2 != 0xffffffff))) {
      _DAT_00587188 = CONCAT13(DAT_00587191._3_1_,(undefined3)DAT_00587191);
      uStack_8 = CONCAT13(1,(undefined3)uStack_8);
      _DAT_0058718c = CONCAT12(DAT_00587197,DAT_00587195);
      (*DAT_00586290)((int)&uStack_8 + 3,&DAT_00587188);
      Sleep(10);
    }
  }
  DAT_00587191._0_3_ = uVar1;
  return 0;
}



/* ======================================================
 * hid_open_path @ 00402eb0
 * ====================================================== */

void __cdecl hid_open_path(LPCSTR param_1)

{
  char cVar1;
  int iVar2;
  undefined4 *_Memory;
  HANDLE pvVar3;
  void *pvVar4;
  undefined4 unaff_ESI;
  undefined1 auStack_58 [4];
  undefined4 local_54;
  uint uStack_18;
  uint local_c;
  
                    /* 0x2eb0  13  hid_open_path */
  local_c = DAT_0057e340 ^ (uint)auStack_58;
  local_54 = 0;
  if (DAT_005862a0 == '\0') {
    iVar2 = FUN_00402730();
    if (iVar2 < 0) {
      if (DAT_0058629c != (HMODULE)0x0) {
        FreeLibrary(DAT_0058629c);
      }
      DAT_0058629c = (HMODULE)0x0;
      DAT_005862a0 = 0;
      __security_check_cookie(local_c ^ (uint)auStack_58);
      return;
    }
    DAT_005862a0 = '\x01';
  }
  _Memory = FUN_00402670();
  pvVar3 = CreateFileA(param_1,0xc0000000,3,(LPSECURITY_ATTRIBUTES)0x0,3,0x40000000,(HANDLE)0x0);
  *_Memory = pvVar3;
  if (pvVar3 != (HANDLE)0xffffffff) {
    cVar1 = (*DAT_005861e8)(pvVar3);
    if (cVar1 != '\0') {
      cVar1 = (*DAT_005861e0)(pvVar3);
      if (cVar1 != '\0') {
        iVar2 = (*DAT_00586204)(local_54);
        if (iVar2 == 0x110000) {
          *(undefined2 *)(_Memory + 2) = local_54._2_2_;
          _Memory[3] = local_54 & 0xffff;
          (*DAT_005861fc)(unaff_ESI);
          pvVar4 = _malloc(_Memory[3]);
          _Memory[7] = pvVar4;
          __security_check_cookie(uStack_18 ^ (uint)&stack0xffffff9c);
          return;
        }
        FUN_004026c0();
        (*DAT_005861fc)(unaff_ESI);
        goto LAB_00402f92;
      }
    }
  }
  FUN_004026c0();
LAB_00402f92:
  CloseHandle((HANDLE)_Memory[0xc]);
  CloseHandle((HANDLE)*_Memory);
  LocalFree((HLOCAL)_Memory[4]);
  _free((void *)_Memory[7]);
  _free(_Memory);
  __security_check_cookie(local_c ^ (uint)auStack_58);
  return;
}



/* ======================================================
 * hid_open @ 00402dd0
 * ====================================================== */

undefined4 __cdecl hid_open(short param_1,short param_2,ushort *param_3)

{
  int *piVar1;
  ushort uVar2;
  undefined4 *puVar3;
  ushort *puVar4;
  int iVar5;
  ushort *puVar6;
  undefined4 *puVar7;
  bool bVar8;
  undefined4 local_c;
  
                    /* 0x2dd0  12  hid_open */
  local_c = 0;
  puVar3 = (undefined4 *)hid_enumerate(param_1,param_2);
  puVar7 = puVar3;
  if (puVar3 != (undefined4 *)0x0) {
    while ((((*(short *)(puVar7 + 1) != param_1 || (*(short *)((int)puVar7 + 6) != param_2)) ||
            (*(short *)(puVar7 + 6) != -0xfe)) || (*(short *)((int)puVar7 + 0x1a) != 1))) {
LAB_00402e5f:
      piVar1 = puVar7 + 8;
      puVar7 = (undefined4 *)*piVar1;
      if ((undefined4 *)*piVar1 == (undefined4 *)0x0) {
        hid_free_enumeration(puVar3);
        return 0;
      }
    }
    if (param_3 != (ushort *)0x0) {
      puVar6 = (ushort *)puVar7[2];
      puVar4 = param_3;
      do {
        uVar2 = *puVar4;
        bVar8 = uVar2 < *puVar6;
        if (uVar2 != *puVar6) {
LAB_00402e56:
          iVar5 = (1 - (uint)bVar8) - (uint)(bVar8 != 0);
          goto LAB_00402e5b;
        }
        if (uVar2 == 0) break;
        uVar2 = puVar4[1];
        bVar8 = uVar2 < puVar6[1];
        if (uVar2 != puVar6[1]) goto LAB_00402e56;
        puVar4 = puVar4 + 2;
        puVar6 = puVar6 + 2;
      } while (uVar2 != 0);
      iVar5 = 0;
LAB_00402e5b:
      if (iVar5 != 0) goto LAB_00402e5f;
    }
    if ((LPCSTR)*puVar7 != (LPCSTR)0x0) {
      local_c = hid_open_path((LPCSTR)*puVar7);
    }
  }
  hid_free_enumeration(puVar3);
  return local_c;
}



/* ======================================================
 * hid_enumerate @ 004028e0
 * ====================================================== */

void __cdecl hid_enumerate(short param_1,short param_2)

{
  byte bVar1;
  int *piVar2;
  char cVar3;
  int iVar4;
  HDEVINFO DeviceInfoSet;
  PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData;
  BOOL BVar5;
  byte *pbVar6;
  int *piVar7;
  CHAR *lpFileName;
  wchar_t *pwVar8;
  char *pcVar9;
  long lVar10;
  char *pcVar11;
  undefined4 unaff_ESI;
  bool bVar12;
  char **ppcVar13;
  char *local_5c4;
  DWORD local_5c0;
  char *local_5bc;
  int *local_5b8;
  PSP_DEVICE_INTERFACE_DETAIL_DATA_A local_5b4;
  DWORD local_5b0;
  HDEVINFO local_5ac;
  int *local_5a8;
  GUID local_5a4;
  undefined4 local_594;
  short sStack_590;
  short sStack_58e;
  undefined2 uStack_58c;
  _SP_DEVINFO_DATA local_588;
  _SP_DEVICE_INTERFACE_DATA local_56c [3];
  byte local_510 [224];
  wchar_t awStack_430 [6];
  wchar_t awStack_424 [6];
  undefined1 auStack_418 [8];
  wchar_t awStack_410 [495];
  undefined2 uStack_32;
  undefined2 uStack_26;
  undefined2 uStack_12;
  uint local_c;
  
                    /* 0x28e0  2  hid_enumerate */
  local_c = DAT_0057e340 ^ (uint)&local_5c4;
  local_5b8 = (int *)0x0;
  local_5a8 = (int *)0x0;
  local_5a4.Data1 = 0x4d1e55b2;
  local_5a4.Data2 = 0xf16f;
  local_5a4.Data3 = 0x11cf;
  local_5a4.Data4[0] = 0x88;
  local_5a4.Data4[1] = 0xcb;
  local_5a4.Data4[2] = '\0';
  local_5a4.Data4[3] = '\x11';
  local_5a4.Data4[4] = '\x11';
  local_5a4.Data4[5] = '\0';
  local_5a4.Data4[6] = '\0';
  local_5a4.Data4[7] = '0';
  local_5b0 = 0;
  if (DAT_005862a0 == '\0') {
    iVar4 = FUN_00402730();
    if (iVar4 < 0) {
      if (DAT_0058629c != (HMODULE)0x0) {
        FreeLibrary(DAT_0058629c);
      }
      DAT_0058629c = (HMODULE)0x0;
      DAT_005862a0 = 0;
      __security_check_cookie(local_c ^ (uint)&local_5c4);
      return;
    }
    DAT_005862a0 = '\x01';
  }
  local_588.ClassGuid.Data1 = 0;
  local_588.ClassGuid.Data2 = 0;
  local_588.ClassGuid.Data3 = 0;
  local_588.ClassGuid.Data4[0] = '\0';
  local_588.ClassGuid.Data4[1] = '\0';
  local_588.ClassGuid.Data4[2] = '\0';
  local_588.ClassGuid.Data4[3] = '\0';
  local_588.ClassGuid.Data4[4] = '\0';
  local_588.ClassGuid.Data4[5] = '\0';
  local_588.ClassGuid.Data4[6] = '\0';
  local_588.ClassGuid.Data4[7] = '\0';
  local_588.DevInst = 0;
  local_588.Reserved = 0;
  local_588.cbSize = 0x1c;
  local_56c[0].cbSize = 0x1c;
  DeviceInfoSet = SetupDiGetClassDevsA(&local_5a4,(PCSTR)0x0,(HWND)0x0,0x12);
  local_5c0 = 0;
  local_5ac = DeviceInfoSet;
  iVar4 = SetupDiEnumDeviceInterfaces(DeviceInfoSet,(PSP_DEVINFO_DATA)0x0,&local_5a4,0,local_56c);
  do {
    if (iVar4 == 0) {
      SetupDiDestroyDeviceInfoList(DeviceInfoSet);
      __security_check_cookie(local_c ^ (uint)&local_5c4);
      return;
    }
    SetupDiGetDeviceInterfaceDetailA
              (DeviceInfoSet,local_56c,(PSP_DEVICE_INTERFACE_DETAIL_DATA_A)0x0,0,&local_5c0,
               (PSP_DEVINFO_DATA)0x0);
    DeviceInterfaceDetailData = _malloc(local_5c0);
    DeviceInterfaceDetailData->cbSize = 5;
    local_5b4 = DeviceInterfaceDetailData;
    BVar5 = SetupDiGetDeviceInterfaceDetailA
                      (DeviceInfoSet,local_56c,DeviceInterfaceDetailData,local_5c0,(PDWORD)0x0,
                       (PSP_DEVINFO_DATA)0x0);
    if (BVar5 != 0) {
      local_5c4 = (char *)0x0;
      BVar5 = SetupDiEnumDeviceInfo(DeviceInfoSet,0,&local_588);
      if (BVar5 != 0) {
LAB_00402a49:
        BVar5 = SetupDiGetDeviceRegistryPropertyA
                          (DeviceInfoSet,&local_588,7,(PDWORD)0x0,local_510,0x100,(PDWORD)0x0);
        if (BVar5 != 0) {
          pcVar11 = "HIDClass";
          pbVar6 = local_510;
          do {
            bVar1 = *pbVar6;
            bVar12 = bVar1 < (byte)*pcVar11;
            if (bVar1 != *pcVar11) {
LAB_00402aa0:
              iVar4 = (1 - (uint)bVar12) - (uint)(bVar12 != 0);
              goto LAB_00402aa5;
            }
            if (bVar1 == 0) break;
            bVar1 = pbVar6[1];
            bVar12 = bVar1 < (byte)pcVar11[1];
            if (bVar1 != pcVar11[1]) goto LAB_00402aa0;
            pbVar6 = pbVar6 + 2;
            pcVar11 = pcVar11 + 2;
          } while (bVar1 != 0);
          iVar4 = 0;
LAB_00402aa5:
          if ((iVar4 != 0) ||
             (BVar5 = SetupDiGetDeviceRegistryPropertyA
                                (DeviceInfoSet,&local_588,9,(PDWORD)0x0,local_510,0x100,(PDWORD)0x0)
             , BVar5 == 0)) break;
          lpFileName = DeviceInterfaceDetailData->DevicePath;
          pcVar11 = CreateFileA(lpFileName,0,3,(LPSECURITY_ATTRIBUTES)0x0,3,0x40000000,(HANDLE)0x0);
          local_5bc = pcVar11;
          if (pcVar11 != (char *)0xffffffff) {
            local_594 = 0xc;
            (*DAT_005861ec)(pcVar11,&local_594);
            if (((param_1 == 0) || (sStack_590 == param_1)) &&
               ((param_2 == 0 || (sStack_58e == param_2)))) {
              local_5c4 = (char *)0x0;
              piVar7 = _calloc(1,0x24);
              piVar2 = piVar7;
              if (local_5a8 != (int *)0x0) {
                local_5a8[8] = (int)piVar7;
                piVar2 = local_5b8;
              }
              local_5b8 = piVar2;
              ppcVar13 = &local_5c4;
              local_5a8 = piVar7;
              cVar3 = (*DAT_005861e0)(pcVar11,ppcVar13);
              if (cVar3 != '\0') {
                iVar4 = (*DAT_00586204)(unaff_ESI,&local_56c[0].Flags);
                if (iVar4 == 0x110000) {
                  *(undefined2 *)(piVar7 + 6) = local_56c[0].InterfaceClassGuid.Data4._2_2_;
                  *(undefined2 *)((int)piVar7 + 0x1a) = local_56c[0].InterfaceClassGuid.Data4._0_2_;
                }
                (*DAT_005861fc)(ppcVar13);
              }
              piVar7[8] = 0;
              if (lpFileName == (CHAR *)0x0) {
                *piVar7 = 0;
              }
              else {
                do {
                  cVar3 = *lpFileName;
                  lpFileName = lpFileName + 1;
                } while (cVar3 != '\0');
                pcVar11 = _calloc((size_t)(lpFileName + (1 - (int)(DeviceInterfaceDetailData + 1))),
                                  1);
                *piVar7 = (int)pcVar11;
                _strncpy(pcVar11,local_5bc + 4,
                         (size_t)(lpFileName + (1 - (int)(DeviceInterfaceDetailData + 1))));
                lpFileName[*piVar7 - (int)(DeviceInterfaceDetailData + 1)] = '\0';
                pcVar11 = local_5c4;
              }
              cVar3 = (*DAT_005861f8)(pcVar11,auStack_418,0x400);
              uStack_26 = 0;
              if (cVar3 != '\0') {
                pwVar8 = __wcsdup(awStack_424);
                piVar7[2] = (int)pwVar8;
              }
              cVar3 = (*DAT_005861f0)(pcVar11,awStack_424,0x400);
              uStack_32 = 0;
              if (cVar3 != '\0') {
                pwVar8 = __wcsdup(awStack_430);
                piVar7[4] = (int)pwVar8;
              }
              cVar3 = (*DAT_00586208)(pcVar11,awStack_430,0x400);
              uStack_12 = 0;
              if (cVar3 != '\0') {
                pwVar8 = __wcsdup(awStack_410);
                piVar7[5] = (int)pwVar8;
              }
              *(short *)(piVar7 + 1) = sStack_590;
              *(short *)((int)piVar7 + 6) = sStack_58e;
              *(undefined2 *)(piVar7 + 3) = uStack_58c;
              piVar7[7] = -1;
              DeviceInfoSet = local_5ac;
              if (((char *)*piVar7 != (char *)0x0) &&
                 (pcVar9 = _strstr((char *)*piVar7,"&mi_"), DeviceInfoSet = local_5ac,
                 pcVar9 != (char *)0x0)) {
                local_5bc = (char *)0x0;
                lVar10 = _strtol(pcVar9 + 4,&local_5bc,0x10);
                piVar7[7] = lVar10;
                DeviceInfoSet = local_5ac;
                if (local_5bc == pcVar9 + 4) {
                  piVar7[7] = -1;
                }
              }
            }
          }
          CloseHandle(pcVar11);
          DeviceInterfaceDetailData = local_5b4;
        }
      }
    }
LAB_00402d24:
    _free(DeviceInterfaceDetailData);
    local_5b0 = local_5b0 + 1;
    local_5c0 = 0;
    iVar4 = SetupDiEnumDeviceInterfaces
                      (DeviceInfoSet,(PSP_DEVINFO_DATA)0x0,&local_5a4,local_5b0,local_56c);
  } while( true );
  local_5c4 = local_5c4 + 1;
  BVar5 = SetupDiEnumDeviceInfo(DeviceInfoSet,(DWORD)local_5c4,&local_588);
  if (BVar5 == 0) goto LAB_00402d24;
  goto LAB_00402a49;
}


