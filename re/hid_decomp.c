/* Decompiled callers of HidD_*/HidP_* in Endgame_Gear_OP1w_4k_v2_Configuration_Tool_v1_02-61846009.exe */

/* ======================================================
 * FUN_00403740 @ 00403740
 * ====================================================== */

char FUN_00403740(void)

{
  char cVar1;
  void *_Dst;
  DWORD DVar2;
  void *unaff_EDI;
  int local_8;
  
  _Dst = (void *)FUN_0041a157(0x40);
  _memset(_Dst,0,0x40);
  FID_conflict__memcpy(_Dst,unaff_EDI,0x40);
  cVar1 = '\0';
  if (DAT_005862a4 != 0) {
    cVar1 = HidD_SetFeature(DAT_005862a4,unaff_EDI,0x40);
  }
  if ((cVar1 == '\0') &&
     ((((DVar2 = GetLastError(), DVar2 == 0x15 || (DVar2 == 0x17)) || (DVar2 == 0x1d)) ||
      ((DVar2 == 0x57 || (DVar2 == 0x65b)))))) {
    local_8 = 1;
    do {
      FID_conflict__memcpy(unaff_EDI,_Dst,0x40);
      cVar1 = '\0';
      if (DAT_005862a4 != 0) {
        cVar1 = HidD_SetFeature(DAT_005862a4,unaff_EDI,0x40);
      }
      if (cVar1 == '\x01') break;
      Sleep(0x32);
      local_8 = local_8 + 1;
    } while (local_8 < 4);
    if (cVar1 == '\0') {
      return '\0';
    }
  }
  FUN_0041a14c(_Dst);
  return cVar1;
}



/* ======================================================
 * FUN_00404860 @ 00404860
 * ====================================================== */

void __thiscall FUN_00404860(void *this,ushort param_1)

{
  char cVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined4 *local_e0;
  undefined1 local_d8 [64];
  undefined1 local_98;
  undefined1 local_97 [63];
  undefined4 local_58;
  undefined2 local_54;
  char local_52;
  undefined4 local_48 [12];
  uint local_18;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052f0cb;
  local_10 = ExceptionList;
  local_18 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  iVar3 = 0;
  local_8 = 0;
  local_98 = 0;
  _memset(local_97,0,0x3f);
  _memset(local_d8,0,0x40);
  local_e0 = (undefined4 *)FUN_004049f0((int)this);
  while( true ) {
    _memset((void *)((int)&local_58 + 1),0,0x3f);
    local_52 = (char)iVar3 + '\x01';
    local_58 = 0x1c0f16a1;
    local_54 = 0;
    puVar4 = local_e0;
    puVar5 = local_48;
    for (iVar2 = 7; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar5 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar5 = puVar5 + 1;
    }
    cVar1 = '\0';
    if (DAT_005862a4 != 0) {
      cVar1 = HidD_SetFeature(DAT_005862a4,&local_58,0x40);
    }
    Sleep((uint)param_1);
    if (cVar1 == '\0') break;
    _memset(&local_98,0,0x40);
    local_98 = 0xa1;
    FUN_00403810();
    Sleep(100);
    local_e0 = local_e0 + 7;
    iVar3 = iVar3 + 1;
    if (1 < iVar3) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
LAB_004049be:
      ExceptionList = local_10;
      __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
      return;
    }
  }
  LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  goto LAB_004049be;
}



/* ======================================================
 * FUN_00403810 @ 00403810
 * ====================================================== */

char FUN_00403810(void)

{
  uint in_EAX;
  void *_Dst;
  DWORD DVar1;
  uint dwMilliseconds;
  int iVar2;
  size_t unaff_ESI;
  void *unaff_EDI;
  int local_10;
  char local_c;
  
  dwMilliseconds = in_EAX & 0xffff;
  _Dst = (void *)FUN_0041a157(unaff_ESI);
  _memset(_Dst,0,unaff_ESI);
  FID_conflict__memcpy(_Dst,unaff_EDI,unaff_ESI);
  local_c = '\0';
  if (DAT_005862a4 != 0) {
    local_c = HidD_GetFeature(DAT_005862a4,unaff_EDI,unaff_ESI);
  }
  if (local_c == '\0') {
    DVar1 = GetLastError();
    Sleep(0x32);
    if ((((DVar1 == 0x15) || (DVar1 == 0x17)) || (DVar1 == 0x1d)) ||
       ((DVar1 == 0x57 || (DVar1 == 0x65b)))) {
      iVar2 = 1;
      do {
        local_c = '\0';
        if (DAT_005862a4 != 0) {
          local_c = HidD_GetFeature(DAT_005862a4,unaff_EDI,unaff_ESI);
        }
        if (local_c != '\0') break;
        Sleep(0x32);
        iVar2 = iVar2 + 1;
      } while (iVar2 < 3);
    }
  }
  else if (*(char *)((int)unaff_EDI + 1) == '\x03') {
    local_10 = 1;
    while( true ) {
      dwMilliseconds = dwMilliseconds + 200;
      Sleep(dwMilliseconds);
      FID_conflict__memcpy(unaff_EDI,_Dst,unaff_ESI);
      local_c = '\0';
      if (DAT_005862a4 != 0) {
        local_c = HidD_GetFeature(DAT_005862a4,unaff_EDI,unaff_ESI);
      }
      if ((local_c != '\0') && (*(char *)((int)unaff_EDI + 1) == '\x01')) break;
      local_10 = local_10 + 1;
      if (2 < local_10) {
        FUN_0041a14c(_Dst);
        return local_c;
      }
    }
  }
  FUN_0041a14c(_Dst);
  return local_c;
}



/* ======================================================
 * FUN_004034e0 @ 004034e0
 * ====================================================== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004034e0(uint param_1,uint param_2)

{
  HDEVINFO DeviceInfoSet;
  char cVar1;
  int iVar2;
  BOOL BVar3;
  DWORD DVar4;
  PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData;
  DWORD local_40;
  DWORD local_3c;
  undefined4 local_38;
  char local_31;
  _SP_DEVICE_INTERFACE_DATA local_30;
  undefined4 local_14;
  uint local_10;
  uint local_c;
  uint local_8;
  
  local_8 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  DAT_005862a4 = (HANDLE)0x0;
  local_38 = 0;
  local_14 = 0xc;
  local_10 = 0;
  local_c = 0;
  local_30.cbSize = 0x1c;
  local_30.InterfaceClassGuid.Data1 = 0;
  local_30.InterfaceClassGuid.Data2 = 0;
  local_30.InterfaceClassGuid.Data3 = 0;
  local_30.InterfaceClassGuid.Data4[0] = '\0';
  local_30.InterfaceClassGuid.Data4[1] = '\0';
  local_30.InterfaceClassGuid.Data4[2] = '\0';
  local_30.InterfaceClassGuid.Data4[3] = '\0';
  local_30.InterfaceClassGuid.Data4[4] = '\0';
  local_30.InterfaceClassGuid.Data4[5] = '\0';
  local_30.InterfaceClassGuid.Data4[6] = '\0';
  local_30.InterfaceClassGuid.Data4[7] = '\0';
  local_30.Flags = 0;
  local_30.Reserved = 0;
  local_31 = '\0';
  local_3c = 0;
  HidD_GetHidGuid(&DAT_0058623c);
  DAT_00586238 = SetupDiGetClassDevsW((GUID *)&DAT_0058623c,(PCWSTR)0x0,(HWND)0x0,0x12);
  if (DAT_00586238 == (HDEVINFO)0xffffffff) {
    __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
    return;
  }
  local_40 = 0;
  iVar2 = SetupDiEnumDeviceInterfaces
                    (DAT_00586238,(PSP_DEVINFO_DATA)0x0,(GUID *)&DAT_0058623c,0,&local_30);
  do {
    if (iVar2 == 0) {
LAB_00403715:
      SetupDiDestroyDeviceInfoList(DAT_00586238);
      __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
      return;
    }
    BVar3 = SetupDiGetDeviceInterfaceDetailW
                      (DAT_00586238,&local_30,(PSP_DEVICE_INTERFACE_DETAIL_DATA_W)0x0,0,&local_3c,
                       (PSP_DEVINFO_DATA)0x0);
    if (((BVar3 != 0) || (DVar4 = GetLastError(), DVar4 == 0x7a)) &&
       (DeviceInterfaceDetailData = _malloc(local_3c), DeviceInfoSet = DAT_00586238,
       DeviceInterfaceDetailData != (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)0x0)) {
      DeviceInterfaceDetailData->cbSize = 6;
      BVar3 = SetupDiGetDeviceInterfaceDetailW
                        (DeviceInfoSet,&local_30,DeviceInterfaceDetailData,local_3c,
                         (PDWORD)&DAT_00586234,(PSP_DEVINFO_DATA)0x0);
      if (BVar3 != 0) {
        DAT_005862a4 = CreateFileW(DeviceInterfaceDetailData->DevicePath,0xc0000000,3,
                                   (LPSECURITY_ATTRIBUTES)0x0,3,0,(HANDLE)0x0);
        if (DAT_005862a4 != (HANDLE)0xffffffff) {
          cVar1 = HidD_GetAttributes(DAT_005862a4,&local_14);
          if (((cVar1 != '\0') && ((local_10 & 0xffff) == param_1)) && (local_10 >> 0x10 == param_2)
             ) {
            cVar1 = HidD_GetPreparsedData(DAT_005862a4,&local_38);
            if (cVar1 == '\0') {
              GetLastError();
            }
            else {
              HidP_GetCaps(local_38,&DAT_00586250);
              if ((DAT_00586252 == -0xff) && (DAT_00586250 == 2)) {
                _DAT_00586210 = local_c & 0xffff;
                _DAT_00586214 = 1;
                local_31 = '\x01';
                FUN_00402620(&DAT_00587228,DeviceInterfaceDetailData->DevicePath);
              }
              HidD_FreePreparsedData(local_38);
              local_38 = 0;
            }
            _free(DeviceInterfaceDetailData);
            if (local_31 == '\0') {
              CloseHandle(DAT_005862a4);
              DAT_005862a4 = (HANDLE)0x0;
              goto LAB_004036ed;
            }
            goto LAB_00403715;
          }
          CloseHandle(DAT_005862a4);
        }
      }
      _free(DeviceInterfaceDetailData);
    }
LAB_004036ed:
    local_40 = local_40 + 1;
    iVar2 = SetupDiEnumDeviceInterfaces
                      (DAT_00586238,(PSP_DEVINFO_DATA)0x0,(GUID *)&DAT_0058623c,local_40,&local_30);
  } while( true );
}


