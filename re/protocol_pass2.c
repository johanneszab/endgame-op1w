/* index

--- callers of 0x004034e0 (FUN_004034e0) ---
    FUN_00409980                 @ 00409980   (call site 004099b3)
    FUN_00409980                 @ 00409980   (call site 004099c8)
    FUN_00409a10                 @ 00409a10   (call site 00409a43)
    FUN_00409a80                 @ 00409a80   (call site 00409ab9)
    FUN_00409a80                 @ 00409a80   (call site 00409af2)
    FUN_00417260                 @ 00417260   (call site 0041778b)
    FUN_004198f0                 @ 004198f0   (call site 00419907)
    FUN_004198f0                 @ 004198f0   (call site 00419930)
    FUN_00417dd0                 @ 00417dd0   (call site 00417efb)
    FUN_00417dd0                 @ 00417dd0   (call site 00418155)
    FUN_00417dd0                 @ 00417dd0   (call site 004181bd)
    FUN_00417dd0                 @ 00417dd0   (call site 004182b8)
    FUN_00417dd0                 @ 00417dd0   (call site 0041830c)
    FUN_00419f10                 @ 00419f10   (call site 00419fd6)
    total new: 7

--- callers of 0x004049f0 (FUN_004049f0) ---
    FUN_00404860                 @ 00404860   (call site 004048e9)
    total new: 1

--- callers of 0x00404860 (FUN_00404860) ---
    FUN_00418dd0                 @ 00418dd0   (call site 00418f12)
    FUN_00418dd0                 @ 00418dd0   (call site 00419019)
    total new: 1

--- callers of 0x00404ce0 (FUN_00404ce0) ---
    FUN_00404720                 @ 00404720   (call site 004047a8)
    total new: 1

--- callers of 0x004041e0 (FUN_004041e0) ---
    FUN_00403d10                 @ 00403d10   (call site 00403e0a)
    FUN_00417260                 @ 00417260   (call site 0041759d)
    FUN_00417260                 @ 00417260   (call site 0041776f)
    total new: 1

*/

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



/* ======================================================
 * FUN_00409980 @ 00409980
 * ====================================================== */

void __fastcall FUN_00409980(int param_1)

{
  char cVar1;
  int iVar2;
  
  if (*(int *)(param_1 + 300) != 0) {
    return;
  }
  *(undefined4 *)(param_1 + 300) = 1;
  if (DAT_0057fc78 == '\x11') {
    iVar2 = FUN_004034e0(0x3367,0x1970);
    if (iVar2 == 0) goto LAB_004099eb;
    DAT_005862a2 = 1;
  }
  else {
    iVar2 = FUN_004034e0(0x3367,0x1970);
  }
  if ((iVar2 != 0) &&
     ((cVar1 = FUN_00405480(), cVar1 == '\x01' || (cVar1 = FUN_004055a0(), cVar1 == '\x01')))) {
    FUN_004039d0();
  }
LAB_004099eb:
  *(undefined4 *)(param_1 + 300) = 0;
  AfxMessageBox(L"Hold LMB, RMB and MMB for 3 seconds",0,0);
  return;
}



/* ======================================================
 * FUN_00409a10 @ 00409a10
 * ====================================================== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00409a10(void)

{
  char cVar1;
  int iVar2;
  
  if (DAT_005862a4 != (HANDLE)0x0) {
    CloseHandle(DAT_005862a4);
  }
  _DAT_00586214 = 0;
  _DAT_00586218 = 0;
  iVar2 = FUN_004034e0(0x3367,0x1970);
  if (iVar2 != 0) {
    iVar2 = 0;
    do {
      cVar1 = FUN_00403ad0();
      if (cVar1 == '\x01') {
        return;
      }
      if (cVar1 == '\x03') {
        return;
      }
      if (cVar1 == '\b') {
        FUN_004055a0();
        Sleep(100);
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < 2);
  }
  return;
}



/* ======================================================
 * FUN_00409a80 @ 00409a80
 * ====================================================== */

void __fastcall FUN_00409a80(int param_1)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined1 local_28;
  undefined4 local_27;
  undefined4 local_23;
  undefined4 local_1f;
  undefined2 local_1b;
  undefined1 local_19;
  undefined1 local_18;
  undefined4 local_17;
  undefined4 local_13;
  undefined4 local_f;
  undefined2 local_b;
  undefined1 local_9;
  
  if (*(int *)(param_1 + 300) != 0) {
    return;
  }
  *(undefined4 *)(param_1 + 300) = 1;
  if (DAT_0057fc78 == '\x11') {
    iVar2 = FUN_004034e0(0x3367,0x1984);
    if (iVar2 == 0) goto LAB_00409b39;
    puVar3 = (undefined4 *)&local_28;
    local_28 = 0;
    local_27 = 0;
    local_23 = 0;
    local_1f = 0;
    local_1b = 0;
    local_19 = 0;
  }
  else {
    iVar2 = FUN_004034e0(0x3367,0x1970);
    if ((iVar2 == 0) ||
       ((cVar1 = FUN_00405480(), cVar1 != '\x01' && (cVar1 = FUN_004055a0(), cVar1 != '\x01'))))
    goto LAB_00409b39;
    local_17 = 0;
    local_13 = 0;
    local_f = 0;
    local_b = 0;
    local_9 = 0;
    puVar3 = (undefined4 *)&local_18;
    local_18 = 0;
  }
  FUN_00403bd0(puVar3);
LAB_00409b39:
  *(undefined4 *)(param_1 + 300) = 0;
  return;
}



/* ======================================================
 * FUN_00417260 @ 00417260
 * ====================================================== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_00417260(char param_1,int param_2)

{
  char cVar1;
  ushort uVar2;
  uint uVar3;
  undefined **ppuVar4;
  wchar_t *pwVar5;
  int iVar6;
  wchar_t *pwVar7;
  undefined1 local_60;
  uint local_5f;
  undefined4 local_5b;
  undefined4 local_57;
  undefined1 local_53;
  undefined1 local_50;
  undefined4 local_4f;
  uint local_4b;
  undefined4 local_47;
  undefined1 local_43;
  wchar_t *local_40;
  float local_3c;
  byte local_36;
  byte local_35;
  wchar_t *local_34;
  char local_2d;
  char local_2c [12];
  char local_20 [12];
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  uint local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_005307c0;
  local_10 = ExceptionList;
  uVar3 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  local_50 = 0;
  local_4f = 0;
  local_4b = 0;
  local_47 = 0;
  local_43 = 0;
  local_20[0] = '\0';
  local_20[1] = '\0';
  local_20[2] = '\0';
  local_20[3] = '\0';
  local_20[4] = '\0';
  local_20[5] = '\0';
  local_20[6] = '\0';
  local_20[7] = '\0';
  local_20[8] = '\0';
  local_20[9] = 0;
  local_14 = uVar3;
  ppuVar4 = FUN_004240a5();
  if (ppuVar4 == (undefined **)0x0) {
                    /* WARNING: Subroutine does not return */
    FUN_00402160((undefined4 *)0x80004005);
  }
  pwVar5 = (wchar_t *)(**(code **)(*ppuVar4 + 0xc))(uVar3);
  pwVar7 = pwVar5 + 8;
  local_8 = 0;
  local_34 = pwVar7;
  if (param_1 == '\"') {
    DAT_005866aa = 1;
    CWnd::EnableWindow((CWnd *)(param_2 + 0x2ad0),1);
    cVar1 = FUN_00405480();
    if (((cVar1 == '\x01') || (local_2d = FUN_004055a0(), local_2d == '\x01')) &&
       (local_2d = FUN_00405340((undefined4 *)&local_50), local_2d == '\x01')) {
      __itoa_s(local_4f & 0xffff,local_20,10,0x10);
      uVar2 = FUN_0050e251(local_20);
      if (uVar2 < 200) {
        local_40 = (wchar_t *)((float)uVar2 / (float)_DAT_0055f3f0);
        FUN_00402230(&local_34,L"%.2f");
        CWnd::SetWindowTextW((CWnd *)(param_2 + 0x2e4c),local_34);
        pwVar7 = local_34;
      }
      cVar1 = FUN_00405480();
      if (((cVar1 == '\x01') || (local_2d = FUN_004055a0(), local_2d == '\x01')) &&
         (local_2d = FUN_00405200((undefined4 *)&local_50,0x15e), local_2d == '\x01')) {
        if (local_4f._1_2_ != 0x1984) {
          *(undefined1 *)(param_2 + 0x3438) = 0xf0;
          DAT_0057fc78 = 0xff;
          FUN_00419160(param_2);
          FUN_00401fa0((int *)&local_34);
          goto LAB_004175d1;
        }
        *(undefined1 *)(param_2 + 0x3438) = 0xf1;
        __itoa_s(local_4b >> 0x10,local_20,10,0x10);
        uVar2 = FUN_0050e251(local_20);
        if (*(char *)(param_2 + 0x3438) == -0xf) {
          pwVar7 = L"Wireless";
        }
        else {
          pwVar7 = L"N/A";
        }
        CWnd::SetWindowTextW((CWnd *)(param_2 + 0x31f4),pwVar7);
        cVar1 = FUN_00405480();
        if ((cVar1 == '\x01') || (local_2d = FUN_004055a0(), local_2d == '\x01')) {
          local_35 = 0;
          local_2d = FUN_004050c0((void *)0x15e,&local_35);
          if (local_2d == '\x01') {
            if (100 < local_35) {
              local_35 = 100;
            }
            FUN_00402230(&local_34,L"%d");
            FUN_0040c380(&local_34,L"%");
            CWnd::SetWindowTextW((CWnd *)(param_2 + 0x32dc),local_34);
            local_3c = (float)uVar2 / (float)_DAT_0055f3f0;
            FUN_00402230(&local_34,L"%.2f");
            CWnd::SetWindowTextW((CWnd *)(param_2 + 0x3268),local_34);
            cVar1 = FUN_00405480();
            if (((cVar1 != '\x01') && (local_2d = FUN_004055a0(), local_2d != '\x01')) ||
               (local_2d = FUN_00403d10(400), local_2d != '\x01')) {
              DAT_0057fc78 = 0xff;
              FUN_00419160(param_2);
              FUN_00401fa0((int *)&local_34);
              goto LAB_004175d1;
            }
            FUN_00419160(param_2);
            CWnd::EnableWindow((CWnd *)(param_2 + 0x301c),0);
            FUN_004041e0();
            FUN_00417d20();
            goto LAB_004175ac;
          }
        }
        DAT_0057fc78 = 0xff;
        FUN_00419160(param_2);
        pwVar7 = local_34;
      }
      else {
        DAT_0057fc78 = 0xff;
        FUN_00419160(param_2);
        FUN_00419cc0();
      }
    }
    else {
LAB_00417413:
      DAT_0057fc78 = 0xff;
      FUN_00419160(param_2);
    }
    pwVar5 = pwVar7 + -8;
    pwVar7 = pwVar7 + -2;
    LOCK();
    iVar6 = *(int *)pwVar7;
    *(int *)pwVar7 = *(int *)pwVar7 + -1;
    UNLOCK();
LAB_00417439:
    local_8 = 0xffffffff;
    if (iVar6 + -1 < 1) {
      (**(code **)(**(int **)pwVar5 + 4))();
    }
  }
  else {
    if (param_1 == '\x11') {
      local_2d = FUN_00405200((undefined4 *)&local_50,0x96);
      if (local_2d != '\x01') {
        DAT_0057fc78 = 0xff;
        FUN_00419160(param_2);
        pwVar7 = pwVar5 + 6;
        LOCK();
        iVar6 = *(int *)pwVar7;
        *(int *)pwVar7 = *(int *)pwVar7 + -1;
        UNLOCK();
        goto LAB_00417439;
      }
      *(undefined1 *)(param_2 + 0x3438) = 0xf1;
      __itoa_s(local_4b >> 0x10,local_20,10,0x10);
      uVar2 = FUN_0050e251(local_20);
      local_3c = (float)uVar2 / (float)_DAT_0055f3f0;
      FUN_00402230(&local_34,L"%.2f");
      pwVar7 = local_34;
      CWnd::SetWindowTextW((CWnd *)(param_2 + 0x3268),local_34);
      local_36 = 0;
      local_2d = FUN_004050c0((void *)0x12c,&local_36);
      if (local_2d != '\x01') goto LAB_00417413;
      if (local_36 < 100) {
        pwVar7 = L"Charging";
      }
      else {
        local_36 = 100;
        FUN_00402230(&local_34,L"%d");
        FUN_0040c380(&local_34,L"%");
        pwVar7 = local_34;
      }
      CWnd::SetWindowTextW((CWnd *)(param_2 + 0x32dc),pwVar7);
      local_2d = FUN_00403d10(0x6e);
      if (local_2d != '\x01') {
        DAT_0057fc78 = 0xff;
        FUN_00419160(param_2);
        FUN_00401fa0((int *)&local_34);
        goto LAB_004175d1;
      }
      FUN_00419160(param_2);
      CWnd::EnableWindow((CWnd *)(param_2 + 0x301c),0);
      FUN_004041e0();
      FUN_00417d20();
      iVar6 = FUN_004034e0((uint)DAT_0058730c,(uint)DAT_0058730e);
      if (iVar6 != 0) {
        DAT_005866aa = 1;
        FUN_00409b50();
        local_60 = 0;
        local_5f = 0;
        local_5b = 0;
        local_57 = 0;
        local_53 = 0;
        local_2c[0] = '\0';
        local_2c[1] = '\0';
        local_2c[2] = '\0';
        local_2c[3] = '\0';
        local_2c[4] = '\0';
        local_2c[5] = '\0';
        local_2c[6] = '\0';
        local_2c[7] = '\0';
        local_2c[8] = '\0';
        local_2c[9] = 0;
        ATL::CStringT<wchar_t,StrTraitMFC<wchar_t,ATL::ChTraitsOS<wchar_t>_>_>::
        CStringT<wchar_t,StrTraitMFC<wchar_t,ATL::ChTraitsOS<wchar_t>_>_>
                  ((CStringT<wchar_t,StrTraitMFC<wchar_t,ATL::ChTraitsOS<wchar_t>_>_> *)&local_40);
        local_8 = CONCAT31(local_8._1_3_,1);
        cVar1 = FUN_00405480();
        if (((cVar1 == '\x01') || (cVar1 = FUN_004055a0(), cVar1 == '\x01')) &&
           (cVar1 = FUN_00405340((undefined4 *)&local_60), cVar1 == '\x01')) {
          __itoa_s(local_5f & 0xffff,local_2c,10,0x10);
          uVar2 = FUN_0050e251(local_2c);
          local_3c = (float)uVar2 / (float)_DAT_0055f3f0;
          FUN_00402230(&local_40,L"%.2f");
          CWnd::SetWindowTextW((CWnd *)(param_2 + 0x2e4c),local_40);
        }
        local_8 = local_8 & 0xffffff00;
        FUN_00401fa0((int *)&local_40);
      }
      FUN_004198f0();
    }
LAB_004175ac:
    local_8 = 0xffffffff;
    pwVar7 = local_34 + -2;
    LOCK();
    iVar6 = *(int *)pwVar7;
    *(int *)pwVar7 = *(int *)pwVar7 + -1;
    UNLOCK();
    if (iVar6 == 1 || iVar6 + -1 < 0) {
      (**(code **)(**(int **)(local_34 + -8) + 4))();
    }
  }
LAB_004175d1:
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}



/* ======================================================
 * FUN_004198f0 @ 004198f0
 * ====================================================== */

undefined1 FUN_004198f0(void)

{
  int iVar1;
  
  DAT_0057fc78 = 0xff;
  iVar1 = FUN_004034e0((uint)DAT_00587304,(uint)DAT_00587306);
  if (iVar1 != 0) {
    DAT_0057fc78 = 0x11;
    return 0x11;
  }
  iVar1 = FUN_004034e0((uint)DAT_0058730c,(uint)DAT_0058730e);
  if (iVar1 != 0) {
    DAT_005866aa = 1;
    DAT_0057fc78 = 0x22;
  }
  return DAT_0057fc78;
}



/* ======================================================
 * FUN_00417dd0 @ 00417dd0
 * ====================================================== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00417dd0(void *this,int param_1,int param_2)

{
  char cVar1;
  ushort uVar2;
  wchar_t *pwVar3;
  int iVar4;
  CWnd *pCVar5;
  undefined1 auStack_b4 [8];
  wchar_t *local_ac;
  float local_a8;
  wchar_t *local_a4;
  undefined1 local_a0;
  undefined4 local_9f;
  undefined4 local_9b;
  undefined4 local_97;
  undefined1 local_93;
  char local_90 [12];
  uint local_84;
  void *local_4c;
  undefined1 *puStack_48;
  undefined4 local_44;
  
  local_44 = 0xffffffff;
  puStack_48 = &LAB_00530800;
  local_4c = ExceptionList;
  local_84 = DAT_0057e340 ^ (uint)auStack_b4;
  ExceptionList = &local_4c;
  FUN_0040c170(&local_ac,(short *)(param_2 + 0x1c));
  local_44 = 0;
  if (param_1 == 0x8000) {
    if (DAT_00587314 == (wchar_t *)0x0) goto LAB_004183e4;
    if (-1 < *(int *)(local_ac + -6)) {
      pwVar3 = _wcsstr(local_ac,DAT_00587314);
      if ((((pwVar3 != (wchar_t *)0x0) && (0 < (int)pwVar3 - (int)local_ac >> 1)) &&
          (DAT_00587318 != (wchar_t *)0x0)) && (-1 < *(int *)(local_ac + -6))) {
        pwVar3 = _wcsstr(local_ac,DAT_00587318);
        if (((pwVar3 != (wchar_t *)0x0) && (0 < (int)pwVar3 - (int)local_ac >> 1)) &&
           (DAT_0057fc78 != '\x11')) {
          if (DAT_0057fc78 == '\"') {
            DAT_0057fc78 = '\x11';
            _DAT_00586218 = 0;
            iVar4 = FUN_004034e0((uint)DAT_00587304,(uint)DAT_00587306);
            if (iVar4 != 0) {
              Sleep(0x44c);
              FUN_00416b70();
              CWnd::SetWindowTextW((CWnd *)((int)this + 0x31f4),L"Wired");
              FUN_00417260(DAT_0057fc78,(int)this);
              goto LAB_004183e4;
            }
          }
          else {
            if (DAT_0057fc78 != -1) goto LAB_004183e4;
            DAT_0057fc78 = '\x11';
            iVar4 = FUN_004034e0((uint)DAT_00587304,(uint)DAT_00587306);
            if (iVar4 != 0) {
              Sleep(0x44c);
              FUN_00416b70();
              CWnd::SetWindowTextW((CWnd *)((int)this + 0x31f4),L"Wired");
              FUN_00417260('\x11',(int)this);
              goto LAB_004183e4;
            }
          }
          CWnd::SetWindowTextW((CWnd *)((int)this + 0x31f4),L"N/A");
          goto LAB_004183e4;
        }
      }
    }
    if ((DAT_00587314 == (wchar_t *)0x0) || (*(int *)(local_ac + -6) < 0)) goto LAB_004183e4;
    pwVar3 = _wcsstr(local_ac,DAT_00587314);
    if ((pwVar3 == (wchar_t *)0x0) ||
       ((((int)pwVar3 - (int)local_ac >> 1 < 1 || (DAT_0058731c == (wchar_t *)0x0)) ||
        (*(int *)(local_ac + -6) < 0)))) goto LAB_004183e4;
    pwVar3 = _wcsstr(local_ac,DAT_0058731c);
    if (((pwVar3 == (wchar_t *)0x0) || ((int)pwVar3 - (int)local_ac >> 1 < 1)) ||
       (DAT_005866aa != '\0')) goto LAB_004183e4;
    DAT_005866aa = '\x01';
    FUN_00409b50();
    if (DAT_0057fc78 == -1) {
      iVar4 = FUN_004034e0((uint)DAT_0058730c,(uint)DAT_0058730e);
      if (iVar4 == 0) goto LAB_004183e4;
      Sleep(0x44c);
LAB_004182d3:
      DAT_0057fc78 = '\"';
      FUN_00416b70();
      FUN_00417260('\"',(int)this);
      goto LAB_004183e4;
    }
    if (DAT_0057fc78 != '\x11') goto LAB_004183e4;
    FUN_004034e0((uint)DAT_0058730c,(uint)DAT_0058730e);
    local_a0 = 0;
    local_9f = 0;
    local_9b = 0;
    local_97 = 0;
    local_93 = 0;
    local_90[0] = '\0';
    local_90[1] = '\0';
    local_90[2] = '\0';
    local_90[3] = '\0';
    local_90[4] = '\0';
    local_90[5] = '\0';
    local_90[6] = '\0';
    local_90[7] = '\0';
    local_90[8] = '\0';
    local_90[9] = 0;
    ATL::CStringT<wchar_t,StrTraitMFC<wchar_t,ATL::ChTraitsOS<wchar_t>_>_>::
    CStringT<wchar_t,StrTraitMFC<wchar_t,ATL::ChTraitsOS<wchar_t>_>_>
              ((CStringT<wchar_t,StrTraitMFC<wchar_t,ATL::ChTraitsOS<wchar_t>_>_> *)&local_a4);
    local_44 = CONCAT31(local_44._1_3_,1);
    cVar1 = FUN_00405480();
    if (cVar1 == '\x01') {
LAB_0041835d:
      cVar1 = FUN_00405340((undefined4 *)&local_a0);
      if (cVar1 == '\x01') {
        __itoa_s((uint)CONCAT11((undefined1)local_9b,local_9f._3_1_),local_90,10,0x10);
        uVar2 = FUN_0050e251(local_90);
        local_a8 = (float)uVar2 / (float)_DAT_0055f3f0;
        FUN_00402230(&local_a4,L"%.2f");
        CWnd::SetWindowTextW((CWnd *)((int)this + 0x2e4c),local_a4);
      }
    }
    else {
      cVar1 = FUN_004055a0();
      if (cVar1 == '\x01') goto LAB_0041835d;
    }
    FUN_00401fa0((int *)&local_a4);
    goto LAB_004183e4;
  }
  if ((param_1 != 0x8004) || (DAT_00587314 == (wchar_t *)0x0)) goto LAB_004183e4;
  if (*(int *)(local_ac + -6) < 0) {
LAB_00417f59:
    if ((DAT_00587314 == (wchar_t *)0x0) || (*(int *)(local_ac + -6) < 0)) goto LAB_004183e4;
    pwVar3 = _wcsstr(local_ac,DAT_00587314);
    if ((pwVar3 == (wchar_t *)0x0) ||
       ((((int)pwVar3 - (int)local_ac >> 1 < 1 || (DAT_0058731c == (wchar_t *)0x0)) ||
        (*(int *)(local_ac + -6) < 0)))) goto LAB_004183e4;
    pwVar3 = _wcsstr(local_ac,DAT_0058731c);
    if (((pwVar3 == (wchar_t *)0x0) || ((int)pwVar3 - (int)local_ac >> 1 < 1)) ||
       (DAT_005866aa != '\x01')) goto LAB_004183e4;
    DAT_005866aa = '\0';
    FUN_00409b50();
    if (DAT_0057fc78 != '\x11') {
      DAT_005862a3 = 0;
      *(undefined1 *)((int)this + 0x3438) = 0;
      _DAT_00586214 = 0;
      _DAT_00586218 = 0;
      FUN_00419160((int)this);
      _DAT_00586214 = 0;
      _DAT_00586218 = 0;
      DAT_0057fc78 = -1;
      CWnd::SetWindowTextW((CWnd *)((int)this + 0x2e4c),L"");
      CWnd::SetWindowTextW((CWnd *)((int)this + 0x3268),L"");
      CWnd::SetWindowTextW((CWnd *)((int)this + 0x31f4),L"N/A");
      pCVar5 = (CWnd *)((int)this + 0x33c4);
      goto LAB_0041807b;
    }
    *(undefined1 *)((int)this + 0x3438) = 0xf1;
    pCVar5 = (CWnd *)((int)this + 0x2e4c);
  }
  else {
    pwVar3 = _wcsstr(local_ac,DAT_00587314);
    if ((((pwVar3 == (wchar_t *)0x0) || ((int)pwVar3 - (int)local_ac >> 1 < 1)) ||
        (DAT_00587318 == (wchar_t *)0x0)) || (*(int *)(local_ac + -6) < 0)) goto LAB_00417f59;
    pwVar3 = _wcsstr(local_ac,DAT_00587318);
    if (((pwVar3 == (wchar_t *)0x0) || ((int)pwVar3 - (int)local_ac >> 1 < 1)) ||
       (DAT_0057fc78 != '\x11')) goto LAB_00417f59;
    _DAT_00586218 = 0;
    _DAT_00586214 = 0;
    CWnd::SetWindowTextW((CWnd *)((int)this + 0x3268),L"");
    CWnd::SetWindowTextW((CWnd *)((int)this + 0x32dc),L"");
    iVar4 = FUN_004034e0((uint)DAT_0058730c,(uint)DAT_0058730e);
    if (iVar4 != 0) {
      DAT_005866aa = '\x01';
      FUN_00409b50();
      goto LAB_004182d3;
    }
    _DAT_00586214 = 0;
    _DAT_00586218 = 0;
    FUN_00419160((int)this);
    CWnd::SetWindowTextW((CWnd *)((int)this + 0x31f4),L"N/A");
    DAT_0057fc78 = -1;
    pCVar5 = (CWnd *)((int)this + 0x2e4c);
LAB_0041807b:
    CWnd::SetWindowTextW(pCVar5,L"");
    pCVar5 = (CWnd *)((int)this + 0x32dc);
  }
  CWnd::SetWindowTextW(pCVar5,L"");
  CWnd::SetWindowTextW((CWnd *)((int)this + 0x3350),L"");
LAB_004183e4:
  local_44 = 0xffffffff;
  pwVar3 = local_ac + -2;
  LOCK();
  iVar4 = *(int *)pwVar3;
  *(int *)pwVar3 = *(int *)pwVar3 + -1;
  UNLOCK();
  if (iVar4 == 1 || iVar4 + -1 < 0) {
    (**(code **)(**(int **)(local_ac + -8) + 4))();
  }
  ExceptionList = local_4c;
  __security_check_cookie(local_84 ^ (uint)auStack_b4);
  return;
}



/* ======================================================
 * FUN_00419f10 @ 00419f10
 * ====================================================== */

void __fastcall FUN_00419f10(int param_1)

{
  char cVar1;
  int iVar2;
  undefined1 local_18;
  undefined4 local_17;
  undefined4 local_13;
  undefined4 local_f;
  undefined1 local_b;
  
  if (DAT_0057fc78 == -1) {
    if (*(char *)(param_1 + 0x3438) == '\0') {
      cVar1 = FUN_00405480();
      if (((cVar1 == '\b') || (cVar1 == '\x03')) || (cVar1 == '\x11')) {
        cVar1 = FUN_004055a0();
      }
      if (cVar1 == '\x01') {
        local_18 = 0;
        local_17 = 0;
        local_13 = 0;
        local_f = 0;
        local_b = 0;
        cVar1 = FUN_00405200((undefined4 *)&local_18,300);
        if (cVar1 == '\x01') {
          if (local_17._1_2_ == 0x1984) {
            *(undefined1 *)(param_1 + 0x3438) = 0xf1;
          }
          else {
            *(undefined1 *)(param_1 + 0x3438) = 0xf0;
            DAT_0057fc78 = -1;
          }
        }
      }
    }
    if (*(char *)(param_1 + 0x3438) == -0xf) {
      iVar2 = FUN_004034e0((uint)DAT_0058730c,(uint)DAT_0058730e);
      if (iVar2 != 0) {
        FUN_00416b70();
        DAT_005866aa = 1;
        CWnd::EnableWindow((CWnd *)(param_1 + 0x2ad0),1);
        DAT_0057fc78 = '\"';
        CWnd::SetWindowTextW((CWnd *)(param_1 + 0x31f4),L"Wireless");
        FUN_00417260('\"',param_1);
        FUN_00419d30();
        CWnd::EnableWindow((CWnd *)(param_1 + 0x301c),0);
      }
    }
  }
  return;
}



/* ======================================================
 * FUN_004049f0 @ 004049f0
 * ====================================================== */

void __fastcall FUN_004049f0(int param_1)

{
  undefined1 *in_EAX;
  
  *in_EAX = *(undefined1 *)(param_1 + 0x2e);
  in_EAX[1] = *(undefined1 *)(param_1 + 0x2f);
  in_EAX[2] = *(undefined1 *)(param_1 + 0x30);
  in_EAX[3] = *(undefined1 *)(param_1 + 0x31);
  in_EAX[4] = *(undefined1 *)(param_1 + 0x32);
  in_EAX[5] = *(undefined1 *)(param_1 + 0x33);
  in_EAX[6] = *(undefined1 *)(param_1 + 0x34);
  in_EAX[7] = *(undefined1 *)(param_1 + 0x36);
  in_EAX[8] = *(undefined1 *)(param_1 + 0x37);
  in_EAX[9] = *(undefined1 *)(param_1 + 0x38);
  in_EAX[10] = *(undefined1 *)(param_1 + 0x39);
  in_EAX[0xb] = *(undefined1 *)(param_1 + 0x3a);
  in_EAX[0xc] = *(undefined1 *)(param_1 + 0x3b);
  in_EAX[0xd] = *(undefined1 *)(param_1 + 0x3c);
  in_EAX[0xe] = *(undefined1 *)(param_1 + 0x3e);
  in_EAX[0xf] = *(undefined1 *)(param_1 + 0x3f);
  in_EAX[0x10] = *(undefined1 *)(param_1 + 0x40);
  in_EAX[0x11] = *(undefined1 *)(param_1 + 0x41);
  in_EAX[0x12] = *(undefined1 *)(param_1 + 0x42);
  in_EAX[0x13] = *(undefined1 *)(param_1 + 0x43);
  in_EAX[0x14] = *(undefined1 *)(param_1 + 0x44);
  in_EAX[0x15] = *(undefined1 *)(param_1 + 0x46);
  in_EAX[0x16] = *(undefined1 *)(param_1 + 0x47);
  in_EAX[0x17] = *(undefined1 *)(param_1 + 0x48);
  in_EAX[0x18] = *(undefined1 *)(param_1 + 0x49);
  in_EAX[0x19] = *(undefined1 *)(param_1 + 0x4a);
  in_EAX[0x1a] = *(undefined1 *)(param_1 + 0x4b);
  in_EAX[0x1b] = *(undefined1 *)(param_1 + 0x4c);
  in_EAX[0x1c] = *(undefined1 *)(param_1 + 0x4e);
  in_EAX[0x1d] = *(undefined1 *)(param_1 + 0x4f);
  in_EAX[0x1e] = *(undefined1 *)(param_1 + 0x50);
  in_EAX[0x1f] = *(undefined1 *)(param_1 + 0x51);
  in_EAX[0x20] = *(undefined1 *)(param_1 + 0x52);
  in_EAX[0x21] = *(undefined1 *)(param_1 + 0x53);
  in_EAX[0x22] = *(undefined1 *)(param_1 + 0x54);
  in_EAX[0x23] = *(undefined1 *)(param_1 + 0x56);
  in_EAX[0x24] = *(undefined1 *)(param_1 + 0x57);
  in_EAX[0x25] = *(undefined1 *)(param_1 + 0x58);
  in_EAX[0x26] = *(undefined1 *)(param_1 + 0x59);
  in_EAX[0x27] = *(undefined1 *)(param_1 + 0x5a);
  in_EAX[0x28] = *(undefined1 *)(param_1 + 0x5b);
  in_EAX[0x29] = *(undefined1 *)(param_1 + 0x5c);
  in_EAX[0x2a] = *(undefined1 *)(param_1 + 0x5e);
  in_EAX[0x2b] = *(undefined1 *)(param_1 + 0x5f);
  in_EAX[0x2c] = *(undefined1 *)(param_1 + 0x60);
  in_EAX[0x2d] = *(undefined1 *)(param_1 + 0x61);
  in_EAX[0x2e] = *(undefined1 *)(param_1 + 0x62);
  in_EAX[0x2f] = *(undefined1 *)(param_1 + 99);
  in_EAX[0x30] = *(undefined1 *)(param_1 + 100);
  in_EAX[0x31] = *(undefined1 *)(param_1 + 0x66);
  in_EAX[0x32] = *(undefined1 *)(param_1 + 0x67);
  in_EAX[0x33] = *(undefined1 *)(param_1 + 0x68);
  in_EAX[0x34] = *(undefined1 *)(param_1 + 0x69);
  in_EAX[0x35] = *(undefined1 *)(param_1 + 0x6a);
  in_EAX[0x36] = *(undefined1 *)(param_1 + 0x6b);
  in_EAX[0x37] = *(undefined1 *)(param_1 + 0x6c);
  return;
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
 * FUN_00418dd0 @ 00418dd0
 * ====================================================== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_00418dd0(int param_1)

{
  uint uVar1;
  int3 iVar2;
  char cVar3;
  byte bVar4;
  undefined1 uVar5;
  undefined4 uStack_c;
  
  if (*(char *)(param_1 + 0x3090) != '\0') {
    return;
  }
  if (*(char *)(param_1 + 0x3090) != '\0') {
    return;
  }
  *(undefined1 *)(param_1 + 0x3090) = 1;
  _DAT_00586214 = 0;
  FUN_00412eb0();
  FUN_00410460();
  FUN_00409f30();
  if (DAT_0057fc78 == -1) {
    FUN_004198f0();
  }
  if (DAT_0057fc78 == '\x11') {
    if ((DAT_005862a2 == '\x01') && (cVar3 = FUN_004198f0(), cVar3 == '\x11')) {
      DAT_005862a2 = '\0';
    }
    CWnd::EnableWindow((CWnd *)(param_1 + 0x301c),0);
    bVar4 = FUN_00419a20();
    iVar2 = (uint3)(bVar4 & 0xff01) << 0x10;
    if ((bVar4 & 1) != 0) {
      uVar5 = FUN_00404720(&DAT_005866ba,500);
      iVar2 = (CONCAT11(bVar4,uVar5) & 0xff01ff) << 8;
    }
    uStack_c = CONCAT13(bVar4,iVar2) & 0x2ffffff;
    if ((bVar4 & 2) == 0) {
LAB_00418eea:
      if ((bVar4 & 4) != 0) {
        if (((uStack_c._2_1_ != '\0') || (uStack_c._3_1_ != '\0')) && (uStack_c._1_1_ != '\x01'))
        goto LAB_00419040;
        bVar4 = FUN_00404860(&DAT_005866b0,500);
        uStack_c = (uint)bVar4 << 8;
      }
      cVar3 = uStack_c._1_1_;
      goto LAB_00419026;
    }
    if ((uStack_c._2_1_ == '\0') || (uStack_c._1_1_ == '\x01')) {
      cVar3 = FUN_00404b80(&DAT_005866b0,500);
      uStack_c = (uint)CONCAT21(uStack_c._2_2_,cVar3) << 8;
      if ((cVar3 == '\x01') && ((DAT_005866b8 == '\0' && ((DAT_005866b7 & 0x20) != 0)))) {
        CWnd::ShowWindow((CWnd *)(param_1 + 0x21e4),0);
      }
      goto LAB_00418eea;
    }
  }
  else {
    if ((DAT_0057fc78 != '\"') ||
       ((cVar3 = FUN_00405480(), cVar3 != '\x01' && (cVar3 = FUN_004055a0(), cVar3 != '\x01'))))
    goto LAB_0041904d;
    cVar3 = '\x01';
    CWnd::EnableWindow((CWnd *)(param_1 + 0x301c),0);
    bVar4 = FUN_00419a20();
    uVar1 = (uint)CONCAT11(bVar4,bVar4) << 8;
    uStack_c = uVar1 & 0xffff01ff;
    if ((bVar4 & 1) != 0) {
      cVar3 = FUN_00404720(&DAT_005866ba,0x370);
    }
    uVar5 = uStack_c._2_1_;
    uStack_c = CONCAT13(uVar5,(undefined3)uStack_c) & 0x2ffffff;
    if ((uVar1 & 0x20000) != 0) {
      if (uStack_c._1_1_ != '\0') {
        if (cVar3 != '\x01') goto LAB_00419040;
        cVar3 = FUN_00405480();
        if (cVar3 != '\x01') {
          FUN_004055a0();
        }
      }
      cVar3 = FUN_00404b80(&DAT_005866b0,0x370);
      if (((cVar3 == '\x01') && (DAT_005866b8 == '\0')) && ((DAT_005866b7 & 0x20) != 0)) {
        CWnd::ShowWindow((CWnd *)(param_1 + 0x21e4),0);
      }
    }
    if ((uVar1 & 0x40000) != 0) {
      if ((uStack_c._1_1_ != '\0') || (uStack_c._3_1_ != '\0')) {
        if (cVar3 != '\x01') goto LAB_00419040;
        cVar3 = FUN_00405480();
        if (cVar3 != '\x01') {
          FUN_004055a0();
        }
      }
      cVar3 = FUN_00404860(&DAT_005866b0,0x370);
    }
LAB_00419026:
    if (cVar3 == '\x01') {
      FUN_00403e50();
      *(undefined1 *)(param_1 + 0x3090) = 0;
      return;
    }
  }
LAB_00419040:
  CWnd::EnableWindow((CWnd *)(param_1 + 0x301c),1);
LAB_0041904d:
  *(undefined1 *)(param_1 + 0x3090) = 0;
  return;
}



/* ======================================================
 * FUN_00404ce0 @ 00404ce0
 * ====================================================== */

void __fastcall FUN_00404ce0(undefined1 *param_1)

{
  undefined1 *in_EAX;
  
  *in_EAX = param_1[2];
  in_EAX[1] = param_1[10];
  in_EAX[2] = param_1[0xb];
  in_EAX[3] = param_1[3];
  in_EAX[4] = param_1[4];
  in_EAX[5] = param_1[7];
  in_EAX[6] = param_1[1];
  in_EAX[7] = *param_1;
  in_EAX[8] = param_1[0xc];
  in_EAX[9] = param_1[0xe];
  in_EAX[10] = param_1[0xf];
  in_EAX[0xb] = param_1[0x10];
  in_EAX[0xc] = param_1[0x11];
  in_EAX[0xd] = param_1[0x12];
  in_EAX[0xe] = param_1[0x14];
  in_EAX[0xf] = param_1[0x15];
  in_EAX[0x10] = param_1[0x16];
  in_EAX[0x11] = param_1[0x17];
  in_EAX[0x12] = param_1[0x18];
  in_EAX[0x13] = param_1[0x1a];
  in_EAX[0x14] = param_1[0x1b];
  in_EAX[0x15] = param_1[0x1c];
  in_EAX[0x16] = param_1[0x1d];
  in_EAX[0x17] = param_1[0x1e];
  in_EAX[0x18] = param_1[0x20];
  in_EAX[0x19] = param_1[0x21];
  in_EAX[0x1a] = param_1[0x22];
  in_EAX[0x1b] = param_1[0x23];
  return;
}



/* ======================================================
 * FUN_00404720 @ 00404720
 * ====================================================== */

void __thiscall FUN_00404720(void *this,ushort param_1)

{
  char cVar1;
  undefined3 extraout_var;
  int iVar2;
  uint *puVar3;
  uint *puVar4;
  undefined1 local_b8;
  undefined1 local_b7 [63];
  undefined4 local_78;
  undefined2 local_74;
  undefined1 local_72;
  uint local_68 [12];
  uint local_38 [10];
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052f10b;
  local_10 = ExceptionList;
  local_38[8] = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  local_b8 = 0;
  _memset(local_b7,0,0x3f);
  local_38[0] = 0;
  local_38[1] = 0;
  local_38[2] = 0;
  local_38[3] = 0;
  local_38[4] = 0;
  local_38[5] = 0;
  local_38[6] = 0;
  local_38[7] = 0;
  FUN_00404ce0(this);
  _memset(&local_78,0,0x40);
  local_78 = 0x1c0f14a1;
  local_74 = 0;
  local_72 = 0;
  puVar3 = local_38;
  puVar4 = local_68;
  for (iVar2 = 8; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar3;
    puVar3 = puVar3 + 1;
    puVar4 = puVar4 + 1;
  }
  cVar1 = FUN_00403740();
  Sleep((uint)param_1);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  else {
    _memset(&local_b8,0,0x40);
    local_b8 = 0xa1;
    FUN_00403810();
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  ExceptionList = local_10;
  __security_check_cookie(local_38[8] ^ (uint)&stack0xfffffffc);
  return;
}



/* ======================================================
 * FUN_004041e0 @ 004041e0
 * ====================================================== */

void FUN_004041e0(void)

{
  byte bVar1;
  undefined1 *in_EAX;
  
  *in_EAX = DAT_005862a8;
  in_EAX[0x11] = DAT_005862a9;
  in_EAX[6] = DAT_005862ab;
  in_EAX[5] = DAT_005862ac;
  in_EAX[4] = DAT_005862ad;
  in_EAX[7] = DAT_005862ae;
  in_EAX[0xc] = DAT_005862af;
  in_EAX[0x14] = DAT_005862b0;
  in_EAX[0x15] = DAT_005862b1;
  in_EAX[0xd] = DAT_005862b2;
  in_EAX[0xe] = DAT_005862b3;
  in_EAX[0xf] = DAT_005862b4;
  in_EAX[10] = DAT_005862b5;
  in_EAX[0xb] = DAT_005862b6;
  in_EAX[0x6e] = DAT_005862b7;
  in_EAX[0x6f] = DAT_005862b8;
  in_EAX[0x70] = DAT_005862b9;
  in_EAX[0x71] = DAT_005862ba;
  in_EAX[0x72] = DAT_005862bb;
  in_EAX[0x73] = DAT_005862bc;
  in_EAX[0x74] = DAT_005862bd;
  in_EAX[0x75] = DAT_005862be;
  in_EAX[0x76] = DAT_005862bf;
  in_EAX[0x77] = DAT_005862c0;
  in_EAX[0x78] = DAT_005862c1;
  in_EAX[0x79] = DAT_005862c2;
  in_EAX[0x7a] = DAT_005862c3;
  in_EAX[0x7b] = DAT_005862c4;
  in_EAX[0x7c] = DAT_005862c5;
  in_EAX[0x7d] = DAT_005862c6;
  in_EAX[0x7e] = DAT_005862c7;
  in_EAX[0x7f] = DAT_005862c8;
  in_EAX[0x80] = DAT_005862c9;
  in_EAX[0x81] = DAT_005862ca;
  in_EAX[0x16] = DAT_005862cb;
  bVar1 = DAT_005862cd;
  *(ushort *)(in_EAX + 0x18) = (ushort)DAT_005862cd << 8;
  *(ushort *)(in_EAX + 0x18) = CONCAT11(bVar1,DAT_005862cc);
  bVar1 = DAT_005862cf;
  *(ushort *)(in_EAX + 0x1a) = (ushort)DAT_005862cf << 8;
  *(ushort *)(in_EAX + 0x1a) = CONCAT11(bVar1,DAT_005862ce);
  in_EAX[0x1c] = DAT_005862d0;
  bVar1 = DAT_005862d2;
  *(ushort *)(in_EAX + 0x1e) = (ushort)DAT_005862d2 << 8;
  *(ushort *)(in_EAX + 0x1e) = CONCAT11(bVar1,DAT_005862d1);
  bVar1 = DAT_005862d4;
  *(ushort *)(in_EAX + 0x20) = (ushort)DAT_005862d4 << 8;
  *(ushort *)(in_EAX + 0x20) = CONCAT11(bVar1,DAT_005862d3);
  in_EAX[0x22] = DAT_005862d5;
  bVar1 = DAT_005862d7;
  *(ushort *)(in_EAX + 0x24) = (ushort)DAT_005862d7 << 8;
  *(ushort *)(in_EAX + 0x24) = CONCAT11(bVar1,DAT_005862d6);
  bVar1 = DAT_005862d9;
  *(ushort *)(in_EAX + 0x26) = (ushort)DAT_005862d9 << 8;
  *(ushort *)(in_EAX + 0x26) = CONCAT11(bVar1,DAT_005862d8);
  in_EAX[0x28] = DAT_005862da;
  bVar1 = DAT_005862dc;
  *(ushort *)(in_EAX + 0x2a) = (ushort)DAT_005862dc << 8;
  *(ushort *)(in_EAX + 0x2a) = CONCAT11(bVar1,DAT_005862db);
  bVar1 = DAT_005862de;
  *(ushort *)(in_EAX + 0x2c) = (ushort)DAT_005862de << 8;
  *(ushort *)(in_EAX + 0x2c) = CONCAT11(bVar1,DAT_005862dd);
  in_EAX[0x2e] = DAT_005862df;
  in_EAX[0x2f] = DAT_005862e0;
  bVar1 = DAT_005862e2;
  *(ushort *)(in_EAX + 0x30) = (ushort)DAT_005862e2 << 8;
  *(ushort *)(in_EAX + 0x30) = CONCAT11(bVar1,DAT_005862e1);
  bVar1 = DAT_005862e4;
  *(ushort *)(in_EAX + 0x32) = (ushort)DAT_005862e4 << 8;
  *(ushort *)(in_EAX + 0x32) = CONCAT11(bVar1,DAT_005862e3);
  in_EAX[0x34] = DAT_005862e5;
  in_EAX[0x36] = DAT_005862e6;
  in_EAX[0x37] = DAT_005862e7;
  bVar1 = DAT_005862e9;
  *(ushort *)(in_EAX + 0x38) = (ushort)DAT_005862e9 << 8;
  *(ushort *)(in_EAX + 0x38) = CONCAT11(bVar1,DAT_005862e8);
  bVar1 = DAT_005862eb;
  *(ushort *)(in_EAX + 0x3a) = (ushort)DAT_005862eb << 8;
  *(ushort *)(in_EAX + 0x3a) = CONCAT11(bVar1,DAT_005862ea);
  in_EAX[0x3c] = DAT_005862ec;
  in_EAX[0x3e] = DAT_005862ed;
  in_EAX[0x3f] = DAT_005862ee;
  bVar1 = DAT_005862f0;
  *(ushort *)(in_EAX + 0x40) = (ushort)DAT_005862f0 << 8;
  *(ushort *)(in_EAX + 0x40) = CONCAT11(bVar1,DAT_005862ef);
  bVar1 = DAT_005862f2;
  *(ushort *)(in_EAX + 0x42) = (ushort)DAT_005862f2 << 8;
  *(ushort *)(in_EAX + 0x42) = CONCAT11(bVar1,DAT_005862f1);
  in_EAX[0x44] = DAT_005862f3;
  in_EAX[0x46] = DAT_005862f4;
  in_EAX[0x47] = DAT_005862f5;
  bVar1 = DAT_005862f7;
  *(ushort *)(in_EAX + 0x48) = (ushort)DAT_005862f7 << 8;
  *(ushort *)(in_EAX + 0x48) = CONCAT11(bVar1,DAT_005862f6);
  bVar1 = DAT_005862f9;
  *(ushort *)(in_EAX + 0x4a) = (ushort)DAT_005862f9 << 8;
  *(ushort *)(in_EAX + 0x4a) = CONCAT11(bVar1,DAT_005862f8);
  in_EAX[0x4c] = DAT_005862fa;
  in_EAX[0x4e] = DAT_005862fb;
  in_EAX[0x4f] = DAT_005862fc;
  bVar1 = DAT_005862fe;
  *(ushort *)(in_EAX + 0x50) = (ushort)DAT_005862fe << 8;
  *(ushort *)(in_EAX + 0x50) = CONCAT11(bVar1,DAT_005862fd);
  bVar1 = DAT_00586300;
  *(ushort *)(in_EAX + 0x52) = (ushort)DAT_00586300 << 8;
  *(ushort *)(in_EAX + 0x52) = CONCAT11(bVar1,DAT_005862ff);
  in_EAX[0x54] = DAT_00586301;
  in_EAX[0x56] = DAT_00586302;
  in_EAX[0x57] = DAT_00586303;
  bVar1 = DAT_00586305;
  *(ushort *)(in_EAX + 0x58) = (ushort)DAT_00586305 << 8;
  *(ushort *)(in_EAX + 0x58) = CONCAT11(bVar1,DAT_00586304);
  bVar1 = DAT_00586307;
  *(ushort *)(in_EAX + 0x5a) = (ushort)DAT_00586307 << 8;
  *(ushort *)(in_EAX + 0x5a) = CONCAT11(bVar1,DAT_00586306);
  in_EAX[0x5c] = DAT_00586308;
  in_EAX[0x5e] = DAT_00586309;
  in_EAX[0x5f] = DAT_0058630a;
  bVar1 = DAT_0058630c;
  *(ushort *)(in_EAX + 0x60) = (ushort)DAT_0058630c << 8;
  *(ushort *)(in_EAX + 0x60) = CONCAT11(bVar1,DAT_0058630b);
  bVar1 = DAT_0058630e;
  *(ushort *)(in_EAX + 0x62) = (ushort)DAT_0058630e << 8;
  *(ushort *)(in_EAX + 0x62) = CONCAT11(bVar1,DAT_0058630d);
  in_EAX[100] = DAT_0058630f;
  in_EAX[0x66] = DAT_00586310;
  in_EAX[0x67] = DAT_00586311;
  bVar1 = DAT_00586313;
  *(ushort *)(in_EAX + 0x68) = (ushort)DAT_00586313 << 8;
  *(ushort *)(in_EAX + 0x68) = CONCAT11(bVar1,DAT_00586312);
  bVar1 = DAT_00586315;
  *(ushort *)(in_EAX + 0x6a) = (ushort)DAT_00586315 << 8;
  *(ushort *)(in_EAX + 0x6a) = CONCAT11(bVar1,DAT_00586314);
  in_EAX[0x6c] = DAT_00586316;
  in_EAX[0x10] = DAT_00586317;
  return;
}



/* ======================================================
 * FUN_00403d10 @ 00403d10
 * ====================================================== */

void __cdecl FUN_00403d10(ushort param_1)

{
  char cVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  undefined1 local_470;
  char local_46f;
  undefined4 local_460 [258];
  undefined4 local_58;
  undefined4 local_54;
  uint local_18;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052f14b;
  local_10 = ExceptionList;
  local_18 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  _memset(&local_58,0,0x40);
  local_58 = 0x12a1;
  local_54 = 0;
  cVar1 = FUN_00403740();
  Sleep((uint)param_1);
  if (CONCAT31(extraout_var,cVar1) != 0) {
    _memset(&local_470,0,0x411);
    local_470 = 0xa0;
    cVar1 = FUN_00403810();
    if (CONCAT31(extraout_var_00,cVar1) != 0) {
      if (local_46f == '\x01') {
        puVar3 = local_460;
        puVar4 = (undefined4 *)&DAT_005862a8;
        for (iVar2 = 0x100; iVar2 != 0; iVar2 = iVar2 + -1) {
          *puVar4 = *puVar3;
          puVar3 = puVar3 + 1;
          puVar4 = puVar4 + 1;
        }
        FUN_004041e0();
        LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
      }
      else {
        LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
      }
      goto LAB_00403e1c;
    }
  }
  LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
LAB_00403e1c:
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}


