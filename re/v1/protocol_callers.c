/* v1/protocol_callers.c -- every command function of the v1 configuration tool.
 *
 * Program: Endgame_Gear_OP1w_4k_Configuration_Tool_v1.03.exe
 *   (OP1w 4k **v1** vendor tool; Ghidra project OP1w, image base 0x00400000)
 * Extracted with Ghidra 12.1.4 headless, scripts in git/re/ghidra_scripts.
 * Evidence class for everything in this file: [BIN].
 *
 * Each function builds a 64-byte report 0xA1, hands it to FUN_004037c0
 * (see v1/hid_decomp.c), sleeps, then reads the answer back with FUN_00403890.
 * The four header bytes appear in the decompilation as one little-endian
 * dword store, so 0x1c0f14a1 reads A1 14 0F 1C on the wire.
 *
 * Address           header          command
 *   FUN_00405340    A1 0D 00 00     dongle info
 *   FUN_00405200    A1 0E 00 00     mouse info (VID/PID/firmware)
 *   FUN_00405480    A1 0F 01 00     probe, target 0x01
 *   FUN_004055a0    A1 0F 0F 00     probe, target 0x0F
 *   FUN_00403d90    A1 12 00 00     read 1024-byte config blob via report 0xA0
 *   FUN_004056c0    A1 13 00 00     factory reset
 *   FUN_00404750    A1 14 0F 1C     sensor & CPI block   (28 B payload)
 *   FUN_00404bb0    A1 15 0F 0A     polling/power/filters (TEN payload bytes)
 *   FUN_00404890    A1 16 0F 1C     button table (2 chunks)
 *   FUN_004050c0    A1 B4 00 00     battery / signal / target
 *   FUN_00403a50    A1 70 00 00     pair dongle
 *   FUN_00403b50    A1 71 00 00     "pair default"
 *   FUN_00403c50    A1 72 00 00     "get pair data"
 *
 * This is the same command set and the same header bytes as the v2 tool.
 *
 * NOTE on cmd 0x15: FUN_00404bb0 writes exactly TEN payload bytes and there is
 * no eleventh store anywhere in the function -- the v1 tool has no sensor-glass
 * byte. It gathers them from the settings struct at 0x00583850:
 *   payload +0 <- +0x0D   motion sync
 *   payload +1 <- +0x04   polling / power mode
 *   payload +2 <- +0x07   flags
 *   payload +3 <- +0x05   power saving
 *   payload +4 <- +0x2E   left      multiclick filter
 *   payload +5 <- +0x36   right
 *   payload +6 <- +0x3E   middle
 *   payload +7 <- +0x46   back
 *   payload +8 <- +0x4E   forward
 *   payload +9 <- +0x06   deep sleep
 * Stride 8 from +0x2E. The v2 equivalent (FUN_00404b80) uses +0x0F, +0x04,
 * +0x07, +0x05, then +0x34/+0x3C/+0x44/+0x4C/+0x54, +0x06, +0x10 -- same shape,
 * shifted by 6, plus the eleventh byte.
 */

/* ============ FUN_00405340 @ 00405340  (req 405340) ============ */
/* calls: LeaveCriticalSection@EXTERNAL:00000026 Sleep@EXTERNAL:00000023 FUN_004037c0@004037c0 FUN_00403890@00403890 __security_check_cookie@0050a225 _memset@0050b7b0 EnterCriticalSection@EXTERNAL:00000025 */
/* called by: FUN_004149a0@004149a0 FUN_00415500@00415500 */

void __cdecl FUN_00405340(undefined4 *param_1)

{
  char cVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined4 local_98;
  undefined2 local_94;
  undefined1 local_58;
  char local_57 [15];
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined2 local_3c;
  uint local_18;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c30b;
  local_10 = ExceptionList;
  local_18 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  local_58 = 0;
  _memset(local_57,0,0x3f);
  _memset(&local_98,0,0x40);
  local_98 = 0xda1;
  local_94 = 0;
  cVar1 = FUN_004037c0();
  Sleep(0x96);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  else {
    _memset(&local_58,0,0x40);
    local_58 = 0xa1;
    cVar1 = FUN_00403890();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_57[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
    else {
      *param_1 = local_48;
      param_1[1] = local_44;
      param_1[2] = local_40;
      *(undefined2 *)(param_1 + 3) = local_3c;
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00405200 @ 00405200  (req 405200) ============ */
/* calls: LeaveCriticalSection@EXTERNAL:00000026 Sleep@EXTERNAL:00000023 FUN_004037c0@004037c0 FUN_00403890@00403890 __security_check_cookie@0050a225 _memset@0050b7b0 EnterCriticalSection@EXTERNAL:00000025 */
/* called by: FUN_004149a0@004149a0 FUN_004168d0@004168d0 */

void __cdecl FUN_00405200(undefined4 *param_1,ushort param_2)

{
  char cVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined4 local_94;
  undefined2 local_90;
  undefined1 local_54;
  char local_53 [15];
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined2 local_38;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c18b;
  local_10 = ExceptionList;
  local_14 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  local_54 = 0;
  _memset(local_53,0,0x3f);
  _memset(&local_94,0,0x40);
  local_94 = 0xea1;
  local_90 = 0;
  cVar1 = FUN_004037c0();
  Sleep((uint)param_2);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  else {
    _memset(&local_54,0,0x40);
    local_54 = 0xa1;
    cVar1 = FUN_00403890();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_53[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
    else {
      *param_1 = local_44;
      param_1[1] = local_40;
      param_1[2] = local_3c;
      *(undefined2 *)(param_1 + 3) = local_38;
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00405480 @ 00405480  (req 405480) ============ */
/* calls: LeaveCriticalSection@EXTERNAL:00000026 Sleep@EXTERNAL:00000023 FUN_004037c0@004037c0 FUN_00403890@00403890 __security_check_cookie@0050a225 _memset@0050b7b0 EnterCriticalSection@EXTERNAL:00000025 */
/* called by: FUN_00409a70@00409a70 FUN_00409b70@00409b70 FUN_004149a0@004149a0 FUN_00415500@00415500 FUN_00416200@00416200 FUN_00416360@00416360 FUN_004168d0@004168d0 */

void FUN_00405480(void)

{
  char cVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined1 local_94;
  char local_93 [63];
  undefined4 local_54;
  undefined4 local_50;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c14b;
  local_10 = ExceptionList;
  local_14 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  local_94 = 0;
  _memset(local_93,0,0x3f);
  _memset(&local_54,0,0x40);
  local_54 = 0x10fa1;
  local_50 = 0;
  cVar1 = FUN_004037c0();
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  else {
    Sleep(300);
    _memset(&local_94,0,0x40);
    local_94 = 0xa1;
    cVar1 = FUN_00403890();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_93[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
    else {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_004055a0 @ 004055a0  (req 004055a0) ============ */
/* calls: LeaveCriticalSection@EXTERNAL:00000026 Sleep@EXTERNAL:00000023 FUN_004037c0@004037c0 FUN_00403890@00403890 __security_check_cookie@0050a225 _memset@0050b7b0 EnterCriticalSection@EXTERNAL:00000025 */
/* called by: FUN_00409a70@00409a70 FUN_00409b00@00409b00 FUN_00409b70@00409b70 FUN_004149a0@004149a0 FUN_00415500@00415500 FUN_00416200@00416200 FUN_00416360@00416360 FUN_004168d0@004168d0 */

void FUN_004055a0(void)

{
  char cVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined1 local_94;
  char local_93 [63];
  undefined4 local_54;
  undefined4 local_50;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c14b;
  local_10 = ExceptionList;
  local_14 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  local_94 = 0;
  _memset(local_93,0,0x3f);
  _memset(&local_54,0,0x40);
  local_54 = 0xf0fa1;
  local_50 = 0;
  cVar1 = FUN_004037c0();
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  else {
    Sleep(300);
    _memset(&local_94,0,0x40);
    local_94 = 0xa1;
    cVar1 = FUN_00403890();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_93[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
    else {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00403d90 @ 00403d90  (req 00403d90) ============ */
/* calls: FUN_00404230@00404230 Sleep@EXTERNAL:00000023 _memset@0050b7b0 __security_check_cookie@0050a225 FUN_004037c0@004037c0 LeaveCriticalSection@EXTERNAL:00000026 EnterCriticalSection@EXTERNAL:00000025 FUN_00403890@00403890 */
/* called by: FUN_004149a0@004149a0 */

void __cdecl FUN_00403d90(ushort param_1)

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
  puStack_c = &LAB_0052c2cb;
  local_10 = ExceptionList;
  local_18 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  _memset(&local_58,0,0x40);
  local_58 = 0x12a1;
  local_54 = 0;
  cVar1 = FUN_004037c0();
  Sleep((uint)param_1);
  if (CONCAT31(extraout_var,cVar1) != 0) {
    _memset(&local_470,0,0x411);
    local_470 = 0xa0;
    cVar1 = FUN_00403890();
    if (CONCAT31(extraout_var_00,cVar1) != 0) {
      if (local_46f == '\x01') {
        puVar3 = local_460;
        puVar4 = (undefined4 *)&DAT_00583448;
        for (iVar2 = 0x100; iVar2 != 0; iVar2 = iVar2 + -1) {
          *puVar4 = *puVar3;
          puVar3 = puVar3 + 1;
          puVar4 = puVar4 + 1;
        }
        FUN_00404230();
        LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
      }
      else {
        LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
      }
      goto LAB_00403e9c;
    }
  }
  LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
LAB_00403e9c:
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_004056c0 @ 004056c0  (req 004056c0) ============ */
/* calls: LeaveCriticalSection@EXTERNAL:00000026 Sleep@EXTERNAL:00000023 FUN_004037c0@004037c0 FUN_00403890@00403890 __security_check_cookie@0050a225 _memset@0050b7b0 EnterCriticalSection@EXTERNAL:00000025 */
/* called by: FUN_00416360@00416360 */

void __cdecl FUN_004056c0(ushort param_1)

{
  char cVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined1 local_98;
  char local_97 [63];
  undefined4 local_58;
  undefined4 local_54;
  uint local_18;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c20b;
  local_10 = ExceptionList;
  local_18 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  local_98 = 0;
  _memset(local_97,0,0x3f);
  _memset(&local_58,0,0x40);
  local_58 = 0x13a1;
  local_54 = 0;
  cVar1 = FUN_004037c0();
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  else {
    Sleep((uint)param_1);
    _memset(&local_98,0,0x40);
    local_98 = 0xa1;
    cVar1 = FUN_00403890();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_97[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
    else {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00404750 @ 00404750  (req 404750) ============ */
/* calls: FUN_00404d00@00404d00 Sleep@EXTERNAL:00000023 _memset@0050b7b0 __security_check_cookie@0050a225 FUN_004037c0@004037c0 LeaveCriticalSection@EXTERNAL:00000026 EnterCriticalSection@EXTERNAL:00000025 FUN_00403890@00403890 */
/* called by: FUN_00416200@00416200 */

void __thiscall FUN_00404750(void *this,ushort param_1)

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
  puStack_c = &LAB_0052c28b;
  local_10 = ExceptionList;
  local_38[8] = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
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
  FUN_00404d00(this);
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
  cVar1 = FUN_004037c0();
  Sleep((uint)param_1);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  else {
    _memset(&local_b8,0,0x40);
    local_b8 = 0xa1;
    FUN_00403890();
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  ExceptionList = local_10;
  __security_check_cookie(local_38[8] ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00404bb0 @ 00404bb0  (req 00404bb0) ============ */
/* calls: Sleep@EXTERNAL:00000023 _memset@0050b7b0 __security_check_cookie@0050a225 FUN_004037c0@004037c0 LeaveCriticalSection@EXTERNAL:00000026 EnterCriticalSection@EXTERNAL:00000025 FUN_00403890@00403890 */
/* called by: FUN_00416200@00416200 */

void __thiscall FUN_00404bb0(void *this,ushort param_1)

{
  char cVar1;
  undefined3 extraout_var;
  undefined1 local_98;
  undefined1 local_97 [63];
  undefined4 local_58;
  undefined2 local_54;
  undefined1 local_48;
  undefined1 local_47;
  undefined1 local_46;
  undefined1 local_45;
  undefined1 local_44;
  undefined1 local_43;
  undefined1 local_42;
  undefined1 local_41;
  undefined1 local_40;
  undefined1 local_3f;
  uint local_18;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c20b;
  local_10 = ExceptionList;
  local_18 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  local_98 = 0;
  _memset(local_97,0,0x3f);
  _memset(&local_58,0,0x40);
  local_48 = *(undefined1 *)((int)this + 0xd);
  local_47 = *(undefined1 *)((int)this + 4);
  local_46 = *(undefined1 *)((int)this + 7);
  local_45 = *(undefined1 *)((int)this + 5);
  local_44 = *(undefined1 *)((int)this + 0x2e);
  local_43 = *(undefined1 *)((int)this + 0x36);
  local_42 = *(undefined1 *)((int)this + 0x3e);
  local_41 = *(undefined1 *)((int)this + 0x46);
  local_40 = *(undefined1 *)((int)this + 0x4e);
  local_3f = *(undefined1 *)((int)this + 6);
  local_58 = 0xa0f15a1;
  local_54 = 0;
  cVar1 = FUN_004037c0();
  Sleep((uint)param_1);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  else {
    _memset(&local_98,0,0x40);
    local_98 = 0xa1;
    FUN_00403890();
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00404890 @ 00404890  (req 404890) ============ */
/* calls: LeaveCriticalSection@EXTERNAL:00000026 HidD_SetFeature@EXTERNAL:00000005 FUN_00404a20@00404a20 Sleep@EXTERNAL:00000023 FUN_00403890@00403890 __security_check_cookie@0050a225 _memset@0050b7b0 EnterCriticalSection@EXTERNAL:00000025 */
/* called by: FUN_00416200@00416200 */

void __thiscall FUN_00404890(void *this,ushort param_1)

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
  puStack_c = &LAB_0052c24b;
  local_10 = ExceptionList;
  local_18 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  iVar3 = 0;
  local_8 = 0;
  local_98 = 0;
  _memset(local_97,0,0x3f);
  _memset(local_d8,0,0x40);
  local_e0 = (undefined4 *)FUN_00404a20((int)this);
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
    if (DAT_00583444 != 0) {
      cVar1 = HidD_SetFeature(DAT_00583444,&local_58,0x40);
    }
    Sleep((uint)param_1);
    if (cVar1 == '\0') break;
    _memset(&local_98,0,0x40);
    local_98 = 0xa1;
    FUN_00403890();
    Sleep(100);
    local_e0 = local_e0 + 7;
    iVar3 = iVar3 + 1;
    if (1 < iVar3) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
LAB_004049ee:
      ExceptionList = local_10;
      __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
      return;
    }
  }
  LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  goto LAB_004049ee;
}


/* ============ FUN_004050c0 @ 004050c0  (req 004050c0) ============ */
/* calls: LeaveCriticalSection@EXTERNAL:00000026 Sleep@EXTERNAL:00000023 FUN_004037c0@004037c0 FUN_00403890@00403890 __security_check_cookie@0050a225 _memset@0050b7b0 EnterCriticalSection@EXTERNAL:00000025 */
/* called by: FUN_004149a0@004149a0 */

void __thiscall FUN_004050c0(void *this,undefined1 *param_1)

{
  char cVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined1 local_98;
  char local_97 [15];
  undefined1 local_88;
  undefined4 local_58;
  undefined2 local_54;
  uint local_18;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c1cb;
  local_10 = ExceptionList;
  local_18 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  local_98 = 0;
  _memset(local_97,0,0x3f);
  _memset(&local_58,0,0x40);
  local_58 = 0xb4a1;
  local_54 = 0;
  cVar1 = FUN_004037c0();
  Sleep((uint)this & 0xffff);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  else {
    _memset(&local_98,0,0x40);
    local_98 = 0xa1;
    cVar1 = FUN_00403890();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_97[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
    else {
      *param_1 = local_88;
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00403a50 @ 00403a50  (req 00403a50) ============ */
/* calls: Sleep@EXTERNAL:00000023 _memset@0050b7b0 __security_check_cookie@0050a225 FUN_004037c0@004037c0 LeaveCriticalSection@EXTERNAL:00000026 EnterCriticalSection@EXTERNAL:00000025 FUN_00403890@00403890 */
/* called by: FUN_00409a70@00409a70 */

void FUN_00403a50(void)

{
  char cVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined1 local_94 [64];
  undefined4 local_54;
  undefined2 local_50;
  undefined1 local_4e;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c14b;
  local_10 = ExceptionList;
  local_14 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  _memset(&local_54,0,0x40);
  local_54 = 0x70a1;
  local_50 = 0;
  local_4e = 0;
  cVar1 = FUN_004037c0();
  if (CONCAT31(extraout_var,cVar1) != 0) {
    Sleep(300);
    _memset(local_94,0,0x40);
    local_94[0] = 0xa1;
    cVar1 = FUN_00403890();
    if (CONCAT31(extraout_var_00,cVar1) != 0) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
      goto LAB_00403b26;
    }
  }
  LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
LAB_00403b26:
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00403b50 @ 00403b50  (req 00403b50) ============ */
/* calls: Sleep@EXTERNAL:00000023 _memset@0050b7b0 __security_check_cookie@0050a225 FUN_004037c0@004037c0 LeaveCriticalSection@EXTERNAL:00000026 EnterCriticalSection@EXTERNAL:00000025 FUN_00403890@00403890 */
/* called by: FUN_00409b00@00409b00 */

void FUN_00403b50(void)

{
  char cVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined1 local_94 [64];
  undefined4 local_54;
  undefined2 local_50;
  undefined1 local_4e;
  uint local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c14b;
  local_10 = ExceptionList;
  local_14 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  _memset(&local_54,0,0x40);
  local_54 = 0x71a1;
  local_50 = 0;
  local_4e = 0;
  cVar1 = FUN_004037c0();
  if (CONCAT31(extraout_var,cVar1) != 0) {
    Sleep(300);
    _memset(local_94,0,0x40);
    local_94[0] = 0xa1;
    cVar1 = FUN_00403890();
    if (CONCAT31(extraout_var_00,cVar1) != 0) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
      goto LAB_00403c28;
    }
  }
  LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
LAB_00403c28:
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00403c50 @ 00403c50  (req 00403c50) ============ */
/* calls: Sleep@EXTERNAL:00000023 _memset@0050b7b0 __security_check_cookie@0050a225 FUN_004037c0@004037c0 LeaveCriticalSection@EXTERNAL:00000026 EnterCriticalSection@EXTERNAL:00000025 FUN_00403890@00403890 */
/* called by: FUN_00409b70@00409b70 */

void __cdecl FUN_00403c50(undefined4 *param_1)

{
  char cVar1;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  undefined4 local_98;
  undefined2 local_94;
  undefined1 local_58;
  char local_57 [15];
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  uint local_18;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c30b;
  local_10 = ExceptionList;
  local_18 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  local_8 = 0;
  local_58 = 0;
  _memset(local_57,0,0x3f);
  _memset(&local_98,0,0x40);
  local_98 = 0x72a1;
  local_94 = 0;
  cVar1 = FUN_004037c0();
  Sleep(0x15e);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
  }
  else {
    _memset(&local_58,0,0x40);
    local_58 = 0xa1;
    cVar1 = FUN_00403890();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_57[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
    else {
      *param_1 = local_48;
      param_1[1] = local_44;
      param_1[2] = local_40;
      param_1[3] = local_3c;
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057cce0);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}


