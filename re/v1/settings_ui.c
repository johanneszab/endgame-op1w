/* v1/settings_ui.c -- everything that writes the settings struct at 0x00583850.
 *
 * Program: Endgame_Gear_OP1w_4k_Configuration_Tool_v1.03.exe
 *   (OP1w 4k **v1** vendor tool; Ghidra project OP1w, image base 0x00400000)
 * Extracted with Ghidra 12.1.4 headless, scripts in git/re/ghidra_scripts.
 * Evidence class for everything in this file: [BIN].
 *
 * This file exists to answer one question: which code writes each field that
 * the cmd 0x14 serializer reads, and in particular settings +0x08, the active
 * CPI stage.
 *
 * Addresses covered:
 *   FUN_0040f820   Basic Settings tab   -> struct   (the CPI tab harvest)
 *   FUN_0040fa10   struct -> Basic Settings tab     (the reverse)
 *   FUN_0040d350   CPI edit/spin controls -> struct (per-stage X/Y)
 *   FUN_004110e0   Performance tab      -> struct   (polling, flags, filters)
 *   FUN_004112b0   struct -> Performance tab
 *   FUN_00409f90   power-timeout controls -> struct (+0x05, +0x06)
 *   FUN_0040a040   struct -> power-timeout controls
 *   FUN_004097e0   button-table UI
 *   FUN_00416470   bulk EnableWindow over the whole control set
 *   FUN_004166f0   hides the v2-only tab pages
 *
 * ==================== THE ANSWER, from RangeXrefs.java ====================
 *
 * The full reference map of 0x00583850 + 0x30 is appended at the bottom. The
 * line for the active-CPI-stage field is:
 *
 *   +0x08  00583858  [FUN_00416200@004162ff:DATA]
 *
 * That single reference is the `MOV ECX,0x583858` in the Apply handler which
 * passes the field's address as the cmd 0x14 struct base. There is NO write
 * reference to 0x00583858 anywhere in the binary. The only code that writes
 * the field at all does so base-relative, and there are exactly two such
 * places, both in v1/blob_parser.c:
 *
 *   1. FUN_00404230, the blob parser:   in_EAX[8] = DAT_00583455  (blob 0x0D)
 *   2. FUN_00416120, the defaults filler: *(undefined4*)(in_EAX+6) = 0x04021103
 *
 * No control, no radio button, no combo box, no message handler writes it.
 * Compare +0x09 (CPI levels), which FUN_0040f820 writes from a combo box at
 * four separate sites, and +0x12/+0x14/... (the per-stage CPI values), which
 * FUN_0040f820 and FUN_0040d350 both write.
 *
 * Note also that FUN_0040fa10, the struct -> Basic Settings tab populate, does
 * not READ +0x08 either: the v1 Basic Settings tab has no widget for the active
 * stage in either direction. That matches the v1 screenshots, which show no
 * radio buttons beside the four CPI rows. [UI]
 *
 * FUN_0040f820 is also the [BIN] evidence for two v1/v2 differences:
 *   - lift-off distance (+0x0F) is only ever set to 1 or 2 -- whole millimetres
 *   - the CPI-levels combo yields 1..4 into +0x09
 * and FUN_004110e0 is the evidence for two more:
 *   - the polling combo yields only 8, 4, 2 into +0x04 (no 0x40, no 0x80)
 *   - the flags byte (+0x07) is built from two checkboxes as 0x01 | 0x10,
 *     i.e. bit 0 and bit 4 -- bit 4 being the v1-only motion jitter filter
 */

/* ============ FUN_0040f820 @ 0040f820  (req 0040f820) ============ */
/* calls: UpdateData@00419797 SendMessageW@EXTERNAL:00000183 */
/* called by: FUN_00416200@00416200 */

void FUN_0040f820(void)

{
  LRESULT LVar1;
  CWnd *unaff_ESI;
  
  CWnd::UpdateData(unaff_ESI,1);
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x6e4),0x147,0,0);
  if (LVar1 == 0) {
    DAT_0058385f = '\x01';
  }
  else {
    LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x6e4),0x147,0,0);
    DAT_0058385f = '\x02' - (LVar1 != 1);
  }
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0xf20),0xf0,0,0);
  DAT_0058385b = LVar1 != 0;
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x1278),0xf0,0,0);
  DAT_0058385e = LVar1 == 0;
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0xf94),0xf0,0,0);
  DAT_0058385c = LVar1 != 0;
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0xf0),0x147,0,0);
  if (LVar1 == 0) {
    DAT_00583859 = 1;
  }
  else {
    LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0xf0),0x147,0,0);
    if (LVar1 == 1) {
      DAT_00583859 = 2;
    }
    else {
      LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0xf0),0x147,0,0);
      if (LVar1 == 2) {
        DAT_00583859 = 3;
      }
      else {
        LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0xf0),0x147,0,0);
        if (LVar1 == 3) {
          DAT_00583859 = 4;
        }
      }
    }
  }
  DAT_00583862 = *(undefined2 *)(unaff_ESI + 0x4e4);
  DAT_00583864 = *(undefined2 *)(unaff_ESI + 0xe78);
  DAT_00583860 = *(int *)(unaff_ESI + 0x4e4) != *(int *)(unaff_ESI + 0xe78);
  DAT_00583868 = *(undefined2 *)(unaff_ESI + 0x4e8);
  DAT_0058386a = *(undefined2 *)(unaff_ESI + 0xe7c);
  DAT_00583866 = *(int *)(unaff_ESI + 0x4e8) != *(int *)(unaff_ESI + 0xe7c);
  DAT_0058386e = *(undefined2 *)(unaff_ESI + 0x4ec);
  DAT_00583870 = *(undefined2 *)(unaff_ESI + 0xe80);
  DAT_0058386c = *(int *)(unaff_ESI + 0x4ec) != *(int *)(unaff_ESI + 0xe80);
  DAT_00583874 = *(undefined2 *)(unaff_ESI + 0x4f0);
  DAT_00583876 = *(undefined2 *)(unaff_ESI + 0xe84);
  DAT_00583872 = *(int *)(unaff_ESI + 0x4ec) != *(int *)(unaff_ESI + 0xe80);
  return;
}


/* ============ FUN_0040fa10 @ 0040fa10  (req 0040fa10) ============ */
/* calls: UpdateData@00419797 SendMessageW@EXTERNAL:00000183 FUN_0040d8b0@0040d8b0 */
/* called by: FUN_00415450@00415450 FUN_00416360@00416360 */

void FUN_0040fa10(void)

{
  ushort uVar1;
  short sVar2;
  short sVar3;
  CWnd *in_EAX;
  uint uVar4;
  HWND pHVar5;
  WPARAM WVar6;
  
  if (DAT_0058385e != '\x01') {
    pHVar5 = *(HWND *)(in_EAX + 0x1278);
  }
  else {
    pHVar5 = *(HWND *)(in_EAX + 0x1278);
  }
  SendMessageW(pHVar5,0xf1,(uint)(DAT_0058385e != '\x01'),0);
  if (DAT_0058385f == '\x01') {
    pHVar5 = *(HWND *)(in_EAX + 0x6e4);
    WVar6 = 0;
  }
  else if (DAT_0058385f == '\x02') {
    pHVar5 = *(HWND *)(in_EAX + 0x6e4);
    WVar6 = 1;
  }
  else {
    pHVar5 = *(HWND *)(in_EAX + 0x6e4);
    WVar6 = 0;
  }
  SendMessageW(pHVar5,0x14e,WVar6,0);
  if (DAT_0058385b != '\x01') {
    pHVar5 = *(HWND *)(in_EAX + 0xf20);
  }
  else {
    pHVar5 = *(HWND *)(in_EAX + 0xf20);
  }
  SendMessageW(pHVar5,0xf1,(uint)(DAT_0058385b == '\x01'),0);
  if (DAT_0058385c != '\x01') {
    pHVar5 = *(HWND *)(in_EAX + 0xf94);
  }
  else {
    pHVar5 = *(HWND *)(in_EAX + 0xf94);
  }
  SendMessageW(pHVar5,0xf1,(uint)(DAT_0058385c == '\x01'),0);
  if (DAT_00583859 == '\x01') {
    pHVar5 = *(HWND *)(in_EAX + 0xf0);
    WVar6 = 0;
    goto LAB_0040fb08;
  }
  if (DAT_00583859 == '\x02') {
    WVar6 = 1;
  }
  else {
    if (DAT_00583859 == '\x03') {
      pHVar5 = *(HWND *)(in_EAX + 0xf0);
      WVar6 = 2;
      goto LAB_0040fb08;
    }
    WVar6 = 3;
    if (DAT_00583859 == '\x04') {
      pHVar5 = *(HWND *)(in_EAX + 0xf0);
      goto LAB_0040fb08;
    }
  }
  pHVar5 = *(HWND *)(in_EAX + 0xf0);
LAB_0040fb08:
  SendMessageW(pHVar5,0x14e,WVar6,0);
  *(uint *)(in_EAX + 0xefc) = (uint)(DAT_00583860 != '\0');
  uVar1 = DAT_00583862;
  uVar4 = (uint)DAT_00583862;
  *(uint *)(in_EAX + 0x4e4) = uVar4;
  *(uint *)(in_EAX + 0xe78) = (uint)DAT_00583864;
  if (uVar4 < 0x32) {
    sVar2 = 0x32;
  }
  else if (uVar4 < 0x6591) {
    uVar1 = uVar1 / 0x32;
    if (0x18 < uVar4 % 0x32) {
      uVar1 = uVar1 + 1;
    }
    sVar2 = uVar1 * 0x32;
  }
  else {
    sVar2 = 26000;
  }
  uVar4 = *(uint *)(in_EAX + 0xe78);
  *(int *)(in_EAX + 0x4e4) = (int)sVar2;
  if (uVar4 < 0x32) {
    sVar3 = 0x32;
  }
  else if (uVar4 < 0x6591) {
    sVar3 = (short)(uVar4 / 0x32);
    if (0x18 < uVar4 % 0x32) {
      sVar3 = sVar3 + 1;
    }
    sVar3 = sVar3 * 0x32;
  }
  else {
    sVar3 = 26000;
  }
  *(int *)(in_EAX + 0xe78) = (int)sVar3;
  SendMessageW(*(HWND *)(in_EAX + 0x164),0x405,1,(uint)(int)sVar2 / 0x32);
  SendMessageW(*(HWND *)(in_EAX + 0x758),0x405,1,*(uint *)(in_EAX + 0xe78) / 0x32);
  uVar1 = DAT_00583868;
  uVar4 = (uint)DAT_00583868;
  *(uint *)(in_EAX + 0x4e8) = uVar4;
  *(uint *)(in_EAX + 0xe7c) = (uint)DAT_0058386a;
  if (uVar4 < 0x32) {
    sVar2 = 0x32;
  }
  else if (uVar4 < 0x6591) {
    uVar1 = uVar1 / 0x32;
    if (0x18 < uVar4 % 0x32) {
      uVar1 = uVar1 + 1;
    }
    sVar2 = uVar1 * 0x32;
  }
  else {
    sVar2 = 26000;
  }
  uVar4 = *(uint *)(in_EAX + 0xe7c);
  *(int *)(in_EAX + 0x4e8) = (int)sVar2;
  if (uVar4 < 0x32) {
    sVar3 = 0x32;
  }
  else if (uVar4 < 0x6591) {
    sVar3 = (short)(uVar4 / 0x32);
    if (0x18 < uVar4 % 0x32) {
      sVar3 = sVar3 + 1;
    }
    sVar3 = sVar3 * 0x32;
  }
  else {
    sVar3 = 26000;
  }
  *(int *)(in_EAX + 0xe7c) = (int)sVar3;
  SendMessageW(*(HWND *)(in_EAX + 0x1d8),0x405,1,(uint)(int)sVar2 / 0x32);
  SendMessageW(*(HWND *)(in_EAX + 0x7cc),0x405,1,*(uint *)(in_EAX + 0xe7c) / 0x32);
  uVar1 = DAT_0058386e;
  uVar4 = (uint)DAT_0058386e;
  *(uint *)(in_EAX + 0x4ec) = uVar4;
  *(uint *)(in_EAX + 0xe80) = (uint)DAT_00583870;
  if (uVar4 < 0x32) {
    sVar2 = 0x32;
  }
  else if (uVar4 < 0x6591) {
    uVar1 = uVar1 / 0x32;
    if (0x18 < uVar4 % 0x32) {
      uVar1 = uVar1 + 1;
    }
    sVar2 = uVar1 * 0x32;
  }
  else {
    sVar2 = 26000;
  }
  uVar4 = *(uint *)(in_EAX + 0xe80);
  *(int *)(in_EAX + 0x4ec) = (int)sVar2;
  if (uVar4 < 0x32) {
    sVar3 = 0x32;
  }
  else if (uVar4 < 0x6591) {
    sVar3 = (short)(uVar4 / 0x32);
    if (0x18 < uVar4 % 0x32) {
      sVar3 = sVar3 + 1;
    }
    sVar3 = sVar3 * 0x32;
  }
  else {
    sVar3 = 26000;
  }
  *(int *)(in_EAX + 0xe80) = (int)sVar3;
  SendMessageW(*(HWND *)(in_EAX + 0x24c),0x405,1,(uint)(int)sVar2 / 0x32);
  SendMessageW(*(HWND *)(in_EAX + 0x840),0x405,1,*(uint *)(in_EAX + 0xe80) / 0x32);
  uVar1 = DAT_00583874;
  uVar4 = (uint)DAT_00583874;
  *(uint *)(in_EAX + 0x4f0) = uVar4;
  *(uint *)(in_EAX + 0xe84) = (uint)DAT_00583876;
  if (uVar4 < 0x32) {
    sVar2 = 0x32;
  }
  else if (uVar4 < 0x6591) {
    uVar1 = uVar1 / 0x32;
    if (0x18 < uVar4 % 0x32) {
      uVar1 = uVar1 + 1;
    }
    sVar2 = uVar1 * 0x32;
  }
  else {
    sVar2 = 26000;
  }
  uVar4 = *(uint *)(in_EAX + 0xe84);
  *(int *)(in_EAX + 0x4f0) = (int)sVar2;
  if (uVar4 < 0x32) {
    sVar3 = 0x32;
  }
  else if (uVar4 < 0x6591) {
    sVar3 = (short)(uVar4 / 0x32);
    if (0x18 < uVar4 % 0x32) {
      sVar3 = sVar3 + 1;
    }
    sVar3 = sVar3 * 0x32;
  }
  else {
    sVar3 = 26000;
  }
  *(int *)(in_EAX + 0xe84) = (int)sVar3;
  SendMessageW(*(HWND *)(in_EAX + 0x2c0),0x405,1,(uint)(int)sVar2 / 0x32);
  SendMessageW(*(HWND *)(in_EAX + 0x8b4),0x405,1,*(uint *)(in_EAX + 0xe84) / 0x32);
  FUN_0040d8b0();
  CWnd::UpdateData(in_EAX,0);
  return;
}


/* ============ FUN_0040d350 @ 0040d350  (req 0040d350) ============ */
/* calls: SendMessageW@EXTERNAL:00000183 GetDlgCtrlID@0041dc9f FUN_00416590@00416590 OnVScroll@0041a995 FUN_00401ed0@00401ed0 UpdateData@00419797 */
/* called by: */

void __thiscall FUN_0040d350(void *this,uint param_1,uint param_2,CWnd *param_3)

{
  LRESULT LVar1;
  int iVar2;
  LPARAM LVar3;
  HWND pHVar4;
  
  if (param_3 == (CWnd *)0x0) goto LAB_0040d843;
  LVar1 = SendMessageW(*(HWND *)((int)this + 0xea8),0xf0,0,0);
  if (LVar1 == 0) {
    iVar2 = CWnd::GetDlgCtrlID(param_3);
    if (iVar2 == 0x405) {
      *(undefined1 *)((int)this + 0xb9) = 1;
      LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
      *(LRESULT *)((int)this + 0x4e4) = LVar1 * 0x32;
      LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
      *(LRESULT *)((int)this + 0xe78) = LVar1 * 0x32;
      LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
      pHVar4 = *(HWND *)((int)this + 0x758);
    }
    else {
      iVar2 = CWnd::GetDlgCtrlID(param_3);
      if (iVar2 != 0x40d) {
        iVar2 = CWnd::GetDlgCtrlID(param_3);
        if (iVar2 == 0x406) {
          *(undefined1 *)((int)this + 0xb9) = 2;
          LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
          *(LRESULT *)((int)this + 0x4e8) = LVar1 * 0x32;
          LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
          *(LRESULT *)((int)this + 0xe7c) = LVar1 * 0x32;
          LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
          pHVar4 = *(HWND *)((int)this + 0x7cc);
        }
        else {
          iVar2 = CWnd::GetDlgCtrlID(param_3);
          if (iVar2 != 0x40e) {
            iVar2 = CWnd::GetDlgCtrlID(param_3);
            if (iVar2 == 0x407) {
              *(undefined1 *)((int)this + 0xb9) = 3;
              iVar2 = FUN_00401ed0();
              *(int *)((int)this + 0x4ec) = iVar2 * 0x32;
              iVar2 = FUN_00401ed0();
              *(int *)((int)this + 0xe80) = iVar2 * 0x32;
              LVar3 = FUN_00401ed0();
              pHVar4 = *(HWND *)((int)this + 0x840);
            }
            else {
              iVar2 = CWnd::GetDlgCtrlID(param_3);
              if (iVar2 != 0x410) {
                iVar2 = CWnd::GetDlgCtrlID(param_3);
                if (iVar2 == 0x408) {
                  *(undefined1 *)((int)this + 0xb9) = 4;
                  iVar2 = FUN_00401ed0();
                  *(int *)((int)this + 0x4f0) = iVar2 * 0x32;
                  iVar2 = FUN_00401ed0();
                  *(int *)((int)this + 0xe84) = iVar2 * 0x32;
                  LVar3 = FUN_00401ed0();
                  pHVar4 = *(HWND *)((int)this + 0x8b4);
                }
                else {
                  iVar2 = CWnd::GetDlgCtrlID(param_3);
                  if (iVar2 != 0x411) goto LAB_0040d83d;
                  *(undefined1 *)((int)this + 0xb9) = 4;
                  iVar2 = FUN_00401ed0();
                  *(int *)((int)this + 0x4f0) = iVar2 * 0x32;
                  iVar2 = FUN_00401ed0();
                  *(int *)((int)this + 0xe84) = iVar2 * 0x32;
                  LVar3 = FUN_00401ed0();
                  pHVar4 = *(HWND *)((int)this + 0x2c0);
                }
                SendMessageW(pHVar4,0x405,1,LVar3);
                DAT_00583874 = *(undefined2 *)((int)this + 0x4f0);
                DAT_00583876 = *(undefined2 *)((int)this + 0xe84);
                goto LAB_0040d83d;
              }
              *(undefined1 *)((int)this + 0xb9) = 3;
              iVar2 = FUN_00401ed0();
              *(int *)((int)this + 0x4ec) = iVar2 * 0x32;
              iVar2 = FUN_00401ed0();
              *(int *)((int)this + 0xe80) = iVar2 * 0x32;
              LVar3 = FUN_00401ed0();
              pHVar4 = *(HWND *)((int)this + 0x24c);
            }
            SendMessageW(pHVar4,0x405,1,LVar3);
            DAT_0058386e = *(undefined2 *)((int)this + 0x4ec);
            DAT_00583870 = *(undefined2 *)((int)this + 0xe80);
            goto LAB_0040d83d;
          }
          *(undefined1 *)((int)this + 0xb9) = 2;
          iVar2 = FUN_00401ed0();
          *(int *)((int)this + 0x4e8) = iVar2 * 0x32;
          iVar2 = FUN_00401ed0();
          *(int *)((int)this + 0xe7c) = iVar2 * 0x32;
          LVar1 = FUN_00401ed0();
          pHVar4 = *(HWND *)((int)this + 0x1d8);
        }
        SendMessageW(pHVar4,0x405,1,LVar1);
        DAT_00583868 = *(undefined2 *)((int)this + 0x4e8);
        DAT_0058386a = *(undefined2 *)((int)this + 0xe7c);
        goto LAB_0040d83d;
      }
      *(undefined1 *)((int)this + 0xb9) = 1;
      LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
      *(LRESULT *)((int)this + 0x4e4) = LVar1 * 0x32;
      LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
      *(LRESULT *)((int)this + 0xe78) = LVar1 * 0x32;
      LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
      pHVar4 = *(HWND *)((int)this + 0x164);
    }
    SendMessageW(pHVar4,0x405,1,LVar1);
    DAT_00583862 = *(undefined2 *)((int)this + 0x4e4);
    DAT_00583864 = *(undefined2 *)((int)this + 0xe78);
  }
  else {
    iVar2 = CWnd::GetDlgCtrlID(param_3);
    if (iVar2 == 0x405) {
      *(undefined1 *)((int)this + 0xb9) = 1;
      LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
      DAT_00583862 = (undefined2)(LVar1 * 0x32);
      *(LRESULT *)((int)this + 0x4e4) = LVar1 * 0x32;
    }
    else {
      iVar2 = CWnd::GetDlgCtrlID(param_3);
      if (iVar2 == 0x40d) {
        *(undefined1 *)((int)this + 0xb9) = 1;
        LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
        DAT_00583864 = (undefined2)(LVar1 * 0x32);
        *(LRESULT *)((int)this + 0xe78) = LVar1 * 0x32;
      }
      else {
        iVar2 = CWnd::GetDlgCtrlID(param_3);
        if (iVar2 == 0x406) {
          *(undefined1 *)((int)this + 0xb9) = 2;
          LVar1 = SendMessageW(*(HWND *)(param_3 + 0x20),0x400,0,0);
          *(LRESULT *)((int)this + 0x4e8) = LVar1 * 0x32;
          DAT_00583868 = (undefined2)(LVar1 * 0x32);
        }
        else {
          iVar2 = CWnd::GetDlgCtrlID(param_3);
          if (iVar2 == 0x40e) {
            *(undefined1 *)((int)this + 0xb9) = 2;
            iVar2 = FUN_00401ed0();
            DAT_0058386a = (undefined2)(iVar2 * 0x32);
            *(int *)((int)this + 0xe7c) = iVar2 * 0x32;
          }
          else {
            iVar2 = CWnd::GetDlgCtrlID(param_3);
            if (iVar2 == 0x407) {
              *(undefined1 *)((int)this + 0xb9) = 3;
              iVar2 = FUN_00401ed0();
              DAT_0058386e = (undefined2)(iVar2 * 0x32);
              *(int *)((int)this + 0x4ec) = iVar2 * 0x32;
            }
            else {
              iVar2 = CWnd::GetDlgCtrlID(param_3);
              if (iVar2 == 0x410) {
                *(undefined1 *)((int)this + 0xb9) = 3;
                iVar2 = FUN_00401ed0();
                *(int *)((int)this + 0xe80) = iVar2 * 0x32;
                DAT_00583870 = (undefined2)(iVar2 * 0x32);
              }
              else {
                iVar2 = CWnd::GetDlgCtrlID(param_3);
                if (iVar2 == 0x408) {
                  *(undefined1 *)((int)this + 0xb9) = 4;
                  iVar2 = FUN_00401ed0();
                  DAT_00583874 = (undefined2)(iVar2 * 0x32);
                  *(int *)((int)this + 0x4f0) = iVar2 * 0x32;
                }
                else {
                  iVar2 = CWnd::GetDlgCtrlID(param_3);
                  if (iVar2 == 0x411) {
                    *(undefined1 *)((int)this + 0xb9) = 4;
                    iVar2 = FUN_00401ed0();
                    *(int *)((int)this + 0xe84) = iVar2 * 0x32;
                    DAT_00583876 = (undefined2)(iVar2 * 0x32);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
LAB_0040d83d:
  FUN_00416590();
LAB_0040d843:
  CWnd::OnVScroll(this,param_1,param_2,(CScrollBar *)param_3);
  CWnd::UpdateData(this,0);
  return;
}


/* ============ FUN_004110e0 @ 004110e0  (req 004110e0) ============ */
/* calls: SendMessageW@EXTERNAL:00000183 UpdateData@00419797 */
/* called by: FUN_00416200@00416200 */

void FUN_004110e0(void)

{
  LRESULT LVar1;
  undefined1 uVar2;
  CWnd *unaff_ESI;
  
  CWnd::UpdateData(unaff_ESI,1);
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x480),0xf0,0,0);
  DAT_0058385d = LVar1 != 0;
  uVar2 = 0;
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x738),0xf0,0,0);
  if (LVar1 == 0) {
    LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x7b0),0xf0,0,0);
    if (LVar1 != 0) {
      uVar2 = 0x10;
    }
  }
  else {
    uVar2 = 1;
    LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x7b0),0xf0,0,0);
    if (LVar1 != 0) {
      uVar2 = 0x11;
    }
  }
  DAT_00583857 = uVar2;
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x40c),0x147,0,0);
  if (LVar1 == 0) {
    DAT_00583854 = 8;
  }
  else if (LVar1 == 1) {
    DAT_00583854 = 4;
  }
  else {
    DAT_00583854 = 2;
  }
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x324),0x147,0,0);
  if (LVar1 == 0) {
    LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0xd8),0x400,0,0);
    DAT_0058387e = (char)LVar1;
  }
  else {
    LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x324),0x147,0,0);
    if (LVar1 == 1) {
      DAT_0058387e = 0xf1;
    }
    else {
      LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x324),0x147,0,0);
      if (LVar1 == 2) {
        DAT_0058387e = 0xf0;
      }
    }
  }
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x398),0x147,0,0);
  if (LVar1 == 0) {
    LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x14c),0x400,0,0);
    DAT_00583886 = (char)LVar1;
  }
  else {
    LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x398),0x147,0,0);
    if (LVar1 == 1) {
      DAT_00583886 = 0xf1;
    }
    else {
      LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x398),0x147,0,0);
      if (LVar1 == 2) {
        DAT_00583886 = 0xf0;
      }
    }
  }
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x1c0),0x400,0,0);
  DAT_0058388e = (char)LVar1;
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x234),0x400,0,0);
  DAT_0058389e = (char)LVar1;
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x2a8),0x400,0,0);
  DAT_00583896 = (char)LVar1;
  return;
}


/* ============ FUN_004112b0 @ 004112b0  (req 004112b0) ============ */
/* calls: UpdateData@00419797 SendMessageW@EXTERNAL:00000183 FUN_00420cf9@00420cf9 EnableWindow@0041dd65 FUN_00402190@00402190 */
/* called by: FUN_00415450@00415450 FUN_00416360@00416360 */

void FUN_004112b0(void)

{
  int *piVar1;
  int iVar2;
  uint uVar3;
  undefined **ppuVar4;
  undefined4 *puVar5;
  CWnd *unaff_ESI;
  bool bVar6;
  HWND pHVar7;
  WPARAM WVar8;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c3a8;
  local_10 = ExceptionList;
  uVar3 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  ppuVar4 = FUN_00420cf9();
  if (ppuVar4 == (undefined **)0x0) {
                    /* WARNING: Subroutine does not return */
    FUN_00402190((undefined4 *)0x80004005);
  }
  puVar5 = (undefined4 *)(**(code **)(*ppuVar4 + 0xc))(uVar3);
  local_8 = 0;
  if (DAT_0058385d == '\x01') {
    SendMessageW(*(HWND *)(unaff_ESI + 0x480),0xf1,1,0);
    *(undefined4 *)(unaff_ESI + 0x78c) = 1;
  }
  else {
    SendMessageW(*(HWND *)(unaff_ESI + 0x480),0xf1,0,0);
    *(undefined4 *)(unaff_ESI + 0x78c) = 0;
  }
  bVar6 = (DAT_00583857 & 1) == 0;
  if (bVar6) {
    pHVar7 = *(HWND *)(unaff_ESI + 0x738);
  }
  else {
    pHVar7 = *(HWND *)(unaff_ESI + 0x738);
  }
  SendMessageW(pHVar7,0xf1,(uint)!bVar6,0);
  bVar6 = (DAT_00583857 & 0x10) == 0;
  if (bVar6) {
    pHVar7 = *(HWND *)(unaff_ESI + 0x7b0);
  }
  else {
    pHVar7 = *(HWND *)(unaff_ESI + 0x7b0);
  }
  SendMessageW(pHVar7,0xf1,(uint)!bVar6,0);
  if (DAT_0057ccd8 == '\x11') {
    SendMessageW(*(HWND *)(unaff_ESI + 0x40c),0x14e,0,0);
    CWnd::EnableWindow(unaff_ESI + 0x3ec,0);
  }
  else if (DAT_0057ccd8 == '\"') {
    CWnd::EnableWindow(unaff_ESI + 0x3ec,1);
    if (DAT_00583854 == '\x02') {
LAB_0041140f:
      pHVar7 = *(HWND *)(unaff_ESI + 0x40c);
      WVar8 = 2;
    }
    else if (DAT_00583854 == '\x04') {
      pHVar7 = *(HWND *)(unaff_ESI + 0x40c);
      WVar8 = 1;
    }
    else {
      if ((DAT_00583854 != '\b') && (DAT_0057ccd8 != '\x11')) {
        if (DAT_0057ccd8 != '\"') goto LAB_00411420;
        goto LAB_0041140f;
      }
      pHVar7 = *(HWND *)(unaff_ESI + 0x40c);
      WVar8 = 0;
    }
    SendMessageW(pHVar7,0x14e,WVar8,0);
  }
LAB_00411420:
  if (DAT_0058387e == (CWnd)0xf1) {
    pHVar7 = *(HWND *)(unaff_ESI + 0x324);
    WVar8 = 1;
LAB_00411463:
    SendMessageW(pHVar7,0x14e,WVar8,0);
    CWnd::EnableWindow(unaff_ESI + 0xb8,0);
    CWnd::EnableWindow(unaff_ESI + 0x4d4,0);
  }
  else {
    if (DAT_0058387e == (CWnd)0xf0) {
      pHVar7 = *(HWND *)(unaff_ESI + 0x324);
      WVar8 = 2;
      goto LAB_00411463;
    }
    SendMessageW(*(HWND *)(unaff_ESI + 0x324),0x14e,0,0);
    CWnd::EnableWindow(unaff_ESI + 0xb8,1);
    CWnd::EnableWindow(unaff_ESI + 0x4d4,1);
    if ((byte)DAT_0058387e < 0x1a) {
      SendMessageW(*(HWND *)(unaff_ESI + 0xd8),0x405,1,(uint)(byte)DAT_0058387e);
      unaff_ESI[0x2fc] = DAT_0058387e;
    }
    else {
      SendMessageW(*(HWND *)(unaff_ESI + 0xd8),0x405,1,8);
      unaff_ESI[0x2fc] = (CWnd)0x8;
    }
  }
  if (DAT_00583886 == (CWnd)0xf1) {
    pHVar7 = *(HWND *)(unaff_ESI + 0x398);
    WVar8 = 1;
  }
  else {
    if (DAT_00583886 != (CWnd)0xf0) {
      SendMessageW(*(HWND *)(unaff_ESI + 0x398),0x14e,0,0);
      CWnd::EnableWindow(unaff_ESI + 300,1);
      CWnd::EnableWindow(unaff_ESI + 0x548,1);
      if ((byte)DAT_00583886 < 0x1a) {
        SendMessageW(*(HWND *)(unaff_ESI + 0x14c),0x405,1,(uint)(byte)DAT_00583886);
        unaff_ESI[0x2fd] = DAT_00583886;
      }
      else {
        SendMessageW(*(HWND *)(unaff_ESI + 0x14c),0x405,1,8);
        unaff_ESI[0x2fd] = (CWnd)0x8;
      }
      goto LAB_004115a2;
    }
    pHVar7 = *(HWND *)(unaff_ESI + 0x398);
    WVar8 = 2;
  }
  SendMessageW(pHVar7,0x14e,WVar8,0);
  CWnd::EnableWindow(unaff_ESI + 300,0);
  CWnd::EnableWindow(unaff_ESI + 0x548,0);
LAB_004115a2:
  if ((byte)DAT_0058388e < 0x1a) {
    SendMessageW(*(HWND *)(unaff_ESI + 0x1c0),0x405,1,(uint)(byte)DAT_0058388e);
    unaff_ESI[0x2fe] = DAT_0058388e;
  }
  else {
    SendMessageW(*(HWND *)(unaff_ESI + 0x1c0),0x405,1,8);
    unaff_ESI[0x2fe] = (CWnd)0x8;
  }
  if ((byte)DAT_0058389e < 0x1a) {
    SendMessageW(*(HWND *)(unaff_ESI + 0x234),0x405,1,(uint)(byte)DAT_0058389e);
    unaff_ESI[0x2ff] = DAT_0058389e;
  }
  else {
    SendMessageW(*(HWND *)(unaff_ESI + 0x234),0x405,1,8);
    unaff_ESI[0x2ff] = (CWnd)0x8;
  }
  if ((byte)DAT_00583896 < 0x1a) {
    SendMessageW(*(HWND *)(unaff_ESI + 0x2a8),0x405,1,(uint)(byte)DAT_00583896);
    unaff_ESI[0x300] = DAT_00583896;
  }
  else {
    SendMessageW(*(HWND *)(unaff_ESI + 0x2a8),0x405,1,8);
    unaff_ESI[0x300] = DAT_00583896;
  }
  CWnd::UpdateData(unaff_ESI,0);
  local_8 = 0xffffffff;
  piVar1 = puVar5 + 3;
  LOCK();
  iVar2 = *piVar1;
  *piVar1 = *piVar1 + -1;
  UNLOCK();
  if (iVar2 == 1 || iVar2 + -1 < 0) {
    (**(code **)(*(int *)*puVar5 + 4))(puVar5);
  }
  ExceptionList = local_10;
  return;
}


/* ============ FUN_00409f90 @ 00409f90  (req 00409f90) ============ */
/* calls: UpdateData@00419797 EnableWindow@0041dd65 SendMessageW@EXTERNAL:00000183 */
/* called by: FUN_00416200@00416200 */

void FUN_00409f90(void)

{
  LRESULT LVar1;
  CWnd *unaff_ESI;
  
  CWnd::UpdateData(unaff_ESI,1);
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x150),0xf0,0,0);
  if (LVar1 == 0) {
    DAT_00583855 = (CWnd)((byte)unaff_ESI[0xb8] | 0x80);
  }
  else {
    DAT_00583855 = unaff_ESI[0xb8];
  }
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x2ac),0xf0,0,0);
  if (LVar1 == 0) {
    DAT_00583856 = (CWnd)((byte)unaff_ESI[0x300] | 0x80);
  }
  else {
    DAT_00583856 = unaff_ESI[0x300];
  }
  if (*(int *)(unaff_ESI + 0xb8) == *(int *)(unaff_ESI + 0x300)) {
    CWnd::EnableWindow(unaff_ESI + 0xbc,0);
    SendMessageW(*(HWND *)(unaff_ESI + 0x150),0xf1,0,0);
  }
  return;
}



/* no function at 4110 */


/* ============ FUN_0040a040 @ 0040a040  (req 0040a040) ============ */
/* calls: UpdateData@00419797 SendMessageW@EXTERNAL:00000183 EnableWindow@0041dd65 */
/* called by: FUN_00415450@00415450 FUN_00416360@00416360 */

void FUN_0040a040(void)

{
  CWnd *unaff_ESI;
  code *pcVar1;
  
  pcVar1 = SendMessageW_exref;
  if ((char)DAT_00583855 < '\0') {
    *(uint *)(unaff_ESI + 0xb8) = DAT_00583855 & 0x7f;
    CWnd::EnableWindow(unaff_ESI + 0xbc,0);
    pcVar1 = SendMessageW_exref;
    SendMessageW(*(HWND *)(unaff_ESI + 0x150),0xf1,0,0);
  }
  else {
    SendMessageW(*(HWND *)(unaff_ESI + 0x150),0xf1,1,0);
    *(uint *)(unaff_ESI + 0xb8) = (uint)DAT_00583855;
  }
  if ((char)DAT_00583856 < '\0') {
    *(uint *)(unaff_ESI + 0x300) = DAT_00583856 & 0x7f;
    CWnd::EnableWindow(unaff_ESI + 0x218,0);
    (*pcVar1)(*(undefined4 *)(unaff_ESI + 0x2ac),0xf1,0,0);
  }
  else {
    (*pcVar1)(*(undefined4 *)(unaff_ESI + 0x2ac),0xf1,1);
    *(uint *)(unaff_ESI + 0x300) = (uint)DAT_00583856;
  }
  if (*(int *)(unaff_ESI + 0xb8) == *(int *)(unaff_ESI + 0x300)) {
    CWnd::EnableWindow(unaff_ESI + 0xbc,0);
    (*pcVar1)(*(undefined4 *)(unaff_ESI + 0x150),0xf1,0,0);
    CWnd::UpdateData(unaff_ESI,0);
    return;
  }
  CWnd::EnableWindow(unaff_ESI + 0xbc,1);
  CWnd::UpdateData(unaff_ESI,0);
  return;
}


/* ============ FUN_004097e0 @ 004097e0  (req 004097e0) ============ */
/* calls: FUN_00409360@00409360 SetWindowTextW@0041dc58 SendMessageW@EXTERNAL:00000183 */
/* called by: FUN_00415450@00415450 FUN_00416360@00416360 */

void FUN_004097e0(void)

{
  undefined *this;
  void *unaff_ESI;
  
  if ((DAT_00583880 == '\0') && (DAT_00583881 == '\x01')) {
    SendMessageW(*(HWND *)((int)unaff_ESI + 0x43c),0xf1,1,0);
    *(undefined4 *)((int)unaff_ESI + 0x380) = 0;
    CWnd::SetWindowTextW((CWnd *)((int)unaff_ESI + 0x3a4),L"Left Button");
    this = &DAT_00583878;
  }
  else {
    SendMessageW(*(HWND *)((int)unaff_ESI + 0x43c),0xf1,0,0);
    *(undefined4 *)((int)unaff_ESI + 0x380) = 1;
    CWnd::SetWindowTextW((CWnd *)((int)unaff_ESI + 0x3a4),L"Right Button");
    this = &DAT_00583880;
  }
  FUN_00409360(this,unaff_ESI);
  FUN_00409360(&DAT_00583888,unaff_ESI);
  FUN_00409360(&DAT_00583898,unaff_ESI);
  FUN_00409360(&DAT_00583890,unaff_ESI);
  FUN_00409360(&DAT_005838a8,unaff_ESI);
  FUN_00409360(&DAT_005838b0,unaff_ESI);
  return;
}


/* ============ FUN_00416470 @ 00416470  (req 00416470) ============ */
/* calls: FUN_00411d70@00411d70 FUN_0040fed0@0040fed0 EnableWindow@0041dd65 */
/* called by: FUN_004147b0@004147b0 FUN_004149a0@004149a0 FUN_00415500@00415500 FUN_00415b70@00415b70 */

void FUN_00416470(void)

{
  int in_EAX;
  int unaff_EBX;
  
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x2b84),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x2bf8),in_EAX);
  FUN_00411d70();
  FUN_0040fed0();
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x22c8),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x233c),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x23b0),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x2424),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x2498),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x250c),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x2580),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x27e0),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x2854),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x293c),in_EAX);
  CWnd::EnableWindow((CWnd *)(unaff_EBX + 0x29b0),in_EAX);
  return;
}


/* ============ FUN_004166f0 @ 004166f0  (req 004166f0) ============ */
/* calls: SendMessageW@EXTERNAL:00000183 ShowWindow@0041dd23 */
/* called by: FUN_004168d0@004168d0 */

void FUN_004166f0(void)

{
  LRESULT LVar1;
  int unaff_ESI;
  
  LVar1 = SendMessageW(*(HWND *)(unaff_ESI + 0x2abc),0x130b,0,0);
  if (LVar1 == 4) {
    SendMessageW(*(HWND *)(unaff_ESI + 0x2abc),0x130c,0,0);
    CWnd::ShowWindow((CWnd *)(unaff_ESI + 0x1eac),0);
    CWnd::ShowWindow((CWnd *)(unaff_ESI + 0x16a8),0);
    CWnd::ShowWindow((CWnd *)(unaff_ESI + 0x25f4),0);
    CWnd::ShowWindow((CWnd *)(unaff_ESI + 0x1394),0);
    CWnd::ShowWindow((CWnd *)(unaff_ESI + 0x2724),0);
    CWnd::ShowWindow((CWnd *)(unaff_ESI + 200),5);
  }
  return;
}


/* ---- RangeXrefs.java over the settings struct 0x00583850 + 0x30 ----

base=00583850 len=0x30
+0x00  00583850  [FUN_00403d90@00403e83:DATA, FUN_004147b0@004148b0:DATA, FUN_00416200@00416266:DATA, FUN_00416200@0041630c:DATA, FUN_00416200@00416319:DATA, FUN_00416360@00416404:DATA, FUN_00416590@004165d9:DATA]
+0x04  00583854  [FUN_004112b0@004113d5:READ, FUN_004110e0@00411171:WRITE, FUN_004110e0@0041117a:WRITE, FUN_004110e0@00411183:WRITE, FUN_00415b70@00415db0:WRITE, FUN_00415b70@00415ddf:WRITE, FUN_00415b70@00415e11:WRITE]
+0x05  00583855  [FUN_0040a140@0040a202:WRITE, FUN_0040a140@0040a212:WRITE, FUN_0040a040@0040a040:READ, FUN_0040a040@0040a093:READ, FUN_00409f90@00409fbc:WRITE, FUN_00409f90@00409fcd:WRITE]
+0x06  00583856  [FUN_0040a500@0040a5c2:WRITE, FUN_0040a500@0040a5d2:WRITE, FUN_0040a040@0040a0a0:READ, FUN_0040a040@0040a0e6:READ, FUN_00409f90@00409fef:WRITE, FUN_00409f90@0040a000:WRITE]
+0x07  00583857  [FUN_004112b0@00411344:READ, FUN_004112b0@0041136d:READ, FUN_00411e40@00411e7f:WRITE, FUN_00411e40@00411e9c:WRITE, FUN_00411eb0@00411eef:WRITE, FUN_00411eb0@00411f0c:WRITE, FUN_004110e0@00411155:WRITE]
+0x08  00583858  [FUN_00416200@004162ff:DATA]
+0x09  00583859  [FUN_0040fa10@0040fadb:READ, FUN_0040fa10@0040feaf:READ, FUN_0040f820@0040f8d9:WRITE, FUN_0040f820@0040f8f9:WRITE, FUN_0040f820@0040f919:WRITE, FUN_0040f820@0040f939:WRITE]
+0x0B  0058385b  [FUN_0040fa10@0040fa85:READ, FUN_0040f820@0040f888:WRITE]
+0x0C  0058385c  [FUN_0040fa10@0040fab0:READ, FUN_0040f820@0040f8c1:WRITE]
+0x0D  0058385d  [FUN_004112b0@00411303:READ, FUN_004110e0@0041110c:WRITE]
+0x0E  0058385e  [FUN_0040fa10@0040fa10:READ, FUN_0040f820@0040f8a4:WRITE]
+0x0F  0058385f  [FUN_0040fa10@0040fa46:READ, FUN_0040f820@0040f846:WRITE, FUN_0040f820@0040f868:WRITE]
+0x10  00583860  [FUN_0040fa10@0040fb0c:READ, FUN_0040f820@0040f96b:WRITE]
+0x12  00583862  [FUN_0040d350@0040d3b3:WRITE, FUN_0040d350@0040d5f7:WRITE, FUN_0040fa10@0040fb1b:READ, FUN_0040f820@0040f947:WRITE]
+0x14  00583864  [FUN_0040d350@0040d3ef:WRITE, FUN_0040d350@0040d605:WRITE, FUN_0040fa10@0040fb28:READ, FUN_0040f820@0040f954:WRITE]
+0x16  00583866  [FUN_0040f820@0040f99b:WRITE]
+0x18  00583868  [FUN_0040d350@0040d428:WRITE, FUN_0040d350@0040d6c2:WRITE, FUN_0040fa10@0040fc1f:READ, FUN_0040f820@0040f977:WRITE]
+0x1A  0058386a  [FUN_0040d350@0040d45b:WRITE, FUN_0040d350@0040d6d0:WRITE, FUN_0040fa10@0040fc2c:READ, FUN_0040f820@0040f985:WRITE]
+0x1C  0058386c  [FUN_0040f820@0040f9cb:WRITE]
+0x1E  0058386e  [FUN_0040d350@0040d48f:WRITE, FUN_0040d350@0040d775:WRITE, FUN_0040fa10@0040fcfb:READ, FUN_0040f820@0040f9a8:WRITE]
+0x20  00583870  [FUN_0040d350@0040d4c0:WRITE, FUN_0040d350@0040d783:WRITE, FUN_0040fa10@0040fd08:READ, FUN_0040f820@0040f9b6:WRITE]
+0x22  00583872  [FUN_0040f820@0040f9fb:WRITE]
+0x24  00583874  [FUN_0040d350@0040d4f3:WRITE, FUN_0040d350@0040d828:WRITE, FUN_0040fa10@0040fdd3:READ, FUN_0040f820@0040f9d8:WRITE]
+0x26  00583876  [FUN_0040d350@0040d836:WRITE, FUN_0040fa10@0040fde0:READ, FUN_0040f820@0040f9e5:WRITE]
+0x28  00583878  [FUN_004070f0@00408801:DATA, FUN_004070f0@00407c0d:DATA, FUN_004070f0@00407cd7:DATA, FUN_004070f0@00407b75:DATA, FUN_004070f0@004086a9:DATA, FUN_004070f0@00408551:DATA, FUN_004070f0@0040840c:DATA, FUN_004070f0@004082c7:DATA, FUN_004070f0@00408182:DATA, FUN_004070f0@0040803d:DATA, FUN_004070f0@00407f2d:DATA, FUN_004070f0@00407e02:DATA, FUN_004070f0@00407972:DATA, FUN_004070f0@004077dc:DATA, FUN_004070f0@00407828:DATA, FUN_004070f0@004076a6:DATA, FUN_004070f0@00407560:DATA, FUN_004070f0@0040741a:DATA, FUN_004070f0@004072d4:DATA, FUN_004070f0@0040718e:DATA, FUN_004097e0@00409823:DATA]
+0x29  00583879  [FUN_004070f0@00408915:DATA, FUN_004070f0@004088d5:DATA, FUN_004070f0@00408852:DATA, FUN_004070f0@0040880f:DATA, FUN_004070f0@00407c1a:DATA, FUN_004070f0@00407db5:DATA, FUN_004070f0@00407d96:DATA, FUN_004070f0@00407d2e:DATA, FUN_004070f0@00407ce5:DATA, FUN_004070f0@00407b88:DATA, FUN_004070f0@004087d4:DATA, FUN_004070f0@00408788:DATA, FUN_004070f0@004086fe:DATA, FUN_004070f0@004086b7:DATA, FUN_004070f0@0040867c:DATA, FUN_004070f0@00408630:DATA, FUN_004070f0@004085a6:DATA, FUN_004070f0@0040855f:DATA, FUN_004070f0@00408529:DATA, FUN_004070f0@004084e5:DATA, FUN_004070f0@0040845e:DATA, FUN_004070f0@0040841a:DATA, FUN_004070f0@004083e4:DATA, FUN_004070f0@004083a0:DATA, FUN_004070f0@00408319:DATA, FUN_004070f0@004082d5:DATA, FUN_004070f0@0040829f:DATA, FUN_004070f0@0040825b:DATA, FUN_004070f0@004081d4:DATA, FUN_004070f0@00408190:DATA, FUN_004070f0@0040815a:DATA, FUN_004070f0@00408116:DATA, FUN_004070f0@0040808f:DATA, FUN_004070f0@0040804b:DATA, FUN_004070f0@00408015:DATA, FUN_004070f0@00407fd1:DATA, FUN_004070f0@00407f3b:DATA, FUN_004070f0@00407ee0:DATA, FUN_004070f0@00407ec1:DATA, FUN_004070f0@00407e59:DATA, FUN_004070f0@00407e10:DATA, FUN_004070f0@00407a6f:DATA, FUN_004070f0@00407a2b:DATA, FUN_004070f0@004079a4:DATA, FUN_004070f0@00407985:DATA, FUN_004070f0@004077ea:DATA, FUN_004070f0@00407925:DATA, FUN_004070f0@004078e1:DATA, FUN_004070f0@0040785a:DATA, FUN_004070f0@0040783b:DATA, FUN_004070f0@0040779f:DATA, FUN_004070f0@0040775c:DATA, FUN_004070f0@004076d6:DATA, FUN_004070f0@004076b8:DATA, FUN_004070f0@00407659:DATA, FUN_004070f0@00407616:DATA, FUN_004070f0@00407590:DATA, FUN_004070f0@00407572:DATA, FUN_004070f0@00407513:DATA, FUN_004070f0@004074d0:DATA, FUN_004070f0@0040744a:DATA, FUN_004070f0@0040742c:DATA, FUN_004070f0@004073cd:DATA, FUN_004070f0@0040738a:DATA, FUN_004070f0@00407304:DATA, FUN_004070f0@004072e6:DATA, FUN_004070f0@00407287:DATA, FUN_004070f0@00407244:DATA, FUN_004070f0@004071be:DATA, FUN_004070f0@004071a0:DATA]
+0x2A  0058387a  [FUN_004070f0@0040892e:DATA, FUN_004070f0@004088e4:DATA, FUN_004070f0@00408861:DATA, FUN_004070f0@0040881e:DATA, FUN_004070f0@00407c2d:DATA, FUN_004070f0@00407dc5:DATA, FUN_004070f0@00407cf5:DATA, FUN_004070f0@00407b9a:DATA, FUN_004070f0@0040879b:DATA, FUN_004070f0@00408711:DATA, FUN_004070f0@004086ca:DATA, FUN_004070f0@00408643:DATA, FUN_004070f0@004085b9:DATA, FUN_004070f0@00408572:DATA, FUN_004070f0@004084f5:DATA, FUN_004070f0@0040846e:DATA, FUN_004070f0@0040842a:DATA, FUN_004070f0@004083b0:DATA, FUN_004070f0@00408329:DATA, FUN_004070f0@004082e5:DATA, FUN_004070f0@0040826b:DATA, FUN_004070f0@004081e4:DATA, FUN_004070f0@004081a0:DATA, FUN_004070f0@00408126:DATA, FUN_004070f0@0040809f:DATA, FUN_004070f0@0040805b:DATA, FUN_004070f0@00407fe1:DATA, FUN_004070f0@00407f4b:DATA, FUN_004070f0@00407ef0:DATA, FUN_004070f0@00407e20:DATA, FUN_004070f0@00407a7f:DATA, FUN_004070f0@00407a3b:DATA, FUN_004070f0@004079b4:DATA, FUN_004070f0@004077fa:DATA, FUN_004070f0@00407935:DATA, FUN_004070f0@004078f1:DATA, FUN_004070f0@0040786a:DATA, FUN_004070f0@004077af:DATA, FUN_004070f0@0040776c:DATA, FUN_004070f0@004076e6:DATA, FUN_004070f0@00407669:DATA, FUN_004070f0@00407626:DATA, FUN_004070f0@004075a0:DATA, FUN_004070f0@00407523:DATA, FUN_004070f0@004074e0:DATA, FUN_004070f0@0040745a:DATA, FUN_004070f0@004073dd:DATA, FUN_004070f0@0040739a:DATA, FUN_004070f0@00407314:DATA, FUN_004070f0@00407297:DATA, FUN_004070f0@00407254:DATA, FUN_004070f0@004071ce:DATA]
+0x2C  0058387c  [FUN_004070f0@0040893e:DATA, FUN_004070f0@004088f9:DATA, FUN_004070f0@00408876:DATA, FUN_004070f0@00408833:DATA, FUN_004070f0@00407c41:DATA, FUN_004070f0@00407dda:DATA, FUN_004070f0@00407d0a:DATA, FUN_004070f0@00407baa:DATA, FUN_004070f0@004087b0:DATA, FUN_004070f0@00408726:DATA, FUN_004070f0@004086df:DATA, FUN_004070f0@00408658:DATA, FUN_004070f0@004085ce:DATA, FUN_004070f0@00408587:DATA, FUN_004070f0@0040850a:DATA, FUN_004070f0@00408483:DATA, FUN_004070f0@0040843f:DATA, FUN_004070f0@004083c5:DATA, FUN_004070f0@0040833e:DATA, FUN_004070f0@004082fa:DATA, FUN_004070f0@00408280:DATA, FUN_004070f0@004081f9:DATA, FUN_004070f0@004081b5:DATA, FUN_004070f0@0040813b:DATA, FUN_004070f0@004080b4:DATA, FUN_004070f0@00408070:DATA, FUN_004070f0@00407ff6:DATA, FUN_004070f0@00407f60:DATA, FUN_004070f0@00407f05:DATA, FUN_004070f0@00407e35:DATA, FUN_004070f0@00407a94:DATA, FUN_004070f0@00407a50:DATA, FUN_004070f0@004079c9:DATA, FUN_004070f0@0040780a:DATA, FUN_004070f0@0040794a:DATA, FUN_004070f0@00407906:DATA, FUN_004070f0@0040787f:DATA, FUN_004070f0@004077c4:DATA, FUN_004070f0@00407781:DATA, FUN_004070f0@004076fb:DATA, FUN_004070f0@0040767e:DATA, FUN_004070f0@0040763b:DATA, FUN_004070f0@004075b5:DATA, FUN_004070f0@00407538:DATA, FUN_004070f0@004074f5:DATA, FUN_004070f0@0040746f:DATA, FUN_004070f0@004073f2:DATA, FUN_004070f0@004073af:DATA, FUN_004070f0@00407329:DATA, FUN_004070f0@004072ac:DATA, FUN_004070f0@00407269:DATA, FUN_004070f0@004071e3:DATA]
+0x2E  0058387e  [FUN_004112b0@00411420:READ, FUN_004112b0@004114ab:READ, FUN_004112b0@004114c8:READ, FUN_00411f20@00411f59:WRITE, FUN_00412040@004120c9:WRITE, FUN_004110e0@004111b4:WRITE, FUN_004110e0@004111ce:WRITE, FUN_004110e0@004111ed:WRITE]

*/


/* ---- RangeXrefs.java over the shadow/baseline copy 0x00584320 + 0x30 ----

base=00584320 len=0x30
+0x00  00584320  [FUN_00403ec0@00403ec3:WRITE, FUN_004147b0@004148ba:DATA, FUN_004149a0@00414ca0:DATA, FUN_004149a0@00414e99:DATA, FUN_00416360@0041640e:DATA, FUN_00416590@004165e9:DATA]
+0x04  00584324  [FUN_00403ec0@00403ee1:WRITE, FUN_00415b70@00415db5:WRITE, FUN_00415b70@00415de4:WRITE, FUN_00415b70@00415e0b:WRITE]
+0x05  00584325  [FUN_00403ec0@00403ed7:WRITE]
+0x06  00584326  [FUN_00403ec0@00403ecd:WRITE]
+0x07  00584327  [FUN_00403ec0@00403eeb:WRITE]
+0x08  00584328  [FUN_00403ec0@00403f31:WRITE]
+0x09  00584329  [FUN_00403ec0@00403f3b:WRITE]
+0x0A  0058432a  [FUN_00403ec0@00403ef5:WRITE]
+0x0B  0058432b  [FUN_00403ec0@00403f13:WRITE]
+0x0C  0058432c  [FUN_00403ec0@00403f1d:WRITE]
+0x0D  0058432d  [FUN_00403ec0@00403f27:WRITE]
+0x0E  0058432e  [FUN_00403ec0@00403eff:WRITE]
+0x0F  0058432f  [FUN_00403ec0@00403f09:WRITE]
+0x10  00584330  [FUN_00403ec0@0040400d:WRITE]
+0x12  00584332  [FUN_00403ec0@00404017:WRITE]
+0x14  00584334  [FUN_00403ec0@00404022:WRITE]
+0x16  00584336  [FUN_00403ec0@0040402d:WRITE]
+0x18  00584338  [FUN_00403ec0@00404037:WRITE]
+0x1A  0058433a  [FUN_00403ec0@00404042:WRITE]
+0x1C  0058433c  [FUN_00403ec0@0040404d:WRITE]
+0x1E  0058433e  [FUN_00403ec0@00404057:WRITE]
+0x20  00584340  [FUN_00403ec0@00404062:WRITE]
+0x22  00584342  [FUN_00403ec0@0040406d:WRITE]
+0x24  00584344  [FUN_00403ec0@00404077:WRITE]
+0x26  00584346  [FUN_00403ec0@00404082:WRITE]
+0x28  00584348  [FUN_00403ec0@0040408d:WRITE]
+0x29  00584349  [FUN_00403ec0@00404097:WRITE]
+0x2A  0058434a  [FUN_00403ec0@004040a1:WRITE]
+0x2C  0058434c  [FUN_00403ec0@004040ac:WRITE]
+0x2E  0058434e  [FUN_00403ec0@004040b7:WRITE]

*/


/* ---- FindRefs.java on the two struct bases and the state globals ----

=== refs to 583850 ===
  00403e83  FUN_00403d90   @ 00403d90  [DATA]  MOV EAX,0x583850
  004148b0  FUN_004147b0   @ 004147b0  [DATA]  MOV EAX,0x583850
  00416266  FUN_00416200   @ 00416200  [DATA]  MOV EAX,0x583850
  0041630c  FUN_00416200   @ 00416200  [DATA]  MOV ECX,0x583850
  00416319  FUN_00416200   @ 00416200  [DATA]  MOV ECX,0x583850
  00416404  FUN_00416360   @ 00416360  [DATA]  MOV EAX,0x583850
  004165d9  FUN_00416590   @ 00416590  [DATA]  MOV ECX,0x583850
  -- distinct functions: [FUN_00403d90 @ 00403d90, FUN_004147b0 @ 004147b0, FUN_00416200 @ 00416200, FUN_00416360 @ 00416360, FUN_00416590 @ 00416590]
=== refs to 583858 ===
  004162ff  FUN_00416200   @ 00416200  [DATA]  MOV ECX,0x583858
  -- distinct functions: [FUN_00416200 @ 00416200]
=== refs to 583443 ===
  0041434d  FUN_004142b0   @ 004142b0  [WRITE]  MOV byte ptr [0x00583443],0x1
  00414365  FUN_004142b0   @ 004142b0  [WRITE]  MOV [0x00583443],AL
  00414403  FUN_00414400   @ 00414400  [READ]  CMP byte ptr [0x00583443],0x0
  00415736  FUN_00415500   @ 00415500  [WRITE]  MOV byte ptr [0x00583443],BL
  -- distinct functions: [FUN_004142b0 @ 004142b0, FUN_00414400 @ 00414400, FUN_00415500 @ 00415500]
=== refs to 583442 ===
  00409aaf  FUN_00409a70   @ 00409a70  [WRITE]  MOV byte ptr [0x00583442],0x1
  00416289  FUN_00416200   @ 00416200  [READ]  CMP byte ptr [0x00583442],0x1
  004162a0  FUN_00416200   @ 00416200  [WRITE]  MOV byte ptr [0x00583442],0x0
  004163a0  FUN_00416360   @ 00416360  [READ]  CMP byte ptr [0x00583442],0x1
  004163b7  FUN_00416360   @ 00416360  [WRITE]  MOV byte ptr [0x00583442],0x0
  -- distinct functions: [FUN_00409a70 @ 00409a70, FUN_00416200 @ 00416200, FUN_00416360 @ 00416360]
=== refs to 0057ccd8 ===
  00409a8e  FUN_00409a70   @ 00409a70  [READ]  MOV CL,byte ptr [0x0057ccd8]
  00409b94  FUN_00409b70   @ 00409b70  [READ]  MOV CL,byte ptr [0x0057ccd8]
  00411396  FUN_004112b0   @ 004112b0  [READ]  MOV CL,byte ptr [0x0057ccd8]
  004113bf  FUN_004112b0   @ 004112b0  [READ]  MOV AL,[0x0057ccd8]
  004113eb  FUN_004112b0   @ 004112b0  [READ]  MOV CL,byte ptr [0x0057ccd8]
  00411406  FUN_004112b0   @ 004112b0  [READ]  MOV AL,[0x0057ccd8]
  0041465a  FUN_00414430   @ 00414430  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  004142c6  FUN_004142b0   @ 004142b0  [READ]  MOV CL,byte ptr [0x0057ccd8]
  004142eb  FUN_004142b0   @ 004142b0  [READ]  MOV CL,byte ptr [0x0057ccd8]
  00414315  FUN_004142b0   @ 004142b0  [READ]  MOV CL,byte ptr [0x0057ccd8]
  00414b39  FUN_004149a0   @ 004149a0  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  00414d08  FUN_004149a0   @ 004149a0  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  00414d40  FUN_004149a0   @ 004149a0  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  00414d5f  FUN_004149a0   @ 004149a0  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  00414fa1  FUN_004149a0   @ 004149a0  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  00414fc1  FUN_004149a0   @ 004149a0  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  00416540  FUN_00416530   @ 00416530  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  00416553  FUN_00416530   @ 00416530  [WRITE]  MOV byte ptr [0x0057ccd8],0x11
  0041655a  FUN_00416530   @ 00416530  [READ]  MOV AL,[0x0057ccd8]
  00416583  FUN_00416530   @ 00416530  [WRITE]  MOV byte ptr [0x0057ccd8],0x22
  0041658a  FUN_00416530   @ 00416530  [READ]  MOV AL,[0x0057ccd8]
  004155dc  FUN_00415500   @ 00415500  [READ]  MOV AL,[0x0057ccd8]
  00415676  FUN_00415500   @ 00415500  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  00415719  FUN_00415500   @ 00415500  [READ]  MOV CL,byte ptr [0x0057ccd8]
  00415777  FUN_00415500   @ 00415500  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  0041584b  FUN_00415500   @ 00415500  [READ]  MOV AL,[0x0057ccd8]
  00415858  FUN_00415500   @ 00415500  [READ]  MOV CL,byte ptr [0x0057ccd8]
  00415875  FUN_00415500   @ 00415500  [WRITE]  MOV byte ptr [0x0057ccd8],0x11
  004158b3  FUN_00415500   @ 00415500  [READ]  MOV CL,byte ptr [0x0057ccd8]
  004158c5  FUN_00415500   @ 00415500  [READ]  MOV DL,byte ptr [0x0057ccd8]
  004158e4  FUN_00415500   @ 00415500  [WRITE]  MOV byte ptr [0x0057ccd8],0x11
  004159cb  FUN_00415500   @ 00415500  [READ]  MOV DL,byte ptr [0x0057ccd8]
  00415a01  FUN_00415500   @ 00415500  [WRITE]  MOV byte ptr [0x0057ccd8],0x22
  00415a1b  FUN_00415500   @ 00415500  [READ]  MOV DL,byte ptr [0x0057ccd8]
  00416270  FUN_00416200   @ 00416200  [READ]  MOV AL,[0x0057ccd8]
  0041627e  FUN_00416200   @ 00416200  [READ]  MOV CL,byte ptr [0x0057ccd8]
  004162a9  FUN_00416200   @ 00416200  [READ]  MOV DL,byte ptr [0x0057ccd8]
  004162d3  FUN_00416200   @ 00416200  [READ]  MOV AL,[0x0057ccd8]
  00416390  FUN_00416360   @ 00416360  [READ]  MOV DL,byte ptr [0x0057ccd8]
  004163c0  FUN_00416360   @ 00416360  [READ]  MOV AL,[0x0057ccd8]
  004163dc  FUN_00416360   @ 00416360  [READ]  MOV CL,byte ptr [0x0057ccd8]
  004163ec  FUN_00416360   @ 00416360  [READ]  MOV DL,byte ptr [0x0057ccd8]
  00416796  FUN_00416790   @ 00416790  [READ]  MOV AL,[0x0057ccd8]
  004167a9  FUN_00416790   @ 00416790  [READ]  MOV CL,byte ptr [0x0057ccd8]
  004168d6  FUN_004168d0   @ 004168d0  [READ]  MOV AL,[0x0057ccd8]
  0041696b  FUN_004168d0   @ 004168d0  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  004169c6  FUN_004168d0   @ 004168d0  [WRITE]  MOV byte ptr [0x0057ccd8],0x22
  00415c1f  FUN_00415b70   @ 00415b70  [READ]  MOV CL,byte ptr [0x0057ccd8]
  00415c66  FUN_00415b70   @ 00415b70  [READ]  MOV DL,byte ptr [0x0057ccd8]
  00415cc8  FUN_00415b70   @ 00415b70  [READ]  MOV DL,byte ptr [0x0057ccd8]
  00415ce3  FUN_00415b70   @ 00415b70  [READ]  MOV AL,[0x0057ccd8]
  00415d14  FUN_00415b70   @ 00415b70  [WRITE]  MOV byte ptr [0x0057ccd8],0xff
  00415dbc  FUN_00415b70   @ 00415b70  [READ]  MOV DL,byte ptr [0x0057ccd8]
  00415deb  FUN_00415b70   @ 00415b70  [READ]  MOV CL,byte ptr [0x0057ccd8]
  -- distinct functions: [FUN_00409a70 @ 00409a70, FUN_00409b70 @ 00409b70, FUN_004112b0 @ 004112b0, FUN_00414430 @ 00414430, FUN_004142b0 @ 004142b0, FUN_004149a0 @ 004149a0, FUN_00416530 @ 00416530, FUN_00415500 @ 00415500, FUN_00416200 @ 00416200, FUN_00416360 @ 00416360, FUN_00416790 @ 00416790, FUN_004168d0 @ 004168d0, FUN_00415b70 @ 00415b70]

*/


/* ---- callers of FUN_0040d350 (Callers.java) ----

--- callers of 0040d350 (FUN_0040d350) ---
    total new: 0

   Nothing CALLS FUN_0040d350 -- it is an MFC message-map handler, reached only
   through the dialog's message map, so Ghidra sees no call site. It is the
   EN_CHANGE / spin handler for the eight CPI edit boxes.
*/


