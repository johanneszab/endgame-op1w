/* v1/apply_path.c -- the Apply button, startup read, and refresh paths.
 *
 * Program: Endgame_Gear_OP1w_4k_Configuration_Tool_v1.03.exe
 *   (OP1w 4k **v1** vendor tool; Ghidra project OP1w, image base 0x00400000)
 * Extracted with Ghidra 12.1.4 headless, scripts in git/re/ghidra_scripts.
 * Evidence class for everything in this file: [BIN].
 *
 * Addresses covered:
 *   FUN_00416200   the APPLY handler                 (no callers: message map)
 *   FUN_004147b0   dialog init
 *   FUN_004149a0   startup / full re-read
 *   FUN_00415500   reconnect + re-read
 *   FUN_004168d0   link-up handler (called from the 0xB4 arm of FUN_00415b70)
 *   FUN_00416360   factory reset handler (cmd 0x13, then FUN_00416120 twice)
 *   FUN_00416530   link/model probe -> DAT_0057ccd8 (0xFF / 0x11 / 0x22)
 *   FUN_00416590   dirty check / profile export via FUN_00404dc0
 *
 * ==================== WHAT APPLY ACTUALLY SENDS ====================
 *
 * FUN_00416200 harvests the UI, saves a baseline copy, then sends EXACTLY ONE
 * block, chosen by the currently selected tab:
 *
 *       LVar3 = SendMessageW(tab control, 0x130B /,TCM_GETCURSEL/, 0, 0);
 *       switch (LVar3) {
 *       case 0:        FUN_00404750(&DAT_00583858, uVar4);   // cmd 0x14
 *       case 1: case 3: FUN_00404bb0(&DAT_00583850, uVar4);  // cmd 0x15
 *       case 2:        FUN_00404890(&DAT_00583850, uVar4);   // cmd 0x16
 *       }
 *
 * Two things to take from this:
 *   - the cmd 0x14 struct base is 0x00583858, i.e. settings base + 8. This is
 *     what pins down the offset mapping in v1/serializers.c.
 *   - Apply is per-tab, not global. The vendor tool never sends 0x14, 0x15 and
 *     0x16 from one button press.
 *
 * The per-command Sleep is also picked here: uVar4 = 100 by default, 500 when
 * DAT_0057ccd8 == 0x11, and 0x370 (880 ms) when DAT_0057ccd8 == 0x22 -- i.e.
 * the wireless path waits far longer than the wired one.
 *
 * FUN_004149a0 is the startup sequence: probes (FUN_00405480 / FUN_004055a0),
 * cmd 0x0D (FUN_00405340), cmd 0x0E (FUN_00405200), cmd 0xB4 (FUN_004050c0),
 * cmd 0x12 (FUN_00403d90), and it contains `MOV ECX,0x1972` at 00414b24 -- the
 * v1 PID gate. It also calls FUN_00404230 twice with EAX = 0x584320, i.e. it
 * re-derives the shadow/baseline copy from the blob rather than from the struct.
 */

/* ============ FUN_00416200 @ 00416200  (req 416200) ============ */
/* calls: FUN_00416530@00416530 FUN_004110e0@004110e0 SendMessageW@EXTERNAL:00000183 FUN_00409f90@00409f90 FUN_004055a0@004055a0 FUN_0040f820@0040f820 FUN_00403ec0@00403ec0 FUN_00404890@00404890 FUN_00404750@00404750 FUN_00404bb0@00404bb0 EnableWindow@0041dd65 FUN_00405480@00405480 */
/* called by: */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_00416200(int param_1)

{
  char cVar1;
  char cVar2;
  LRESULT LVar3;
  ushort uVar4;
  
  if ((*(int *)(param_1 + 0x2c6c) == 0) && (*(int *)(param_1 + 0x2c6c) == 0)) {
    *(undefined4 *)(param_1 + 0x2c6c) = 1;
    cVar2 = '\0';
    cVar1 = '\0';
    uVar4 = 100;
    _DAT_005833b4 = 0;
    FUN_004110e0();
    FUN_0040f820();
    FUN_00409f90();
    FUN_00403ec0();
    if (DAT_0057ccd8 == -1) {
      FUN_00416530();
    }
    if (DAT_0057ccd8 == '\x11') {
      uVar4 = 500;
      cVar2 = cVar1;
      if ((DAT_00583442 == '\x01') && (cVar1 = FUN_00416530(), cVar1 == '\x11')) {
        DAT_00583442 = '\0';
      }
    }
    else if (DAT_0057ccd8 == '\"') {
      uVar4 = 0x370;
      cVar1 = FUN_00405480();
      cVar2 = '\x01';
      if (cVar1 != '\x01') {
        cVar2 = FUN_004055a0();
      }
    }
    if (DAT_0057ccd8 != -1) {
      LVar3 = SendMessageW(*(HWND *)(param_1 + 0x2abc),0x130b,0,0);
      switch(LVar3) {
      case 0:
        cVar2 = FUN_00404750(&DAT_00583858,uVar4);
        break;
      case 1:
      case 3:
        cVar2 = FUN_00404bb0(&DAT_00583850,uVar4);
        break;
      case 2:
        cVar2 = FUN_00404890(&DAT_00583850,uVar4);
      }
      if (cVar2 == '\x01') {
        CWnd::EnableWindow((CWnd *)(param_1 + 0x2bf8),0);
      }
    }
    *(undefined4 *)(param_1 + 0x2c6c) = 0;
  }
  return;
}


/* ============ FUN_004147b0 @ 004147b0  (req 004147b0) ============ */
/* calls: FUN_00428aff@00428aff PostQuitMessage@EXTERNAL:00000176 SetWindowTextW@0041dc58 FUN_004149a0@004149a0 FUN_00414940@00414940 RegisterHotKey@EXTERNAL:00000156 EnableWindow@0041dd65 FUN_004150f0@004150f0 FUN_00416120@00416120 FUN_00416470@00416470 SendMessageW@EXTERNAL:00000183 OnInitDialog@0041f95b FUN_00416530@00416530 FUN_00403500@00403500 FindWindowW@EXTERNAL:00000161 AfxMessageBox@004201d6 FUN_004142b0@004142b0 RegisterRawInputDevices@EXTERNAL:0000014c */
/* called by: */

undefined4 __fastcall FUN_004147b0(CDialog *param_1)

{
  char cVar1;
  HWND pHVar2;
  int *piVar3;
  int iVar4;
  BOOL BVar5;
  RAWINPUTDEVICE local_14;
  
  CDialog::OnInitDialog(param_1);
  SendMessageW(*(HWND *)(param_1 + 0x20),0x80,1,*(LPARAM *)(param_1 + 0xb8));
  SendMessageW(*(HWND *)(param_1 + 0x20),0x80,0,*(LPARAM *)(param_1 + 0xb8));
  pHVar2 = FindWindowW((LPCWSTR)0x0,L"Endgame Gear 4k Dongle Firmware Updater");
  if (pHVar2 != (HWND)0x0) {
    AfxMessageBox(L"The 4K dongle is being updated. Please wait until the update is complete before opening the Configuration Tool."
                  ,0,0);
    PostQuitMessage(0);
    return 1;
  }
  local_14.usUsagePage = 1;
  local_14.usUsage = 2;
  local_14.dwFlags = 0x100;
  piVar3 = (int *)FUN_00428aff();
  if (piVar3 == (int *)0x0) {
    iVar4 = 0;
  }
  else {
    iVar4 = (**(code **)(*piVar3 + 0x74))();
  }
  local_14.hwndTarget = *(HWND *)(iVar4 + 0x20);
  BVar5 = RegisterRawInputDevices(&local_14,1,0xc);
  if (BVar5 == 0) {
    AfxMessageBox(L"regist input failed!",0,0);
  }
  iVar4 = FUN_00403500();
  if (iVar4 == 0) {
    AfxMessageBox(L"RegisterDeviceNotification Failed!",0,0);
  }
  RegisterHotKey(*(HWND *)(param_1 + 0x20),1,1,0x50);
  CWnd::SetWindowTextW((CWnd *)param_1,L"Endgame Gear OP1w 4k Configuration Tool");
  FUN_00414940();
  FUN_004150f0();
  FUN_00416120();
  FUN_00416120();
  cVar1 = FUN_00416530();
  FUN_004142b0();
  if (cVar1 == '\x11') {
    CWnd::SetWindowTextW((CWnd *)(param_1 + 0x2dd0),L"Wired");
    FUN_004149a0('\x11',(int)param_1);
  }
  else {
    if (cVar1 == '\"') {
      cVar1 = FUN_004149a0('\"',(int)param_1);
      if (cVar1 == '\x01') goto LAB_0041491b;
    }
    FUN_00416470();
    CWnd::SetWindowTextW((CWnd *)(param_1 + 0x2dd0),L"N/A");
  }
LAB_0041491b:
  CWnd::EnableWindow((CWnd *)(param_1 + 0x2bf8),0);
  return 1;
}


/* ============ FUN_004149a0 @ 004149a0  (req 004149a0) ============ */
/* calls: FUN_00416530@00416530 FUN_00420cf9@00420cf9 FUN_00416470@00416470 __itoa_s@0050b1bb FUN_00401fd0@00401fd0 FUN_004050c0@004050c0 FUN_00409c40@00409c40 FUN_00416680@00416680 FUN_00402190@00402190 FUN_00404230@00404230 FUN_00402260@00402260 FUN_0050b1fb@0050b1fb FUN_0040c2a0@0040c2a0 FUN_00405200@00405200 FUN_00415450@00415450 __security_check_cookie@0050a225 FUN_004035a0@004035a0 FUN_00403d90@00403d90 FUN_00405340@00405340 FUN_004055a0@004055a0 CStringT<wchar_t,StrTraitMFC<wchar_t,ATL::ChTraitsOS<wchar_t>_>_>@00401fa0 EnableWindow@0041dd65 FUN_00405480@00405480 SetWindowTextW@0041dc58 */
/* called by: FUN_004147b0@004147b0 FUN_00415500@00415500 FUN_004168d0@004168d0 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __fastcall FUN_004149a0(char param_1,int param_2)

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
  puStack_c = &LAB_0052d7e0;
  local_10 = ExceptionList;
  uVar3 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
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
  ppuVar4 = FUN_00420cf9();
  if (ppuVar4 == (undefined **)0x0) {
                    /* WARNING: Subroutine does not return */
    FUN_00402190((undefined4 *)0x80004005);
  }
  pwVar5 = (wchar_t *)(**(code **)(*ppuVar4 + 0xc))(uVar3);
  pwVar7 = pwVar5 + 8;
  local_8 = 0;
  local_34 = pwVar7;
  if (param_1 == '\"') {
    DAT_0058384a = 1;
    CWnd::EnableWindow((CWnd *)(param_2 + 0x26ac),1);
    cVar1 = FUN_00405480();
    if (((cVar1 == '\x01') || (local_2d = FUN_004055a0(), local_2d == '\x01')) &&
       (local_2d = FUN_00405340((undefined4 *)&local_50), local_2d == '\x01')) {
      __itoa_s(local_4f & 0xffff,local_20,10,0x10);
      uVar2 = FUN_0050b1fb(local_20);
      if (uVar2 < 200) {
        local_40 = (wchar_t *)((float)uVar2 / (float)_DAT_0055c5b8);
        FUN_00402260(&local_34,L"%.2f");
        CWnd::SetWindowTextW((CWnd *)(param_2 + 0x2a28),local_34);
        pwVar7 = local_34;
      }
      cVar1 = FUN_00405480();
      if (((cVar1 != '\x01') && (local_2d = FUN_004055a0(), local_2d != '\x01')) ||
         (local_2d = FUN_00405200((undefined4 *)&local_50,0x15e), local_2d != '\x01')) {
        FUN_00416680();
        goto LAB_00414d04;
      }
      if (local_4f._1_2_ != 0x1972) {
        *(undefined1 *)(param_2 + 0x3014) = 0xf0;
        DAT_0057ccd8 = 0xff;
        FUN_00416470();
        FUN_00401fd0((int *)&local_34);
        goto LAB_00414cd9;
      }
      *(undefined1 *)(param_2 + 0x3014) = 0xf1;
      __itoa_s(local_4b >> 0x10,local_20,10,0x10);
      uVar2 = FUN_0050b1fb(local_20);
      if (*(char *)(param_2 + 0x3014) == -0xf) {
        pwVar7 = L"Wireless";
      }
      else {
        pwVar7 = L"N/A";
      }
      CWnd::SetWindowTextW((CWnd *)(param_2 + 0x2dd0),pwVar7);
      cVar1 = FUN_00405480();
      if ((cVar1 == '\x01') || (local_2d = FUN_004055a0(), local_2d == '\x01')) {
        local_35 = 0;
        local_2d = FUN_004050c0((void *)0x15e,&local_35);
        if (local_2d == '\x01') {
          if (100 < local_35) {
            local_35 = 100;
          }
          FUN_00402260(&local_34,L"%d");
          FUN_0040c2a0(&local_34,L"%");
          CWnd::SetWindowTextW((CWnd *)(param_2 + 0x2eb8),local_34);
          local_3c = (float)uVar2 / (float)_DAT_0055c5b8;
          FUN_00402260(&local_34,L"%.2f");
          CWnd::SetWindowTextW((CWnd *)(param_2 + 0x2e44),local_34);
          cVar1 = FUN_00405480();
          if (((cVar1 != '\x01') && (local_2d = FUN_004055a0(), local_2d != '\x01')) ||
             (local_2d = FUN_00403d90(400), local_2d != '\x01')) {
            DAT_0057ccd8 = 0xff;
            FUN_00416470();
            FUN_00401fd0((int *)&local_34);
            goto LAB_00414cd9;
          }
          FUN_00416470();
          CWnd::EnableWindow((CWnd *)(param_2 + 0x2bf8),0);
          FUN_00404230();
          FUN_00415450();
          goto LAB_00414cb4;
        }
      }
      DAT_0057ccd8 = 0xff;
      FUN_00416470();
      pwVar7 = local_34;
    }
    else {
LAB_00414d04:
      DAT_0057ccd8 = 0xff;
      FUN_00416470();
    }
    pwVar5 = pwVar7 + -8;
    pwVar7 = pwVar7 + -2;
    LOCK();
    iVar6 = *(int *)pwVar7;
    *(int *)pwVar7 = *(int *)pwVar7 + -1;
    UNLOCK();
  }
  else {
    if (param_1 != '\x11') {
LAB_00414cb4:
      local_8 = 0xffffffff;
      pwVar7 = local_34 + -2;
      LOCK();
      iVar6 = *(int *)pwVar7;
      *(int *)pwVar7 = *(int *)pwVar7 + -1;
      UNLOCK();
      if (iVar6 == 1 || iVar6 + -1 < 0) {
        (**(code **)(**(int **)(local_34 + -8) + 4))();
      }
      goto LAB_00414cd9;
    }
    local_2d = FUN_00405200((undefined4 *)&local_50,0x96);
    if (local_2d == '\x01') {
      *(undefined1 *)(param_2 + 0x3014) = 0xf1;
      __itoa_s(local_4b >> 0x10,local_20,10,0x10);
      uVar2 = FUN_0050b1fb(local_20);
      local_3c = (float)uVar2 / (float)_DAT_0055c5b8;
      FUN_00402260(&local_34,L"%.2f");
      pwVar7 = local_34;
      CWnd::SetWindowTextW((CWnd *)(param_2 + 0x2e44),local_34);
      local_36 = 0;
      local_2d = FUN_004050c0((void *)0x12c,&local_36);
      if (local_2d != '\x01') goto LAB_00414d04;
      if (local_36 < 100) {
        pwVar7 = L"Charging";
      }
      else {
        local_36 = 100;
        FUN_00402260(&local_34,L"%d");
        FUN_0040c2a0(&local_34,L"%");
        pwVar7 = local_34;
      }
      CWnd::SetWindowTextW((CWnd *)(param_2 + 0x2eb8),pwVar7);
      local_2d = FUN_00403d90(0x6e);
      if (local_2d != '\x01') {
        DAT_0057ccd8 = 0xff;
        FUN_00416470();
        FUN_00401fd0((int *)&local_34);
        goto LAB_00414cd9;
      }
      FUN_00416470();
      CWnd::EnableWindow((CWnd *)(param_2 + 0x2bf8),0);
      FUN_00404230();
      FUN_00415450();
      iVar6 = FUN_004035a0((uint)DAT_00584484,(uint)DAT_00584486);
      if (iVar6 != 0) {
        DAT_0058384a = 1;
        FUN_00409c40();
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
          uVar2 = FUN_0050b1fb(local_2c);
          local_3c = (float)uVar2 / (float)_DAT_0055c5b8;
          FUN_00402260(&local_40,L"%.2f");
          CWnd::SetWindowTextW((CWnd *)(param_2 + 0x2a28),local_40);
        }
        local_8 = local_8 & 0xffffff00;
        FUN_00401fd0((int *)&local_40);
      }
      FUN_00416530();
      goto LAB_00414cb4;
    }
    DAT_0057ccd8 = 0xff;
    FUN_00416470();
    pwVar7 = pwVar5 + 6;
    LOCK();
    iVar6 = *(int *)pwVar7;
    *(int *)pwVar7 = *(int *)pwVar7 + -1;
    UNLOCK();
  }
  local_8 = 0xffffffff;
  if (iVar6 + -1 < 1) {
    (**(code **)(**(int **)pwVar5 + 4))();
  }
LAB_00414cd9:
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00415500 @ 00415500  (req 415500) ============ */
/* calls: Sleep@EXTERNAL:00000023 FUN_00416470@00416470 __itoa_s@0050b1bb FUN_00401fd0@00401fd0 FUN_0040c090@0040c090 FUN_00409c40@00409c40 _wcsstr@0050b206 FUN_004149a0@004149a0 FUN_00402260@00402260 FUN_0050b1fb@0050b1fb FUN_004035a0@004035a0 __security_check_cookie@0050a225 FUN_004142b0@004142b0 FUN_00405340@00405340 FUN_004055a0@004055a0 CStringT<wchar_t,StrTraitMFC<wchar_t,ATL::ChTraitsOS<wchar_t>_>_>@00401fa0 FUN_00405480@00405480 SetWindowTextW@0041dc58 */
/* called by: */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall FUN_00415500(void *this,int param_1,int param_2)

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
  puStack_48 = &LAB_0052d820;
  local_4c = ExceptionList;
  local_84 = DAT_0057b3b0 ^ (uint)auStack_b4;
  ExceptionList = &local_4c;
  FUN_0040c090(&local_ac,(short *)(param_2 + 0x1c));
  local_44 = 0;
  if (param_1 == 0x8000) {
    if (DAT_0058448c == (wchar_t *)0x0) goto LAB_00415b12;
    if (-1 < *(int *)(local_ac + -6)) {
      pwVar3 = _wcsstr(local_ac,DAT_0058448c);
      if ((((pwVar3 != (wchar_t *)0x0) && (0 < (int)pwVar3 - (int)local_ac >> 1)) &&
          (DAT_00584490 != (wchar_t *)0x0)) && (-1 < *(int *)(local_ac + -6))) {
        pwVar3 = _wcsstr(local_ac,DAT_00584490);
        if (((pwVar3 != (wchar_t *)0x0) && (0 < (int)pwVar3 - (int)local_ac >> 1)) &&
           (DAT_0057ccd8 != '\x11')) {
          if (DAT_0057ccd8 == '\"') {
            DAT_0057ccd8 = '\x11';
            _DAT_005833b8 = 0;
            iVar4 = FUN_004035a0((uint)DAT_0058447c,(uint)DAT_0058447e);
            if (iVar4 != 0) {
              Sleep(0x44c);
              FUN_004142b0();
              CWnd::SetWindowTextW((CWnd *)((int)this + 0x2dd0),L"Wired");
              FUN_004149a0(DAT_0057ccd8,(int)this);
              goto LAB_00415b12;
            }
          }
          else {
            if (DAT_0057ccd8 != -1) goto LAB_00415b12;
            DAT_0057ccd8 = '\x11';
            iVar4 = FUN_004035a0((uint)DAT_0058447c,(uint)DAT_0058447e);
            if (iVar4 != 0) {
              Sleep(0x44c);
              FUN_004142b0();
              CWnd::SetWindowTextW((CWnd *)((int)this + 0x2dd0),L"Wired");
              FUN_004149a0('\x11',(int)this);
              goto LAB_00415b12;
            }
          }
          CWnd::SetWindowTextW((CWnd *)((int)this + 0x2dd0),L"N/A");
          goto LAB_00415b12;
        }
      }
    }
    if ((DAT_0058448c == (wchar_t *)0x0) || (*(int *)(local_ac + -6) < 0)) goto LAB_00415b12;
    pwVar3 = _wcsstr(local_ac,DAT_0058448c);
    if ((pwVar3 == (wchar_t *)0x0) ||
       ((((int)pwVar3 - (int)local_ac >> 1 < 1 || (DAT_00584494 == (wchar_t *)0x0)) ||
        (*(int *)(local_ac + -6) < 0)))) goto LAB_00415b12;
    pwVar3 = _wcsstr(local_ac,DAT_00584494);
    if (((pwVar3 == (wchar_t *)0x0) || ((int)pwVar3 - (int)local_ac >> 1 < 1)) ||
       (DAT_0058384a != '\0')) goto LAB_00415b12;
    DAT_0058384a = '\x01';
    FUN_00409c40();
    if (DAT_0057ccd8 == -1) {
      iVar4 = FUN_004035a0((uint)DAT_00584484,(uint)DAT_00584486);
      if (iVar4 == 0) goto LAB_00415b12;
      Sleep(0x44c);
LAB_00415a01:
      DAT_0057ccd8 = '\"';
      FUN_004142b0();
      FUN_004149a0('\"',(int)this);
      goto LAB_00415b12;
    }
    if (DAT_0057ccd8 != '\x11') goto LAB_00415b12;
    FUN_004035a0((uint)DAT_00584484,(uint)DAT_00584486);
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
LAB_00415a8b:
      cVar1 = FUN_00405340((undefined4 *)&local_a0);
      if (cVar1 == '\x01') {
        __itoa_s((uint)CONCAT11((undefined1)local_9b,local_9f._3_1_),local_90,10,0x10);
        uVar2 = FUN_0050b1fb(local_90);
        local_a8 = (float)uVar2 / (float)_DAT_0055c5b8;
        FUN_00402260(&local_a4,L"%.2f");
        CWnd::SetWindowTextW((CWnd *)((int)this + 0x2a28),local_a4);
      }
    }
    else {
      cVar1 = FUN_004055a0();
      if (cVar1 == '\x01') goto LAB_00415a8b;
    }
    FUN_00401fd0((int *)&local_a4);
    goto LAB_00415b12;
  }
  if ((param_1 != 0x8004) || (DAT_0058448c == (wchar_t *)0x0)) goto LAB_00415b12;
  if (*(int *)(local_ac + -6) < 0) {
LAB_00415688:
    if ((DAT_0058448c == (wchar_t *)0x0) || (*(int *)(local_ac + -6) < 0)) goto LAB_00415b12;
    pwVar3 = _wcsstr(local_ac,DAT_0058448c);
    if ((pwVar3 == (wchar_t *)0x0) ||
       ((((int)pwVar3 - (int)local_ac >> 1 < 1 || (DAT_00584494 == (wchar_t *)0x0)) ||
        (*(int *)(local_ac + -6) < 0)))) goto LAB_00415b12;
    pwVar3 = _wcsstr(local_ac,DAT_00584494);
    if (((pwVar3 == (wchar_t *)0x0) || ((int)pwVar3 - (int)local_ac >> 1 < 1)) ||
       (DAT_0058384a != '\x01')) goto LAB_00415b12;
    DAT_0058384a = '\0';
    FUN_00409c40();
    if (DAT_0057ccd8 != '\x11') {
      DAT_00583443 = 0;
      *(undefined1 *)((int)this + 0x3014) = 0;
      _DAT_005833b4 = 0;
      _DAT_005833b8 = 0;
      FUN_00416470();
      _DAT_005833b4 = 0;
      _DAT_005833b8 = 0;
      DAT_0057ccd8 = -1;
      CWnd::SetWindowTextW((CWnd *)((int)this + 0x2a28),L"");
      CWnd::SetWindowTextW((CWnd *)((int)this + 0x2e44),L"");
      CWnd::SetWindowTextW((CWnd *)((int)this + 0x2dd0),L"N/A");
      pCVar5 = (CWnd *)((int)this + 0x2fa0);
      goto LAB_004157a9;
    }
    *(undefined1 *)((int)this + 0x3014) = 0xf1;
    pCVar5 = (CWnd *)((int)this + 0x2a28);
  }
  else {
    pwVar3 = _wcsstr(local_ac,DAT_0058448c);
    if ((((pwVar3 == (wchar_t *)0x0) || ((int)pwVar3 - (int)local_ac >> 1 < 1)) ||
        (DAT_00584490 == (wchar_t *)0x0)) || (*(int *)(local_ac + -6) < 0)) goto LAB_00415688;
    pwVar3 = _wcsstr(local_ac,DAT_00584490);
    if (((pwVar3 == (wchar_t *)0x0) || ((int)pwVar3 - (int)local_ac >> 1 < 1)) ||
       (DAT_0057ccd8 != '\x11')) goto LAB_00415688;
    _DAT_005833b8 = 0;
    _DAT_005833b4 = 0;
    CWnd::SetWindowTextW((CWnd *)((int)this + 0x2e44),L"");
    CWnd::SetWindowTextW((CWnd *)((int)this + 0x2eb8),L"");
    iVar4 = FUN_004035a0((uint)DAT_00584484,(uint)DAT_00584486);
    if (iVar4 != 0) {
      DAT_0058384a = '\x01';
      FUN_00409c40();
      goto LAB_00415a01;
    }
    _DAT_005833b4 = 0;
    _DAT_005833b8 = 0;
    FUN_00416470();
    CWnd::SetWindowTextW((CWnd *)((int)this + 0x2dd0),L"N/A");
    DAT_0057ccd8 = -1;
    pCVar5 = (CWnd *)((int)this + 0x2a28);
LAB_004157a9:
    CWnd::SetWindowTextW(pCVar5,L"");
    pCVar5 = (CWnd *)((int)this + 0x2eb8);
  }
  CWnd::SetWindowTextW(pCVar5,L"");
  CWnd::SetWindowTextW((CWnd *)((int)this + 0x2f2c),L"");
LAB_00415b12:
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


/* ============ FUN_004168d0 @ 004168d0  (req 004168d0) ============ */
/* calls: FUN_00405200@00405200 FUN_004035a0@004035a0 FUN_004166f0@004166f0 FUN_004142b0@004142b0 FUN_004055a0@004055a0 EnableWindow@0041dd65 FUN_004149a0@004149a0 FUN_00405480@00405480 SetWindowTextW@0041dc58 */
/* called by: FUN_00416790@00416790 FUN_00415b70@00415b70 */

void __fastcall FUN_004168d0(int param_1)

{
  char cVar1;
  int iVar2;
  undefined1 local_18;
  undefined4 local_17;
  undefined4 local_13;
  undefined4 local_f;
  undefined1 local_b;
  
  if (DAT_0057ccd8 == -1) {
    if (*(char *)(param_1 + 0x3014) == '\0') {
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
          if (local_17._1_2_ == 0x1972) {
            *(undefined1 *)(param_1 + 0x3014) = 0xf1;
          }
          else {
            *(undefined1 *)(param_1 + 0x3014) = 0xf0;
            DAT_0057ccd8 = -1;
          }
        }
      }
    }
    if (*(char *)(param_1 + 0x3014) == -0xf) {
      iVar2 = FUN_004035a0((uint)DAT_00584484,(uint)DAT_00584486);
      if (iVar2 != 0) {
        FUN_004142b0();
        DAT_0058384a = 1;
        CWnd::EnableWindow((CWnd *)(param_1 + 0x26ac),1);
        DAT_0057ccd8 = '\"';
        CWnd::SetWindowTextW((CWnd *)(param_1 + 0x2dd0),L"Wireless");
        FUN_004149a0('\"',param_1);
        FUN_004166f0();
        CWnd::EnableWindow((CWnd *)(param_1 + 0x2bf8),0);
      }
    }
  }
  return;
}


/* ============ FUN_00416360 @ 00416360  (req 416360) ============ */
/* calls: FUN_00416530@00416530 FUN_00416120@00416120 FUN_004097e0@004097e0 FUN_004112b0@004112b0 FUN_0040fa10@0040fa10 FUN_004055a0@004055a0 EnableWindow@0041dd65 FUN_00405480@00405480 FUN_004056c0@004056c0 FUN_0040a040@0040a040 */
/* called by: */

void __fastcall FUN_00416360(int param_1)

{
  char cVar1;
  ushort uVar2;
  
  if ((*(int *)(param_1 + 0x2c6c) == 0) && (*(int *)(param_1 + 0x2c6c) == 0)) {
    *(undefined4 *)(param_1 + 0x2c6c) = 1;
    uVar2 = 200;
    if (DAT_0057ccd8 == '\x11') {
      uVar2 = 800;
      if (DAT_00583442 == '\x01') {
        cVar1 = FUN_00416530();
        if (cVar1 == '\x11') {
          DAT_00583442 = '\0';
        }
      }
    }
    else if (DAT_0057ccd8 == '\"') {
      uVar2 = 0x438;
      cVar1 = FUN_00405480();
      if (cVar1 != '\x01') {
        FUN_004055a0();
      }
    }
    if (DAT_0057ccd8 == -1) {
      FUN_00416530();
    }
    if (DAT_0057ccd8 != -1) {
      cVar1 = FUN_004056c0(uVar2);
      if (cVar1 == '\x01') {
        FUN_00416120();
        FUN_00416120();
        FUN_004112b0();
        FUN_0040fa10();
        FUN_004097e0();
        FUN_0040a040();
        CWnd::EnableWindow((CWnd *)(param_1 + 0x2bf8),0);
      }
    }
    *(undefined4 *)(param_1 + 0x2c6c) = 0;
  }
  return;
}


/* ============ FUN_00416530 @ 00416530  (req 416530) ============ */
/* calls: FUN_004035a0@004035a0 */
/* called by: FUN_004147b0@004147b0 FUN_004149a0@004149a0 FUN_00416200@00416200 FUN_00416360@00416360 */

undefined1 FUN_00416530(void)

{
  int iVar1;
  
  DAT_0057ccd8 = 0xff;
  iVar1 = FUN_004035a0((uint)DAT_0058447c,(uint)DAT_0058447e);
  if (iVar1 != 0) {
    DAT_0057ccd8 = 0x11;
    return 0x11;
  }
  iVar1 = FUN_004035a0((uint)DAT_00584484,(uint)DAT_00584486);
  if (iVar1 != 0) {
    DAT_0058384a = 1;
    DAT_0057ccd8 = 0x22;
  }
  return DAT_0057ccd8;
}


/* ============ FUN_00416590 @ 00416590  (req 416590) ============ */
/* calls: __security_check_cookie@0050a225 _memset@0050b7b0 FUN_00404dc0@00404dc0 EnableWindow@0041dd65 */
/* called by: FUN_004070f0@004070f0 FUN_0040a140@0040a140 FUN_0040a500@0040a500 FUN_0040d350@0040d350 FUN_00411f20@00411f20 FUN_00412040@00412040 FUN_00412130@00412130 FUN_00412220@00412220 FUN_00412310@00412310 FUN_00412400@00412400 FUN_00411e40@00411e40 FUN_00411eb0@00411eb0 */

void FUN_00416590(void)

{
  int iVar1;
  int iVar2;
  char local_f0 [116];
  char local_7c [116];
  uint local_8;
  
  iVar1 = DAT_0058431c;
  local_8 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  local_7c[0] = '\0';
  _memset(local_7c + 1,0,0x71);
  local_f0[0] = '\0';
  _memset(local_f0 + 1,0,0x71);
  FUN_00404dc0(&DAT_00583850);
  FUN_00404dc0(&DAT_00584320);
  iVar2 = 0;
  do {
    if (local_7c[iVar2] != local_f0[iVar2]) {
      CWnd::EnableWindow((CWnd *)(iVar1 + 0x2bf8),1);
      __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
      return;
    }
    if (iVar2 == 0x71) {
      CWnd::EnableWindow((CWnd *)(iVar1 + 0x2bf8),0);
    }
    iVar2 = iVar2 + 1;
  } while (iVar2 < 0x72);
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}


/* ---- disassembly context: the factory-reset call site ----

==== 004163f0  (FUN_00416360 @ 00416360) ====
004163f2  80 fa ff                 CMP DL,0xff
004163f5  74 5a                    JZ 0x00416451
004163f7  56                       PUSH ESI
004163f8  e8 c3 f2 fe ff           CALL 0x004056c0
004163fd  83 c4 04                 ADD ESP,0x4
00416400  3c 01                    CMP AL,0x1
00416402  75 4d                    JNZ 0x00416451
00416404  b8 50 38 58 00           MOV EAX,0x583850
00416409  e8 12 fd ff ff           CALL 0x00416120
0041640e  b8 20 43 58 00           MOV EAX,0x584320
00416413  e8 08 fd ff ff           CALL 0x00416120
00416418  8d b7 a8 16 00 00        LEA ESI,[EDI + 0x16a8]

*/


