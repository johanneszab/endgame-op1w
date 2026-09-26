/* v1/notify.c -- the notification channel of the v1 configuration tool.
 *
 * Program: Endgame_Gear_OP1w_4k_Configuration_Tool_v1.03.exe
 *   (OP1w 4k **v1** vendor tool; Ghidra project OP1w, image base 0x00400000)
 * Extracted with Ghidra 12.1.4 headless, scripts in git/re/ghidra_scripts.
 * Evidence class for everything in this file: [BIN].
 *
 * Addresses covered:
 *   FUN_004142b0   starts the listener: hid_open() on a SECOND handle
 *                  DAT_00584314, installs the dispatch callback into
 *                  DAT_00583430, then _beginthreadex(FUN_00414210)
 *   FUN_00414210   the blocking hid_read_timeout loop
 *                  -- v2 counterpart FUN_00416ad0
 *   FUN_00414400   the callback in DAT_00583430 -- a gate, nothing more
 *   FUN_00415b70   THE ACTUAL EVENT DISPATCHER: switch on the event code
 *   hid_read_timeout @ 00403190, hid_read @ 004032a0
 *
 * Globals:
 *   DAT_00584314   second hidapi handle, notification collection
 *                  (v2 counterpart DAT_00587180)
 *   DAT_00583430   function pointer, set to FUN_00414400 (v2: DAT_00586290)
 *   DAT_00583443   "listener running" flag, tested by FUN_00414400
 *   DAT_0057ccd8   link/model state: 0xFF = no mouse, 0x11 = v1 wired?,
 *                  0x22 = wireless link up (values from FUN_00416530)
 *
 * SHAPE OF THE LOOP (identical to v2): the 8-byte buffer is pre-loaded with
 * report ID 3 at buffer[0], hid_read_timeout() reads 8 bytes, and report bytes
 * 1..4 are repacked into one dword that is handed to the callback:
 *
 *       local_10 = 3;                               // buffer[0] = report ID
 *       while (DAT_00584314 != NULL &&
 *              hid_read_timeout(DAT_00584314, &local_10, 8, ...) != -1) {
 *           local_18 = CONCAT13(local_c, uStack_f); // report bytes 1,2,3,4
 *           local_19 = 1;
 *           local_14._0_3_ = CONCAT12(uStack_9, uStack_b);  // bytes 5,6,7
 *           (*DAT_00583430)(&local_19, &local_18);
 *           Sleep(10);
 *       }
 *
 * FUN_00414400 ignores the payload argument entirely and calls FUN_00415b70,
 * which reads the packed dword through an unaff_EDI that Ghidra could not tie
 * to a parameter. In FUN_00415b70:
 *       uVar2 & 0xff   = report byte 1  -- the event code
 *       local_1b       = report byte 2
 *       sStack_1a      = report bytes 3,4 as a little-endian short
 *
 * THE COMPLETE SET OF EVENT CODES THE v1 TOOL COMPARES AGAINST is the switch in
 * FUN_00415b70, and it is only these four:
 *
 *   case 0x06   polling/power-mode change. Sub-value = report byte 2, and only
 *               8, 4 and 2 are handled (1000/2000/4000 Hz). It selects the
 *               matching combo-box item (CB_SETCURSEL 0x14E with index 0/1/2)
 *               and writes the raw value into BOTH the live settings struct
 *               (DAT_00583854 = settings +0x04) and the shadow copy
 *               (DAT_00584324), then UpdateData(FALSE). Two details worth
 *               noting: the whole arm is gated on this[0x3014] == 0xF1, and an
 *               unrecognised sub-value jumps into the "2" arm past its
 *               DAT_0057ccd8 == 0x22 test, so it is applied unconditionally.
 *               *** This event code is not in PROTOCOL.md S4a. ***
 *   case 0x0E   mouse-info event. Gate: `if (sStack_1a != 0x1972)` -- i.e. the
 *               tool compares report bytes 3,4 against the v1 mouse PID and
 *               calls FUN_00416470 (enable/disable the whole control set) when
 *               they do not match. Note this is bytes 3,4, not the +2..+3 of a
 *               cmd 0x0E *response*; the event layout here is not established.
 *   case 0xB1   radio link event. Acts only when report byte 2 is 0xF0 or 0xF1,
 *               and only if DAT_0057ccd8 is neither 0x11 nor 0xFF: blanks the
 *               five status labels, sets "Connection: N/A", DAT_0057ccd8 = 0xFF.
 *               (0xF1 as a link-down sub-code is not in PROTOCOL.md either.)
 *   case 0xB4   battery event. report byte 2 is the percentage; < 100 with
 *               DAT_0057ccd8 == 0x11 shows "Charging", otherwise "<n>%".
 *
 * There is NO case 0x31, and no default arm -- an unrecognised code falls off
 * the end of the switch and is silently discarded.
 */

/* ======================================================
 * hid_read_timeout @ 00403190
 * ====================================================== */

size_t __cdecl hid_read_timeout(undefined4 *param_1,void *param_2,DWORD param_3,DWORD param_4)

{
  HANDLE hEvent;
  char *_Src;
  BOOL BVar1;
  DWORD DVar2;
  DWORD _Size;
  DWORD local_8;
  
                    /* 0x3190  15  hid_read_timeout */
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
        goto LAB_004031fc;
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
LAB_004031fc:
  FUN_00402730();
  return 0xffffffff;
}


/* ======================================================
 * hid_read @ 004032a0
 * ====================================================== */

void __cdecl hid_read(undefined4 *param_1,void *param_2,DWORD param_3)

{
                    /* 0x32a0  14  hid_read */
  hid_read_timeout(param_1,param_2,param_3,-(uint)(param_1[1] != 0));
  return;
}


/* ============ FUN_004142b0 @ 004142b0  (req 004142b0) ============ */
/* calls: hid_close@004033a0 __beginthreadex@0050b061 hid_open@00402e40 CloseHandle@EXTERNAL:00000020 */
/* called by: FUN_004147b0@004147b0 FUN_00415500@00415500 FUN_004168d0@004168d0 */

/* WARNING: Removing unreachable block (ram,0x004142bd) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_004142b0(void)

{
  short sVar1;
  short sVar2;
  uint local_8;
  
  DAT_00583430 = FUN_00414400;
  sVar1 = DAT_0058447c;
  sVar2 = DAT_0058447e;
  if ((DAT_0057ccd8 == '\x11') || (sVar1 = DAT_00584484, sVar2 = DAT_00584486, DAT_0057ccd8 == '\"')
     ) {
    DAT_00584314 = hid_open(sVar1,sVar2,(ushort *)0x0);
  }
  if (DAT_0057ccd8 != -1) {
    if (DAT_00584314 == 0) {
      hid_close((undefined4 *)0x0);
      return 0xff;
    }
    DAT_00583443 = 1;
    _DAT_00584318 = (HANDLE)__beginthreadex((void *)0x0,0,FUN_00414210,(void *)0x1,0,&local_8);
    if (_DAT_00584318 == (HANDLE)0x0) {
      DAT_00583443 = 0;
      return 0x13;
    }
    CloseHandle(_DAT_00584318);
  }
  return 5;
}


/* ======================================================
 * FUN_00414210 @ 00414210
 * ====================================================== */

void FUN_00414210(int param_1)

{
  undefined3 uVar1;
  size_t sVar2;
  undefined1 local_19;
  undefined4 local_18;
  undefined4 local_14;
  undefined1 local_10;
  undefined3 uStack_f;
  undefined1 local_c;
  undefined2 uStack_b;
  undefined1 uStack_9;
  uint local_8;
  
  local_8 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  local_18 = 0;
  local_14 = 0;
  local_c = 0;
  uStack_b = 0;
  uStack_9 = 0;
  local_19 = 0;
  uStack_f = 0;
  uVar1 = uStack_f;
  local_10 = 3;
  uStack_f = 0;
  if (param_1 != 0) {
    while ((uVar1 = uStack_f, DAT_00584314 != (undefined4 *)0x0 &&
           (sVar2 = hid_read_timeout(DAT_00584314,&local_10,8,-(uint)(DAT_00584314[1] != 0)),
           uVar1 = uStack_f, sVar2 != 0xffffffff))) {
      local_18 = CONCAT13(local_c,uStack_f);
      local_19 = 1;
      local_14._0_3_ = CONCAT12(uStack_9,uStack_b);
      (*DAT_00583430)(&local_19,&local_18);
      Sleep(10);
    }
  }
  uStack_f = uVar1;
  __security_check_cookie(local_8 ^ (uint)&stack0xfffffffc);
  return;
}


/* ============ FUN_00414400 @ 00414400  (req 414400) ============ */
/* calls: FUN_00415b70@00415b70 */
/* called by: FUN_004142b0@004142b0 */

undefined4 FUN_00414400(char *param_1)

{
  if ((DAT_00583443 != '\0') && (*param_1 != '\0')) {
    FUN_00415b70();
  }
  return 1;
}


/* ============ FUN_00415b70 @ 00415b70  (req 00415b70) ============ */
/* calls: FUN_0040c2a0@0040c2a0 FUN_00402260@00402260 FUN_00402190@00402190 UpdateData@00419797 FUN_00420cf9@00420cf9 FUN_004168d0@004168d0 SetWindowTextW@0041dc58 FUN_00416470@00416470 SendMessageW@EXTERNAL:00000183 */
/* called by: FUN_00414400@00414400 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00415b70(void)

{
  wchar_t *pwVar1;
  CWnd *this;
  uint uVar2;
  undefined **ppuVar3;
  int iVar4;
  uint *unaff_EDI;
  HWND hWnd;
  byte local_1b;
  short sStack_1a;
  wchar_t *local_14;
  void *local_10;
  undefined1 *puStack_c;
  undefined4 local_8;
  
  this = DAT_0058431c;
  local_8 = 0xffffffff;
  puStack_c = &LAB_0052c6e8;
  local_10 = ExceptionList;
  uVar2 = DAT_0057b3b0 ^ (uint)&stack0xfffffffc;
  ExceptionList = &local_10;
  ppuVar3 = FUN_00420cf9();
  if (ppuVar3 == (undefined **)0x0) {
                    /* WARNING: Subroutine does not return */
    FUN_00402190((undefined4 *)0x80004005);
  }
  iVar4 = (**(code **)(*ppuVar3 + 0xc))(uVar2);
  local_14 = (wchar_t *)(iVar4 + 0x10);
  local_8 = 0;
  uVar2 = *unaff_EDI;
  local_1b = (byte)(uVar2 >> 8);
  sStack_1a = (short)(uVar2 >> 0x10);
  switch(uVar2 & 0xff) {
  case 6:
    if (this[0x3014] != (CWnd)0xf1) break;
    uVar2 = uVar2 >> 8 & 0xff;
    if (uVar2 == 2) {
      if (DAT_0057ccd8 == '\"') {
        hWnd = *(HWND *)(this + 0x1ab4);
LAB_00415e05:
        SendMessageW(hWnd,0x14e,2,0);
        DAT_00584324 = 2;
        DAT_00583854 = 2;
      }
    }
    else if (uVar2 == 4) {
      if (DAT_0057ccd8 == '\"') {
        SendMessageW(*(HWND *)(this + 0x1ab4),0x14e,1,0);
        DAT_00583854 = 4;
        DAT_00584324 = 4;
      }
    }
    else {
      if (uVar2 != 8) {
        hWnd = *(HWND *)(this + 0x1ab4);
        goto LAB_00415e05;
      }
      SendMessageW(*(HWND *)(this + 0x1ab4),0x14e,0,0);
      DAT_00583854 = 8;
      DAT_00584324 = 8;
    }
    CWnd::UpdateData(this,0);
    break;
  case 0xe:
    if (sStack_1a != 0x1972) {
      FUN_00416470();
    }
    break;
  case 0xb1:
    if ((DAT_0057ccd8 != '\x11') &&
       (((local_1b == 0xf0 || (local_1b == 0xf1)) && (DAT_0057ccd8 != -1)))) {
      this[0x3015] = (CWnd)0x0;
      FUN_00416470();
      _DAT_005833b4 = 0;
      DAT_0057ccd8 = -1;
      CWnd::SetWindowTextW(this + 0x2e44,L"");
      CWnd::SetWindowTextW(this + 0x2dd0,L"N/A");
      CWnd::SetWindowTextW(this + 0x2fa0,L"");
      CWnd::SetWindowTextW(this + 0x2eb8,L"");
      CWnd::SetWindowTextW(this + 0x2f2c,L"");
    }
    break;
  case 0xb4:
    if (this[0x3014] == (CWnd)0xf1) {
      if (DAT_0057ccd8 != '\"') {
        if (DAT_0057ccd8 != '\x11') {
          FUN_004168d0((int)this);
          break;
        }
        if (local_1b < 100) {
          CWnd::SetWindowTextW(this + 0x2eb8,L"Charging");
          break;
        }
      }
      FUN_00402260(&local_14,L"%d");
      FUN_0040c2a0(&local_14,L"%");
      CWnd::SetWindowTextW(this + 0x2eb8,local_14);
    }
  }
  local_8 = 0xffffffff;
  pwVar1 = local_14 + -2;
  LOCK();
  iVar4 = *(int *)pwVar1;
  *(int *)pwVar1 = *(int *)pwVar1 + -1;
  UNLOCK();
  if (iVar4 == 1 || iVar4 + -1 < 0) {
    (**(code **)(**(int **)(local_14 + -8) + 4))(local_14 + -8);
  }
  ExceptionList = local_10;
  return;
}


/* ---- refs to the dispatch globals (FindRefs.java) ----
=== refs to 583430 ===
  00414286  FUN_00414210   @ 00414210  [READ]  CALL dword ptr [0x00583430]
  004142cc  FUN_004142b0   @ 004142b0  [WRITE]  MOV dword ptr [0x00583430],0x414400
  -- distinct functions: [FUN_00414210 @ 00414210, FUN_004142b0 @ 004142b0]
=== refs to 584314 ===
  00414241  FUN_00414210   @ 00414210  [READ]  MOV EAX,[0x00584314]
  0041430d  FUN_004142b0   @ 004142b0  [WRITE]  MOV [0x00584314],EAX
  00414320  FUN_004142b0   @ 004142b0  [READ]  CMP dword ptr [0x00584314],0x0
  -- distinct functions: [FUN_00414210 @ 00414210, FUN_004142b0 @ 004142b0]

*/


/* ---- every instruction in the binary whose scalar operand is 0x31
       (FindConst.java). All four in-range hits are the struct offset
       +0x31, inside the button table; none is an event or command code. ----

looking for scalars: [49]
0x31  FUN_00403ec0 @ 00403ec0   004040c7  MOVZX EDX,byte ptr [EAX + 0x31]
0x31  FUN_00404230 @ 00404230   004044f2  MOV byte ptr [EAX + 0x31],CL
0x31  FUN_00404a20 @ 00404a20   00404a57  MOVZX EDX,byte ptr [ECX + 0x31]
0x31  FUN_00404a20 @ 00404a20   00404b7a  MOV byte ptr [EAX + 0x31],DL
0x31  FUN_00404dc0 @ 00404dc0   00404f0b  MOV byte ptr [EAX + 0x31],DL
0x31  FUN_00404dc0 @ 00404dc0   00404f69  MOVZX EDX,byte ptr [ECX + 0x31]
0x31  FUN_0040b560 @ 0040b560   0040b609  MOV EAX,0x31
0x31  FUN_00415500 @ 00415500   00415a58  MOV byte ptr [ESP + 0x31],AL
0x31  MeasureItem @ 0042340a   00423450  PUSH 0x31
0x31  DrawItem @ 0042362c   0042374e  PUSH 0x31
0x31  OnSettingChange @ 0042991b   00429928  PUSH 0x31
0x31  OnPromptReset @ 004379b7   00437a51  PUSH 0x31
0x31  AfxFormatStrings @ 00441045   00441077  CMP EAX,0x31
0x31  AfxFormatStrings @ 00441045   004410e4  CMP EAX,0x31
0x31  OnDrawRibbonCaption @ 00450a8a   00450b01  PUSH 0x31
0x31  GetFont @ 00461748   0046174c  PUSH 0x31
0x31  OnDraw @ 00492c28   00492c5f  PUSH 0x31
0x31  OnDraw @ 00492c28   00492c69  PUSH 0x31
0x31  __cftoa_l @ 005157b7   00515940  MOV byte ptr [ESI + 0x2],0x31
0x31  __fptostr @ 0051da42   0051dad2  CMP byte ptr [ESI],0x31
0x31  ___strgtold12_l @ 0051ec49   0051ee88  SUB AL,0x31
total hits: 21

*/


/* ---- every instruction whose scalar operand is a wireless mouse PID ----

looking for scalars: [6514, 6532, 6504, 6530]
0x1972  FUN_00409b70 @ 00409b70   00409b9f  PUSH 0x1972
0x1972  FUN_004149a0 @ 004149a0   00414b24  MOV ECX,0x1972
0x1972  FUN_00415b70 @ 00415b70   00415cac  MOV ECX,0x1972
0x1972  FUN_004168d0 @ 004168d0   0041695a  MOV ECX,0x1972
total hits: 4

*/


