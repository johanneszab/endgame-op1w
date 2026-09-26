/* v1/hid_decomp.c -- HID transport layer of the v1 configuration tool.
 *
 * Program: Endgame_Gear_OP1w_4k_Configuration_Tool_v1.03.exe
 *   (OP1w 4k **v1** vendor tool; Ghidra project OP1w, image base 0x00400000)
 * Extracted with Ghidra 12.1.4 headless, scripts in git/re/ghidra_scripts.
 * Evidence class for everything in this file: [BIN].
 *
 * Addresses covered:
 *   FUN_004037c0  feature-report WRITE wrapper (HidD_SetFeature + retry/back-off)
 *                 -- v2 counterpart FUN_00403740
 *   FUN_00403890  feature-report READ  wrapper (HidD_GetFeature + retry/back-off)
 *                 -- v2 counterpart FUN_00403810
 *   FUN_004035a0  device discovery / open (HidD_GetPreparsedData, collection match)
 *   hid_open        @ 00402e40
 *   hid_open_path   @ 00402f30
 *   hid_enumerate   @ 00402950
 *
 * Globals:
 *   DAT_00583444   the feature-report HidD handle
 *                  (v2 counterpart DAT_005862a4)
 *   DAT_0057cce0   the critical section serialising ALL device access
 *                  (v2 counterpart DAT_0057fc80)
 *
 * The retry rule is byte-for-byte the v2 rule: on failure, if GetLastError() is
 * one of 0x15, 0x17, 0x1D, 0x57, 0x65B, restore the saved 64-byte report and
 * retry up to 3 more times with Sleep(0x32) between attempts.
 */

/* ============ FUN_004037c0 @ 004037c0  (req 004037c0) ============ */
/* calls: FUN_00416b07@00416b07 Sleep@EXTERNAL:00000023 _memset@0050b7b0 GetLastError@EXTERNAL:00000011 FID_conflict:_memcpy@0050e580 FUN_00416afc@00416afc HidD_SetFeature@EXTERNAL:00000005 */
/* called by: FUN_00403a50@00403a50 FUN_00403b50@00403b50 FUN_00403c50@00403c50 FUN_00403d90@00403d90 FUN_00404750@00404750 FUN_00404bb0@00404bb0 FUN_004050c0@004050c0 FUN_00405200@00405200 FUN_00405340@00405340 FUN_00405480@00405480 FUN_004055a0@004055a0 FUN_004056c0@004056c0 */

char FUN_004037c0(void)

{
  char cVar1;
  void *_Dst;
  DWORD DVar2;
  void *unaff_EDI;
  int local_8;
  
  _Dst = (void *)FUN_00416b07(0x40);
  _memset(_Dst,0,0x40);
  FID_conflict__memcpy(_Dst,unaff_EDI,0x40);
  cVar1 = '\0';
  if (DAT_00583444 != 0) {
    cVar1 = HidD_SetFeature(DAT_00583444,unaff_EDI,0x40);
  }
  if ((cVar1 == '\0') &&
     ((((DVar2 = GetLastError(), DVar2 == 0x15 || (DVar2 == 0x17)) || (DVar2 == 0x1d)) ||
      ((DVar2 == 0x57 || (DVar2 == 0x65b)))))) {
    local_8 = 1;
    do {
      FID_conflict__memcpy(unaff_EDI,_Dst,0x40);
      cVar1 = '\0';
      if (DAT_00583444 != 0) {
        cVar1 = HidD_SetFeature(DAT_00583444,unaff_EDI,0x40);
      }
      if (cVar1 == '\x01') break;
      Sleep(0x32);
      local_8 = local_8 + 1;
    } while (local_8 < 4);
    if (cVar1 == '\0') {
      return '\0';
    }
  }
  FUN_00416afc(_Dst);
  return cVar1;
}


/* ============ FUN_00403890 @ 00403890  (req 403890) ============ */
/* calls: FUN_00416b07@00416b07 Sleep@EXTERNAL:00000023 _memset@0050b7b0 GetLastError@EXTERNAL:00000011 FID_conflict:_memcpy@0050e580 HidD_GetFeature@EXTERNAL:00000004 FUN_00416afc@00416afc */
/* called by: FUN_00403a50@00403a50 FUN_00403b50@00403b50 FUN_00403c50@00403c50 FUN_00403d90@00403d90 FUN_00404750@00404750 FUN_00404890@00404890 FUN_00404bb0@00404bb0 FUN_004050c0@004050c0 FUN_00405200@00405200 FUN_00405340@00405340 FUN_00405480@00405480 FUN_004055a0@004055a0 FUN_004056c0@004056c0 */

char FUN_00403890(void)

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
  _Dst = (void *)FUN_00416b07(unaff_ESI);
  _memset(_Dst,0,unaff_ESI);
  FID_conflict__memcpy(_Dst,unaff_EDI,unaff_ESI);
  local_c = '\0';
  if (DAT_00583444 != 0) {
    local_c = HidD_GetFeature(DAT_00583444,unaff_EDI,unaff_ESI);
  }
  if (local_c == '\0') {
    DVar1 = GetLastError();
    Sleep(0x32);
    if ((((DVar1 == 0x15) || (DVar1 == 0x17)) || (DVar1 == 0x1d)) ||
       ((DVar1 == 0x57 || (DVar1 == 0x65b)))) {
      iVar2 = 1;
      do {
        local_c = '\0';
        if (DAT_00583444 != 0) {
          local_c = HidD_GetFeature(DAT_00583444,unaff_EDI,unaff_ESI);
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
      if (DAT_00583444 != 0) {
        local_c = HidD_GetFeature(DAT_00583444,unaff_EDI,unaff_ESI);
      }
      if ((local_c != '\0') && (*(char *)((int)unaff_EDI + 1) == '\x01')) break;
      local_10 = local_10 + 1;
      if (2 < local_10) {
        FUN_00416afc(_Dst);
        return local_c;
      }
    }
  }
  FUN_00416afc(_Dst);
  return local_c;
}


/* ============ FUN_004035a0 @ 004035a0  (req 004035a0) ============ */
/* calls: CreateFileW@EXTERNAL:00000022 SetupDiEnumDeviceInterfaces@EXTERNAL:00000245 SetupDiGetDeviceInterfaceDetailW@EXTERNAL:00000247 HidD_GetAttributes@EXTERNAL:00000006 SetupDiDestroyDeviceInfoList@EXTERNAL:00000248 SetupDiGetClassDevsW@EXTERNAL:00000246 _free@0050a6e1 __security_check_cookie@0050a225 HidD_GetHidGuid@EXTERNAL:00000007 HidP_GetCaps@EXTERNAL:00000002 _malloc@0050a7c9 HidD_FreePreparsedData@EXTERNAL:00000001 FUN_004025f0@004025f0 HidD_GetPreparsedData@EXTERNAL:00000003 */
/* called by: FUN_00409a70@00409a70 FUN_00409b00@00409b00 FUN_00409b70@00409b70 FUN_004149a0@004149a0 FUN_00416530@00416530 FUN_00415500@00415500 FUN_004168d0@004168d0 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl FUN_004035a0(uint param_1,uint param_2)

{
  WCHAR *pWVar1;
  WCHAR WVar2;
  HDEVINFO DeviceInfoSet;
  BOOL BVar3;
  PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData;
  int iVar4;
  WCHAR *pWVar5;
  undefined4 local_44;
  undefined4 local_40;
  DWORD local_3c;
  DWORD local_38;
  char local_31;
  _SP_DEVICE_INTERFACE_DATA local_30;
  undefined4 local_14;
  ushort local_10;
  ushort local_e;
  ushort local_c;
  uint local_8;
  
  local_8 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  DAT_00583444 = (HANDLE)0x0;
  local_3c = 0;
  HidD_GetHidGuid(&DAT_005833dc);
  DAT_005833d8 = SetupDiGetClassDevsW((GUID *)&DAT_005833dc,(PCWSTR)0x0,(HWND)0x0,0x12);
  local_30.cbSize = 0x1c;
  local_38 = 0;
  local_31 = '\0';
  do {
    _DAT_005833b4 = 0;
    BVar3 = SetupDiEnumDeviceInterfaces
                      (DAT_005833d8,(PSP_DEVINFO_DATA)0x0,(GUID *)&DAT_005833dc,local_38,&local_30);
    if (BVar3 == 0) {
      local_31 = '\x01';
    }
    else {
      _DAT_005832b0 = 5;
      SetupDiGetDeviceInterfaceDetailW
                (DAT_005833d8,&local_30,(PSP_DEVICE_INTERFACE_DETAIL_DATA_W)0x0,0,&local_3c,
                 (PSP_DEVINFO_DATA)0x0);
      DeviceInterfaceDetailData = _malloc(local_3c);
      DeviceInfoSet = DAT_005833d8;
      DeviceInterfaceDetailData->cbSize = 6;
      SetupDiGetDeviceInterfaceDetailW
                (DeviceInfoSet,&local_30,DeviceInterfaceDetailData,local_3c,(PDWORD)&DAT_005833d4,
                 (PSP_DEVINFO_DATA)0x0);
      DAT_00583444 = CreateFileW(DeviceInterfaceDetailData->DevicePath,0xc0000000,3,
                                 (LPSECURITY_ATTRIBUTES)0x0,3,0,(HANDLE)0x0);
      local_14 = 0xc;
      HidD_GetAttributes(DAT_00583444,&local_14);
      if ((local_10 == param_1) && (local_e == param_2)) {
        HidD_GetPreparsedData(DAT_00583444,&local_40);
        HidP_GetCaps(local_40,&DAT_005833f0);
        HidD_FreePreparsedData(local_40);
        HidD_GetPreparsedData(DAT_00583444,&local_44);
        HidP_GetCaps(local_44,&DAT_005833f0);
        HidD_FreePreparsedData(local_44);
        if ((DAT_005833f2 == -0xff) && (DAT_005833f0 == 2)) {
          _DAT_005832ac = (uint)local_c;
          _DAT_005833b4 = 1;
          pWVar1 = DeviceInterfaceDetailData->DevicePath;
          if (pWVar1 == (WCHAR *)0x0) {
            iVar4 = 0;
          }
          else {
            pWVar5 = pWVar1;
            do {
              WVar2 = *pWVar5;
              pWVar5 = pWVar5 + 1;
            } while (WVar2 != L'\0');
            iVar4 = (int)pWVar5 - (int)(DeviceInterfaceDetailData + 1) >> 1;
          }
          FUN_004025f0(&DAT_0058439c,pWVar1,iVar4);
          _free(DeviceInterfaceDetailData);
          __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
          return;
        }
      }
      _free(DeviceInterfaceDetailData);
    }
    local_38 = local_38 + 1;
    if (local_31 != '\0') {
      SetupDiDestroyDeviceInfoList(DAT_005833d8);
      __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
      return;
    }
  } while( true );
}


/* ============ hid_open @ 00402e40  (req 00402e40) ============ */
/* calls: hid_free_enumeration@00402df0 hid_open_path@00402f30 hid_enumerate@00402950 */
/* called by: FUN_004142b0@004142b0 */

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
  
                    /* 0x2e40  12  hid_open */
  local_c = 0;
  puVar3 = (undefined4 *)hid_enumerate(param_1,param_2);
  puVar7 = puVar3;
  if (puVar3 != (undefined4 *)0x0) {
    while ((((*(short *)(puVar7 + 1) != param_1 || (*(short *)((int)puVar7 + 6) != param_2)) ||
            (*(short *)(puVar7 + 6) != -0xfe)) || (*(short *)((int)puVar7 + 0x1a) != 1))) {
LAB_00402ecf:
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
LAB_00402ec6:
          iVar5 = (1 - (uint)bVar8) - (uint)(bVar8 != 0);
          goto LAB_00402ecb;
        }
        if (uVar2 == 0) break;
        uVar2 = puVar4[1];
        bVar8 = uVar2 < puVar6[1];
        if (uVar2 != puVar6[1]) goto LAB_00402ec6;
        puVar4 = puVar4 + 2;
        puVar6 = puVar6 + 2;
      } while (uVar2 != 0);
      iVar5 = 0;
LAB_00402ecb:
      if (iVar5 != 0) goto LAB_00402ecf;
    }
    if ((LPCSTR)*puVar7 != (LPCSTR)0x0) {
      local_c = hid_open_path((LPCSTR)*puVar7);
    }
  }
  hid_free_enumeration(puVar3);
  return local_c;
}


/* ============ hid_open_path @ 00402f30  (req 00402f30) ============ */
/* calls: FUN_004027a0@004027a0 _free@0050a6e1 __security_check_cookie@0050a225 CreateFileA@EXTERNAL:00000012 FUN_00402730@00402730 _calloc@0050a789 _malloc@0050a7c9 FreeLibrary@EXTERNAL:00000013 CreateEventW@EXTERNAL:0000001d CloseHandle@EXTERNAL:00000020 LocalFree@EXTERNAL:00000021 */
/* called by: hid_open@00402e40 */

void __cdecl hid_open_path(LPCSTR param_1)

{
  char cVar1;
  int iVar2;
  undefined4 *_Memory;
  HANDLE pvVar3;
  void *pvVar4;
  undefined4 unaff_ESI;
  undefined4 local_54;
  undefined1 auStack_50 [56];
  uint uStack_18;
  uint local_c;
  
                    /* 0x2f30  13  hid_open_path */
  local_c = DAT_0057b3b0 ^ (uint)&local_54;
  local_54 = 0;
  if (DAT_00583440 == '\0') {
    iVar2 = FUN_004027a0();
    if (iVar2 < 0) {
      if (DAT_0058343c != (HMODULE)0x0) {
        FreeLibrary(DAT_0058343c);
      }
      DAT_0058343c = (HMODULE)0x0;
      DAT_00583440 = 0;
      __security_check_cookie(local_c ^ (uint)&local_54);
      return;
    }
    DAT_00583440 = '\x01';
  }
  _Memory = _calloc(1,0x34);
  *_Memory = 0xffffffff;
  _Memory[1] = 1;
  *(undefined2 *)(_Memory + 2) = 0;
  _Memory[3] = 0;
  _Memory[4] = 0;
  _Memory[5] = 0;
  _Memory[6] = 0;
  _Memory[7] = 0;
  _Memory[8] = 0;
  _Memory[9] = 0;
  _Memory[10] = 0;
  _Memory[0xb] = 0;
  _Memory[0xc] = 0;
  pvVar3 = CreateEventW((LPSECURITY_ATTRIBUTES)0x0,0,0,(LPCWSTR)0x0);
  _Memory[0xc] = pvVar3;
  pvVar3 = CreateFileA(param_1,0xc0000000,3,(LPSECURITY_ATTRIBUTES)0x0,3,0x40000000,(HANDLE)0x0);
  *_Memory = pvVar3;
  if (pvVar3 != (HANDLE)0xffffffff) {
    cVar1 = (*DAT_00583288)(pvVar3,0x40);
    if (cVar1 != '\0') {
      cVar1 = (*DAT_00583280)(*_Memory,&local_54);
      if (cVar1 != '\0') {
        iVar2 = (*DAT_005832a4)(local_54,auStack_50);
        if (iVar2 == 0x110000) {
          *(undefined2 *)(_Memory + 2) = local_54._2_2_;
          _Memory[3] = local_54 & 0xffff;
          (*DAT_0058329c)(unaff_ESI);
          pvVar4 = _malloc(_Memory[3]);
          _Memory[7] = pvVar4;
          __security_check_cookie(uStack_18 ^ (uint)&stack0xffffffa0);
          return;
        }
        FUN_00402730();
        (*DAT_0058329c)(unaff_ESI);
        goto LAB_0040304f;
      }
    }
  }
  FUN_00402730();
LAB_0040304f:
  CloseHandle((HANDLE)_Memory[0xc]);
  CloseHandle((HANDLE)*_Memory);
  LocalFree((HLOCAL)_Memory[4]);
  _free((void *)_Memory[7]);
  _free(_Memory);
  __security_check_cookie(local_c ^ (uint)&local_54);
  return;
}


/* ============ hid_enumerate @ 00402950  (req 00402950) ============ */
/* calls: _strncpy@0050abd0 SetupDiGetDeviceRegistryPropertyA@EXTERNAL:00000243 CreateFileA@EXTERNAL:00000012 SetupDiEnumDeviceInterfaces@EXTERNAL:00000245 SetupDiDestroyDeviceInfoList@EXTERNAL:00000248 CloseHandle@EXTERNAL:00000020 SetupDiGetClassDevsA@EXTERNAL:00000241 FUN_004027a0@004027a0 _free@0050a6e1 SetupDiGetDeviceInterfaceDetailA@EXTERNAL:00000244 __security_check_cookie@0050a225 __wcsdup@0050acf4 _strtol@0050ab98 _malloc@0050a7c9 _calloc@0050a789 SetupDiEnumDeviceInfo@EXTERNAL:00000242 _strstr@0050a860 FreeLibrary@EXTERNAL:00000013 */
/* called by: hid_open@00402e40 */

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
  
                    /* 0x2950  2  hid_enumerate */
  local_c = DAT_0057b3b0 ^ (uint)&local_5c4;
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
  if (DAT_00583440 == '\0') {
    iVar4 = FUN_004027a0();
    if (iVar4 < 0) {
      if (DAT_0058343c != (HMODULE)0x0) {
        FreeLibrary(DAT_0058343c);
      }
      DAT_0058343c = (HMODULE)0x0;
      DAT_00583440 = 0;
      __security_check_cookie(local_c ^ (uint)&local_5c4);
      return;
    }
    DAT_00583440 = '\x01';
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
LAB_00402ab9:
        BVar5 = SetupDiGetDeviceRegistryPropertyA
                          (DeviceInfoSet,&local_588,7,(PDWORD)0x0,local_510,0x100,(PDWORD)0x0);
        if (BVar5 != 0) {
          pcVar11 = "HIDClass";
          pbVar6 = local_510;
          do {
            bVar1 = *pbVar6;
            bVar12 = bVar1 < (byte)*pcVar11;
            if (bVar1 != *pcVar11) {
LAB_00402b10:
              iVar4 = (1 - (uint)bVar12) - (uint)(bVar12 != 0);
              goto LAB_00402b15;
            }
            if (bVar1 == 0) break;
            bVar1 = pbVar6[1];
            bVar12 = bVar1 < (byte)pcVar11[1];
            if (bVar1 != pcVar11[1]) goto LAB_00402b10;
            pbVar6 = pbVar6 + 2;
            pcVar11 = pcVar11 + 2;
          } while (bVar1 != 0);
          iVar4 = 0;
LAB_00402b15:
          if ((iVar4 != 0) ||
             (BVar5 = SetupDiGetDeviceRegistryPropertyA
                                (DeviceInfoSet,&local_588,9,(PDWORD)0x0,local_510,0x100,(PDWORD)0x0)
             , BVar5 == 0)) break;
          lpFileName = DeviceInterfaceDetailData->DevicePath;
          pcVar11 = CreateFileA(lpFileName,0,3,(LPSECURITY_ATTRIBUTES)0x0,3,0x40000000,(HANDLE)0x0);
          local_5bc = pcVar11;
          if (pcVar11 != (char *)0xffffffff) {
            local_594 = 0xc;
            (*DAT_0058328c)(pcVar11,&local_594);
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
              cVar3 = (*DAT_00583280)(pcVar11,ppcVar13);
              if (cVar3 != '\0') {
                iVar4 = (*DAT_005832a4)(unaff_ESI,&local_56c[0].Flags);
                if (iVar4 == 0x110000) {
                  *(undefined2 *)(piVar7 + 6) = local_56c[0].InterfaceClassGuid.Data4._2_2_;
                  *(undefined2 *)((int)piVar7 + 0x1a) = local_56c[0].InterfaceClassGuid.Data4._0_2_;
                }
                (*DAT_0058329c)(ppcVar13);
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
              cVar3 = (*DAT_00583298)(pcVar11,auStack_418,0x400);
              uStack_26 = 0;
              if (cVar3 != '\0') {
                pwVar8 = __wcsdup(awStack_424);
                piVar7[2] = (int)pwVar8;
              }
              cVar3 = (*DAT_00583290)(pcVar11,awStack_424,0x400);
              uStack_32 = 0;
              if (cVar3 != '\0') {
                pwVar8 = __wcsdup(awStack_430);
                piVar7[4] = (int)pwVar8;
              }
              cVar3 = (*DAT_005832a8)(pcVar11,awStack_430,0x400);
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
LAB_00402d94:
    _free(DeviceInterfaceDetailData);
    local_5b0 = local_5b0 + 1;
    local_5c0 = 0;
    iVar4 = SetupDiEnumDeviceInterfaces
                      (DeviceInfoSet,(PSP_DEVINFO_DATA)0x0,&local_5a4,local_5b0,local_56c);
  } while( true );
  local_5c4 = local_5c4 + 1;
  BVar5 = SetupDiEnumDeviceInfo(DeviceInfoSet,(DWORD)local_5c4,&local_588);
  if (BVar5 == 0) goto LAB_00402d94;
  goto LAB_00402ab9;
}


/* ---- index: callers of the HID APIs (FindApi.java) ----

=== HidD_SetFeature ===
  symbol HidD_SetFeature @ EXTERNAL:00000005  type=Function
    ref from 005301dc (no func)
    ref from FUN_004037c0 @ 004037c0 (site 004037f7)
    ref from FUN_004037c0 @ 004037c0 (site 0040384b)
    ref from FUN_00404890 @ 00404890 (site 0040496b)
      -- callers of FUN_004037c0 @ 004037c0:
         FUN_00403a50 @ 00403a50
         FUN_00403b50 @ 00403b50
         FUN_00403c50 @ 00403c50
         FUN_00403d90 @ 00403d90
         FUN_00404750 @ 00404750
         FUN_00404bb0 @ 00404bb0
         FUN_004050c0 @ 004050c0
         FUN_00405200 @ 00405200
         FUN_00405340 @ 00405340
         FUN_00405480 @ 00405480
         FUN_004055a0 @ 004055a0
         FUN_004056c0 @ 004056c0
      -- callers of FUN_00404890 @ 00404890:
         FUN_00416200 @ 00416200
=== HidD_GetFeature ===
  symbol HidD_GetFeature @ EXTERNAL:00000004  type=Function
    ref from 005301d8 (no func)
    ref from FUN_00403890 @ 00403890 (site 004038c9)
    ref from FUN_00403890 @ 00403890 (site 0040391a)
    ref from FUN_00403890 @ 00403890 (site 00403994)
      -- callers of FUN_00403890 @ 00403890:
         FUN_00403a50 @ 00403a50
         FUN_00403b50 @ 00403b50
         FUN_00403c50 @ 00403c50
         FUN_00403d90 @ 00403d90
         FUN_00404750 @ 00404750
         FUN_00404890 @ 00404890
         FUN_00404bb0 @ 00404bb0
         FUN_004050c0 @ 004050c0
         FUN_00405200 @ 00405200
         FUN_00405340 @ 00405340
         FUN_00405480 @ 00405480
         FUN_004055a0 @ 004055a0
         FUN_004056c0 @ 004056c0
=== HidD_GetPreparsedData ===
  symbol HidD_GetPreparsedData @ EXTERNAL:00000003  type=Function
    ref from 005301d4 (no func)
    ref from FUN_004035a0 @ 004035a0 (site 004036b6)
    ref from FUN_004035a0 @ 004035a0 (site 004036e0)
      -- callers of FUN_004035a0 @ 004035a0:
         FUN_00409a70 @ 00409a70
         FUN_00409b00 @ 00409b00
         FUN_00409b70 @ 00409b70
         FUN_004149a0 @ 004149a0
         FUN_00416530 @ 00416530
         FUN_00415500 @ 00415500
         FUN_004168d0 @ 004168d0

*/


