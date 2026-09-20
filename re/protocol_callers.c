/* index

--- callers of 0x00403740 (FUN_00403740) ---
    FUN_004039d0                 @ 004039d0   (call site 00403a42)
    FUN_00403ad0                 @ 00403ad0   (call site 00403b42)
    FUN_00403bd0                 @ 00403bd0   (call site 00403c63)
    FUN_00403d10                 @ 00403d10   (call site 00403d7e)
    FUN_00404720                 @ 00404720   (call site 004047da)
    FUN_00404b80                 @ 00404b80   (call site 00404c53)
    FUN_004050c0                 @ 004050c0   (call site 0040514f)
    FUN_00405200                 @ 00405200   (call site 00405293)
    FUN_00405340                 @ 00405340   (call site 004053d3)
    FUN_00405480                 @ 00405480   (call site 00405503)
    FUN_004055a0                 @ 004055a0   (call site 00405623)
    FUN_004056c0                 @ 004056c0   (call site 00405743)
    total new: 12

*/

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
 * FUN_004039d0 @ 004039d0
 * ====================================================== */

void FUN_004039d0(void)

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
  puStack_c = &LAB_0052f1cb;
  local_10 = ExceptionList;
  local_14 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  _memset(&local_54,0,0x40);
  local_54 = 0x70a1;
  local_50 = 0;
  local_4e = 0;
  cVar1 = FUN_00403740();
  if (CONCAT31(extraout_var,cVar1) != 0) {
    Sleep(300);
    _memset(local_94,0,0x40);
    local_94[0] = 0xa1;
    cVar1 = FUN_00403810();
    if (CONCAT31(extraout_var_00,cVar1) != 0) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
      goto LAB_00403aa6;
    }
  }
  LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
LAB_00403aa6:
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}



/* ======================================================
 * FUN_00403ad0 @ 00403ad0
 * ====================================================== */

void FUN_00403ad0(void)

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
  puStack_c = &LAB_0052f1cb;
  local_10 = ExceptionList;
  local_14 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  _memset(&local_54,0,0x40);
  local_54 = 0x71a1;
  local_50 = 0;
  local_4e = 0;
  cVar1 = FUN_00403740();
  if (CONCAT31(extraout_var,cVar1) != 0) {
    Sleep(300);
    _memset(local_94,0,0x40);
    local_94[0] = 0xa1;
    cVar1 = FUN_00403810();
    if (CONCAT31(extraout_var_00,cVar1) != 0) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
      goto LAB_00403ba8;
    }
  }
  LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
LAB_00403ba8:
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}



/* ======================================================
 * FUN_00403bd0 @ 00403bd0
 * ====================================================== */

void __cdecl FUN_00403bd0(undefined4 *param_1)

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
  puStack_c = &LAB_0052f18b;
  local_10 = ExceptionList;
  local_18 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  local_58 = 0;
  _memset(local_57,0,0x3f);
  _memset(&local_98,0,0x40);
  local_98 = 0x72a1;
  local_94 = 0;
  cVar1 = FUN_00403740();
  Sleep(0x15e);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  else {
    _memset(&local_58,0,0x40);
    local_58 = 0xa1;
    cVar1 = FUN_00403810();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_57[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
    else {
      *param_1 = local_48;
      param_1[1] = local_44;
      param_1[2] = local_40;
      param_1[3] = local_3c;
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
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
 * FUN_00404b80 @ 00404b80
 * ====================================================== */

void __thiscall FUN_00404b80(void *this,ushort param_1)

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
  undefined1 local_3e;
  uint local_18;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052f08b;
  local_10 = ExceptionList;
  local_18 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  local_98 = 0;
  _memset(local_97,0,0x3f);
  _memset(&local_58,0,0x40);
  local_48 = *(undefined1 *)((int)this + 0xf);
  local_47 = *(undefined1 *)((int)this + 4);
  local_46 = *(undefined1 *)((int)this + 7);
  local_45 = *(undefined1 *)((int)this + 5);
  local_44 = *(undefined1 *)((int)this + 0x34);
  local_43 = *(undefined1 *)((int)this + 0x3c);
  local_42 = *(undefined1 *)((int)this + 0x44);
  local_41 = *(undefined1 *)((int)this + 0x4c);
  local_40 = *(undefined1 *)((int)this + 0x54);
  local_3f = *(undefined1 *)((int)this + 6);
  local_3e = *(undefined1 *)((int)this + 0x10);
  local_58 = 0xa0f15a1;
  local_54 = 0;
  cVar1 = FUN_00403740();
  Sleep((uint)param_1);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  else {
    _memset(&local_98,0,0x40);
    local_98 = 0xa1;
    FUN_00403810();
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}



/* ======================================================
 * FUN_004050c0 @ 004050c0
 * ====================================================== */

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
  puStack_c = &LAB_0052f04b;
  local_10 = ExceptionList;
  local_18 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  local_98 = 0;
  _memset(local_97,0,0x3f);
  _memset(&local_58,0,0x40);
  local_58 = 0xb4a1;
  local_54 = 0;
  cVar1 = FUN_00403740();
  Sleep((uint)this & 0xffff);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  else {
    _memset(&local_98,0,0x40);
    local_98 = 0xa1;
    cVar1 = FUN_00403810();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_97[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
    else {
      *param_1 = local_88;
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}



/* ======================================================
 * FUN_00405200 @ 00405200
 * ====================================================== */

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
  puStack_c = &LAB_0052f00b;
  local_10 = ExceptionList;
  local_14 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  local_54 = 0;
  _memset(local_53,0,0x3f);
  _memset(&local_94,0,0x40);
  local_94 = 0xea1;
  local_90 = 0;
  cVar1 = FUN_00403740();
  Sleep((uint)param_2);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  else {
    _memset(&local_54,0,0x40);
    local_54 = 0xa1;
    cVar1 = FUN_00403810();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_53[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
    else {
      *param_1 = local_44;
      param_1[1] = local_40;
      param_1[2] = local_3c;
      *(undefined2 *)(param_1 + 3) = local_38;
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}



/* ======================================================
 * FUN_00405340 @ 00405340
 * ====================================================== */

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
  puStack_c = &LAB_0052f18b;
  local_10 = ExceptionList;
  local_18 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  local_58 = 0;
  _memset(local_57,0,0x3f);
  _memset(&local_98,0,0x40);
  local_98 = 0xda1;
  local_94 = 0;
  cVar1 = FUN_00403740();
  Sleep(0x96);
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  else {
    _memset(&local_58,0,0x40);
    local_58 = 0xa1;
    cVar1 = FUN_00403810();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_57[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
    else {
      *param_1 = local_48;
      param_1[1] = local_44;
      param_1[2] = local_40;
      *(undefined2 *)(param_1 + 3) = local_3c;
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}



/* ======================================================
 * FUN_00405480 @ 00405480
 * ====================================================== */

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
  puStack_c = &LAB_0052f1cb;
  local_10 = ExceptionList;
  local_14 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  local_94 = 0;
  _memset(local_93,0,0x3f);
  _memset(&local_54,0,0x40);
  local_54 = 0x10fa1;
  local_50 = 0;
  cVar1 = FUN_00403740();
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  else {
    Sleep(300);
    _memset(&local_94,0,0x40);
    local_94 = 0xa1;
    cVar1 = FUN_00403810();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_93[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
    else {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}



/* ======================================================
 * FUN_004055a0 @ 004055a0
 * ====================================================== */

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
  puStack_c = &LAB_0052f1cb;
  local_10 = ExceptionList;
  local_14 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  local_94 = 0;
  _memset(local_93,0,0x3f);
  _memset(&local_54,0,0x40);
  local_54 = 0xf0fa1;
  local_50 = 0;
  cVar1 = FUN_00403740();
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  else {
    Sleep(300);
    _memset(&local_94,0,0x40);
    local_94 = 0xa1;
    cVar1 = FUN_00403810();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_93[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
    else {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_14 ^ (uint)&stack0xfffffffc);
  return;
}



/* ======================================================
 * FUN_004056c0 @ 004056c0
 * ====================================================== */

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
  puStack_c = &LAB_0052f08b;
  local_10 = ExceptionList;
  local_18 = DAT_0057e340 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  EnterCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  local_8 = 0;
  local_98 = 0;
  _memset(local_97,0,0x3f);
  _memset(&local_58,0,0x40);
  local_58 = 0x13a1;
  local_54 = 0;
  cVar1 = FUN_00403740();
  if (CONCAT31(extraout_var,cVar1) == 0) {
    LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
  }
  else {
    Sleep((uint)param_1);
    _memset(&local_98,0,0x40);
    local_98 = 0xa1;
    cVar1 = FUN_00403810();
    if ((CONCAT31(extraout_var_00,cVar1) == 0) || (local_97[0] != '\x01')) {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
    else {
      LeaveCriticalSection((LPCRITICAL_SECTION)&DAT_0057fc80);
    }
  }
  ExceptionList = local_10;
  __security_check_cookie(local_18 ^ (uint)&stack0xfffffffc);
  return;
}


