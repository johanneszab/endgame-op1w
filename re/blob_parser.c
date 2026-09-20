/* index

--- callers of 0x004041e0 (FUN_004041e0) ---
    FUN_00403d10                 @ 00403d10   (call site 00403e0a)
    FUN_00417260                 @ 00417260   (call site 0041759d)
    FUN_00417260                 @ 00417260   (call site 0041776f)
    total new: 2

*/

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


