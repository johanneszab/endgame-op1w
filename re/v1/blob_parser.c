/* v1/blob_parser.c -- config blob -> settings struct, and the struct copies.
 *
 * Program: Endgame_Gear_OP1w_4k_Configuration_Tool_v1.03.exe
 *   (OP1w 4k **v1** vendor tool; Ghidra project OP1w, image base 0x00400000)
 * Extracted with Ghidra 12.1.4 headless, scripts in git/re/ghidra_scripts.
 * Evidence class for everything in this file: [BIN].
 *
 * Addresses covered:
 *   FUN_00404230   the blob parser        -- v2 counterpart FUN_004041e0
 *   FUN_00403ec0   settings struct -> "shadow"/baseline copy at 0x00584320
 *   FUN_00404dc0   settings struct -> flat 0x6F-byte profile buffer
 *                  (used by FUN_00416590, the dirty-check / profile export)
 *   FUN_00416120   FACTORY-DEFAULTS filler for the settings struct
 *
 * Bases established from the disassembly of FUN_00403d90 (see ctx dump at the
 * bottom of this file):
 *       MOV EDI,0x583448   <- the 1024-byte blob lands here (blob offset 0)
 *       MOV EAX,0x583850   <- the settings struct the parser fills
 *       REP MOVSD ; CALL FUN_00404230
 *
 * So in FUN_00404230, in_EAX[k] = DAT_005834xx means
 *       settings struct + k   <-   blob byte (0x5834xx - 0x583448).
 *
 * The mapping for the cmd 0x14 fields comes out as:
 *   settings +0x08 <- blob 0x0D   active CPI stage
 *   settings +0x09 <- blob 0x0E   CPI levels
 *   settings +0x0A <- blob 0x07   (cmd 0x14 payload +0, always zero)
 *   settings +0x0B <- blob 0x0A   angle snapping
 *   settings +0x0C <- blob 0x0B   ripple control
 *   settings +0x0E <- blob 0x08   LED on lift-off
 *   settings +0x0F <- blob 0x09   lift-off distance
 * and for the cmd 0x15 fields:
 *   settings +0x00 <- blob 0x00
 *   settings +0x04 <- blob 0x05   polling / power mode
 *   settings +0x05 <- blob 0x04   power saving
 *   settings +0x06 <- blob 0x03   deep sleep
 *   settings +0x07 <- blob 0x06   flags
 *   settings +0x0D <- blob 0x0C   motion sync
 * These agree with PROTOCOL.md S4's blob table, i.e. the v1 and v2 blob layouts
 * really are the same.
 *
 * FUN_00416120 is worth reading closely: it is the only code besides the blob
 * parser that writes settings +0x08. It does so as part of a dword store,
 *       *(undefined4 *)(in_EAX + 6) = 0x04021103;
 * i.e. +0x06 = 0x03 (deep sleep 3 min), +0x07 = 0x11 (flags), +0x08 = 0x02
 * (active CPI stage), +0x09 = 0x04 (CPI levels). It is called from
 * FUN_004147b0 (dialog init) and FUN_00416360 (factory reset), each time on
 * BOTH 0x00583850 and the shadow copy 0x00584320 -- never from a UI control.
 */

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


/* ============ FUN_00404230 @ 00404230  (req 404230) ============ */
/* calls: */
/* called by: FUN_00403d90@00403d90 FUN_004149a0@004149a0 */

void FUN_00404230(void)

{
  byte bVar1;
  undefined1 *in_EAX;
  
  *in_EAX = DAT_00583448;
  in_EAX[6] = DAT_0058344b;
  in_EAX[5] = DAT_0058344c;
  in_EAX[4] = DAT_0058344d;
  in_EAX[7] = DAT_0058344e;
  in_EAX[10] = DAT_0058344f;
  in_EAX[0xe] = DAT_00583450;
  in_EAX[0xf] = DAT_00583451;
  in_EAX[0xb] = DAT_00583452;
  in_EAX[0xc] = DAT_00583453;
  in_EAX[0xd] = DAT_00583454;
  in_EAX[8] = DAT_00583455;
  in_EAX[9] = DAT_00583456;
  in_EAX[0x68] = DAT_00583457;
  in_EAX[0x69] = DAT_00583458;
  in_EAX[0x6a] = DAT_00583459;
  in_EAX[0x6b] = DAT_0058345a;
  in_EAX[0x6c] = DAT_0058345b;
  in_EAX[0x6d] = DAT_0058345c;
  in_EAX[0x6e] = DAT_0058345d;
  in_EAX[0x6f] = DAT_0058345e;
  in_EAX[0x70] = DAT_0058345f;
  in_EAX[0x71] = DAT_00583460;
  in_EAX[0x72] = DAT_00583461;
  in_EAX[0x73] = DAT_00583462;
  in_EAX[0x74] = DAT_00583463;
  in_EAX[0x75] = DAT_00583464;
  in_EAX[0x76] = DAT_00583465;
  in_EAX[0x77] = DAT_00583466;
  in_EAX[0x78] = DAT_00583467;
  in_EAX[0x79] = DAT_00583468;
  in_EAX[0x7a] = DAT_00583469;
  in_EAX[0x7b] = DAT_0058346a;
  in_EAX[0x10] = DAT_0058346b;
  bVar1 = DAT_0058346d;
  *(ushort *)(in_EAX + 0x12) = (ushort)DAT_0058346d << 8;
  *(ushort *)(in_EAX + 0x12) = CONCAT11(bVar1,DAT_0058346c);
  bVar1 = DAT_0058346f;
  *(ushort *)(in_EAX + 0x14) = (ushort)DAT_0058346f << 8;
  *(ushort *)(in_EAX + 0x14) = CONCAT11(bVar1,DAT_0058346e);
  in_EAX[0x16] = DAT_00583470;
  bVar1 = DAT_00583472;
  *(ushort *)(in_EAX + 0x18) = (ushort)DAT_00583472 << 8;
  *(ushort *)(in_EAX + 0x18) = CONCAT11(bVar1,DAT_00583471);
  bVar1 = DAT_00583474;
  *(ushort *)(in_EAX + 0x1a) = (ushort)DAT_00583474 << 8;
  *(ushort *)(in_EAX + 0x1a) = CONCAT11(bVar1,DAT_00583473);
  in_EAX[0x1c] = DAT_00583475;
  bVar1 = DAT_00583477;
  *(ushort *)(in_EAX + 0x1e) = (ushort)DAT_00583477 << 8;
  *(ushort *)(in_EAX + 0x1e) = CONCAT11(bVar1,DAT_00583476);
  bVar1 = DAT_00583479;
  *(ushort *)(in_EAX + 0x20) = (ushort)DAT_00583479 << 8;
  *(ushort *)(in_EAX + 0x20) = CONCAT11(bVar1,DAT_00583478);
  in_EAX[0x22] = DAT_0058347a;
  bVar1 = DAT_0058347c;
  *(ushort *)(in_EAX + 0x24) = (ushort)DAT_0058347c << 8;
  *(ushort *)(in_EAX + 0x24) = CONCAT11(bVar1,DAT_0058347b);
  bVar1 = DAT_0058347e;
  *(ushort *)(in_EAX + 0x26) = (ushort)DAT_0058347e << 8;
  *(ushort *)(in_EAX + 0x26) = CONCAT11(bVar1,DAT_0058347d);
  in_EAX[0x28] = DAT_0058347f;
  in_EAX[0x29] = DAT_00583480;
  bVar1 = DAT_00583482;
  *(ushort *)(in_EAX + 0x2a) = (ushort)DAT_00583482 << 8;
  *(ushort *)(in_EAX + 0x2a) = CONCAT11(bVar1,DAT_00583481);
  bVar1 = DAT_00583484;
  *(ushort *)(in_EAX + 0x2c) = (ushort)DAT_00583484 << 8;
  *(ushort *)(in_EAX + 0x2c) = CONCAT11(bVar1,DAT_00583483);
  in_EAX[0x2e] = DAT_00583485;
  in_EAX[0x30] = DAT_00583486;
  in_EAX[0x31] = DAT_00583487;
  bVar1 = DAT_00583489;
  *(ushort *)(in_EAX + 0x32) = (ushort)DAT_00583489 << 8;
  *(ushort *)(in_EAX + 0x32) = CONCAT11(bVar1,DAT_00583488);
  bVar1 = DAT_0058348b;
  *(ushort *)(in_EAX + 0x34) = (ushort)DAT_0058348b << 8;
  *(ushort *)(in_EAX + 0x34) = CONCAT11(bVar1,DAT_0058348a);
  in_EAX[0x36] = DAT_0058348c;
  in_EAX[0x38] = DAT_0058348d;
  in_EAX[0x39] = DAT_0058348e;
  bVar1 = DAT_00583490;
  *(ushort *)(in_EAX + 0x3a) = (ushort)DAT_00583490 << 8;
  *(ushort *)(in_EAX + 0x3a) = CONCAT11(bVar1,DAT_0058348f);
  bVar1 = DAT_00583492;
  *(ushort *)(in_EAX + 0x3c) = (ushort)DAT_00583492 << 8;
  *(ushort *)(in_EAX + 0x3c) = CONCAT11(bVar1,DAT_00583491);
  in_EAX[0x3e] = DAT_00583493;
  in_EAX[0x40] = DAT_00583494;
  in_EAX[0x41] = DAT_00583495;
  bVar1 = DAT_00583497;
  *(ushort *)(in_EAX + 0x42) = (ushort)DAT_00583497 << 8;
  *(ushort *)(in_EAX + 0x42) = CONCAT11(bVar1,DAT_00583496);
  bVar1 = DAT_00583499;
  *(ushort *)(in_EAX + 0x44) = (ushort)DAT_00583499 << 8;
  *(ushort *)(in_EAX + 0x44) = CONCAT11(bVar1,DAT_00583498);
  in_EAX[0x46] = DAT_0058349a;
  in_EAX[0x48] = DAT_0058349b;
  in_EAX[0x49] = DAT_0058349c;
  bVar1 = DAT_0058349e;
  *(ushort *)(in_EAX + 0x4a) = (ushort)DAT_0058349e << 8;
  *(ushort *)(in_EAX + 0x4a) = CONCAT11(bVar1,DAT_0058349d);
  bVar1 = DAT_005834a0;
  *(ushort *)(in_EAX + 0x4c) = (ushort)DAT_005834a0 << 8;
  *(ushort *)(in_EAX + 0x4c) = CONCAT11(bVar1,DAT_0058349f);
  in_EAX[0x4e] = DAT_005834a1;
  in_EAX[0x50] = DAT_005834a2;
  in_EAX[0x51] = DAT_005834a3;
  bVar1 = DAT_005834a5;
  *(ushort *)(in_EAX + 0x52) = (ushort)DAT_005834a5 << 8;
  *(ushort *)(in_EAX + 0x52) = CONCAT11(bVar1,DAT_005834a4);
  bVar1 = DAT_005834a7;
  *(ushort *)(in_EAX + 0x54) = (ushort)DAT_005834a7 << 8;
  *(ushort *)(in_EAX + 0x54) = CONCAT11(bVar1,DAT_005834a6);
  in_EAX[0x56] = DAT_005834a8;
  in_EAX[0x58] = DAT_005834a9;
  in_EAX[0x59] = DAT_005834aa;
  bVar1 = DAT_005834ac;
  *(ushort *)(in_EAX + 0x5a) = (ushort)DAT_005834ac << 8;
  *(ushort *)(in_EAX + 0x5a) = CONCAT11(bVar1,DAT_005834ab);
  bVar1 = DAT_005834ae;
  *(ushort *)(in_EAX + 0x5c) = (ushort)DAT_005834ae << 8;
  *(ushort *)(in_EAX + 0x5c) = CONCAT11(bVar1,DAT_005834ad);
  in_EAX[0x5e] = DAT_005834af;
  in_EAX[0x60] = DAT_005834b0;
  in_EAX[0x61] = DAT_005834b1;
  bVar1 = DAT_005834b3;
  *(ushort *)(in_EAX + 0x62) = (ushort)DAT_005834b3 << 8;
  *(ushort *)(in_EAX + 0x62) = CONCAT11(bVar1,DAT_005834b2);
  bVar1 = DAT_005834b5;
  *(ushort *)(in_EAX + 100) = (ushort)DAT_005834b5 << 8;
  *(ushort *)(in_EAX + 100) = CONCAT11(bVar1,DAT_005834b4);
  in_EAX[0x66] = DAT_005834b6;
  return;
}


/* ============ FUN_00403ec0 @ 00403ec0  (req 00403ec0) ============ */
/* calls: */
/* called by: FUN_00416200@00416200 */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00403ec0(void)

{
  undefined1 *in_EAX;
  
  DAT_00584320 = *in_EAX;
  DAT_00584326 = in_EAX[6];
  DAT_00584325 = in_EAX[5];
  DAT_00584324 = in_EAX[4];
  DAT_00584327 = in_EAX[7];
  DAT_0058432a = in_EAX[10];
  DAT_0058432e = in_EAX[0xe];
  DAT_0058432f = in_EAX[0xf];
  DAT_0058432b = in_EAX[0xb];
  DAT_0058432c = in_EAX[0xc];
  DAT_0058432d = in_EAX[0xd];
  DAT_00584328 = in_EAX[8];
  DAT_00584329 = in_EAX[9];
  DAT_00584388 = in_EAX[0x68];
  DAT_00584389 = in_EAX[0x69];
  DAT_0058438a = in_EAX[0x6a];
  DAT_0058438b = in_EAX[0x6b];
  DAT_0058438c = in_EAX[0x6c];
  DAT_0058438d = in_EAX[0x6d];
  DAT_0058438e = in_EAX[0x6e];
  DAT_0058438f = in_EAX[0x6f];
  DAT_00584390 = in_EAX[0x70];
  DAT_00584391 = in_EAX[0x71];
  DAT_00584392 = in_EAX[0x72];
  DAT_00584393 = in_EAX[0x73];
  DAT_00584394 = in_EAX[0x74];
  DAT_00584395 = in_EAX[0x75];
  DAT_00584396 = in_EAX[0x76];
  DAT_00584397 = in_EAX[0x77];
  DAT_00584398 = in_EAX[0x78];
  DAT_00584399 = in_EAX[0x79];
  DAT_0058439a = in_EAX[0x7a];
  DAT_0058439b = in_EAX[0x7b];
  DAT_00584330 = in_EAX[0x10];
  _DAT_00584332 = *(undefined2 *)(in_EAX + 0x12);
  _DAT_00584334 = *(undefined2 *)(in_EAX + 0x14);
  DAT_00584336 = in_EAX[0x16];
  _DAT_00584338 = *(undefined2 *)(in_EAX + 0x18);
  _DAT_0058433a = *(undefined2 *)(in_EAX + 0x1a);
  DAT_0058433c = in_EAX[0x1c];
  _DAT_0058433e = *(undefined2 *)(in_EAX + 0x1e);
  _DAT_00584340 = *(undefined2 *)(in_EAX + 0x20);
  DAT_00584342 = in_EAX[0x22];
  _DAT_00584344 = *(undefined2 *)(in_EAX + 0x24);
  _DAT_00584346 = *(undefined2 *)(in_EAX + 0x26);
  DAT_00584348 = in_EAX[0x28];
  DAT_00584349 = in_EAX[0x29];
  _DAT_0058434a = *(undefined2 *)(in_EAX + 0x2a);
  _DAT_0058434c = *(undefined2 *)(in_EAX + 0x2c);
  DAT_0058434e = in_EAX[0x2e];
  DAT_00584350 = in_EAX[0x30];
  DAT_00584351 = in_EAX[0x31];
  _DAT_00584352 = *(undefined2 *)(in_EAX + 0x32);
  _DAT_00584354 = *(undefined2 *)(in_EAX + 0x34);
  DAT_00584356 = in_EAX[0x36];
  DAT_00584358 = in_EAX[0x38];
  DAT_00584359 = in_EAX[0x39];
  _DAT_0058435a = *(undefined2 *)(in_EAX + 0x3a);
  _DAT_0058435c = *(undefined2 *)(in_EAX + 0x3c);
  DAT_0058435e = in_EAX[0x3e];
  DAT_00584360 = in_EAX[0x40];
  DAT_00584361 = in_EAX[0x41];
  _DAT_00584362 = *(undefined2 *)(in_EAX + 0x42);
  _DAT_00584364 = *(undefined2 *)(in_EAX + 0x44);
  DAT_00584366 = in_EAX[0x46];
  DAT_00584368 = in_EAX[0x48];
  DAT_00584369 = in_EAX[0x49];
  _DAT_0058436a = *(undefined2 *)(in_EAX + 0x4a);
  _DAT_0058436c = *(undefined2 *)(in_EAX + 0x4c);
  DAT_0058436e = in_EAX[0x4e];
  DAT_00584370 = in_EAX[0x50];
  DAT_00584371 = in_EAX[0x51];
  _DAT_00584372 = *(undefined2 *)(in_EAX + 0x52);
  _DAT_00584374 = *(undefined2 *)(in_EAX + 0x54);
  DAT_00584376 = in_EAX[0x56];
  DAT_00584378 = in_EAX[0x58];
  DAT_00584379 = in_EAX[0x59];
  _DAT_0058437a = *(undefined2 *)(in_EAX + 0x5a);
  _DAT_0058437c = *(undefined2 *)(in_EAX + 0x5c);
  DAT_0058437e = in_EAX[0x5e];
  DAT_00584380 = in_EAX[0x60];
  DAT_00584381 = in_EAX[0x61];
  _DAT_00584382 = *(undefined2 *)(in_EAX + 0x62);
  _DAT_00584384 = *(undefined2 *)(in_EAX + 100);
  DAT_00584386 = in_EAX[0x66];
  return;
}


/* ============ FUN_00404dc0 @ 00404dc0  (req 00404dc0) ============ */
/* calls: */
/* called by: FUN_00416590@00416590 */

void __fastcall FUN_00404dc0(undefined1 *param_1)

{
  undefined1 *in_EAX;
  
  *in_EAX = *param_1;
  in_EAX[3] = param_1[6];
  in_EAX[4] = param_1[5];
  in_EAX[5] = param_1[4];
  in_EAX[6] = param_1[7];
  in_EAX[7] = param_1[10];
  in_EAX[8] = param_1[0xe];
  in_EAX[9] = param_1[0xf];
  in_EAX[10] = param_1[0xb];
  in_EAX[0xb] = param_1[0xc];
  in_EAX[0xc] = param_1[0xd];
  in_EAX[0xd] = param_1[8];
  in_EAX[0xe] = param_1[9];
  in_EAX[0xf] = param_1[0x68];
  in_EAX[0x10] = param_1[0x69];
  in_EAX[0x11] = param_1[0x6a];
  in_EAX[0x12] = param_1[0x6b];
  in_EAX[0x13] = param_1[0x6c];
  in_EAX[0x14] = param_1[0x6d];
  in_EAX[0x15] = param_1[0x6e];
  in_EAX[0x16] = param_1[0x6f];
  in_EAX[0x17] = param_1[0x70];
  in_EAX[0x18] = param_1[0x71];
  in_EAX[0x19] = param_1[0x72];
  in_EAX[0x1a] = param_1[0x73];
  in_EAX[0x1b] = param_1[0x74];
  in_EAX[0x1c] = param_1[0x75];
  in_EAX[0x1d] = param_1[0x76];
  in_EAX[0x1e] = param_1[0x77];
  in_EAX[0x1f] = param_1[0x78];
  in_EAX[0x20] = param_1[0x79];
  in_EAX[0x21] = param_1[0x7a];
  in_EAX[0x22] = param_1[0x7b];
  in_EAX[0x23] = param_1[0x10];
  in_EAX[0x24] = param_1[0x12];
  in_EAX[0x25] = param_1[0x13];
  in_EAX[0x26] = param_1[0x14];
  in_EAX[0x27] = param_1[0x15];
  in_EAX[0x28] = param_1[0x16];
  in_EAX[0x29] = param_1[0x18];
  in_EAX[0x2a] = param_1[0x19];
  in_EAX[0x2b] = param_1[0x1a];
  in_EAX[0x2c] = param_1[0x1b];
  in_EAX[0x2d] = param_1[0x1c];
  in_EAX[0x2e] = param_1[0x1e];
  in_EAX[0x2f] = param_1[0x1f];
  in_EAX[0x30] = param_1[0x20];
  in_EAX[0x31] = param_1[0x21];
  in_EAX[0x32] = param_1[0x22];
  in_EAX[0x33] = param_1[0x24];
  in_EAX[0x34] = param_1[0x25];
  in_EAX[0x35] = param_1[0x26];
  in_EAX[0x36] = param_1[0x27];
  in_EAX[0x37] = param_1[0x28];
  in_EAX[0x38] = param_1[0x29];
  in_EAX[0x39] = param_1[0x2a];
  in_EAX[0x3a] = param_1[0x2b];
  in_EAX[0x3b] = param_1[0x2c];
  in_EAX[0x3c] = param_1[0x2d];
  in_EAX[0x3d] = param_1[0x2e];
  in_EAX[0x3e] = param_1[0x30];
  in_EAX[0x3f] = param_1[0x31];
  in_EAX[0x40] = param_1[0x32];
  in_EAX[0x41] = param_1[0x33];
  in_EAX[0x42] = param_1[0x34];
  in_EAX[0x43] = param_1[0x35];
  in_EAX[0x44] = param_1[0x36];
  in_EAX[0x45] = param_1[0x38];
  in_EAX[0x46] = param_1[0x39];
  in_EAX[0x47] = param_1[0x3a];
  in_EAX[0x48] = param_1[0x3b];
  in_EAX[0x49] = param_1[0x3c];
  in_EAX[0x4a] = param_1[0x3d];
  in_EAX[0x4b] = param_1[0x3e];
  in_EAX[0x4c] = param_1[0x40];
  in_EAX[0x4d] = param_1[0x41];
  in_EAX[0x4e] = param_1[0x42];
  in_EAX[0x4f] = param_1[0x43];
  in_EAX[0x50] = param_1[0x44];
  in_EAX[0x51] = param_1[0x45];
  in_EAX[0x52] = param_1[0x46];
  in_EAX[0x53] = param_1[0x48];
  in_EAX[0x54] = param_1[0x49];
  in_EAX[0x55] = param_1[0x4a];
  in_EAX[0x56] = param_1[0x4b];
  in_EAX[0x57] = param_1[0x4c];
  in_EAX[0x58] = param_1[0x4d];
  in_EAX[0x59] = param_1[0x4e];
  in_EAX[0x5a] = param_1[0x50];
  in_EAX[0x5b] = param_1[0x51];
  in_EAX[0x5c] = param_1[0x52];
  in_EAX[0x5d] = param_1[0x53];
  in_EAX[0x5e] = param_1[0x54];
  in_EAX[0x5f] = param_1[0x55];
  in_EAX[0x60] = param_1[0x56];
  in_EAX[0x61] = param_1[0x58];
  in_EAX[0x62] = param_1[0x59];
  in_EAX[99] = param_1[0x5a];
  in_EAX[100] = param_1[0x5b];
  in_EAX[0x65] = param_1[0x5c];
  in_EAX[0x66] = param_1[0x5d];
  in_EAX[0x67] = param_1[0x5e];
  in_EAX[0x68] = param_1[0x60];
  in_EAX[0x69] = param_1[0x61];
  in_EAX[0x6a] = param_1[0x62];
  in_EAX[0x6b] = param_1[99];
  in_EAX[0x6c] = param_1[100];
  in_EAX[0x6d] = param_1[0x65];
  in_EAX[0x6e] = param_1[0x66];
  return;
}


/* ============ FUN_00416120 @ 00416120  (req 416120) ============ */
/* calls: */
/* called by: FUN_004147b0@004147b0 FUN_00416360@00416360 */

void FUN_00416120(void)

{
  undefined1 *in_EAX;
  
  *(undefined2 *)(in_EAX + 0x12) = 400;
  *(undefined2 *)(in_EAX + 0x14) = 400;
  *(undefined2 *)(in_EAX + 0x18) = 800;
  *(undefined2 *)(in_EAX + 0x1a) = 800;
  *(undefined2 *)(in_EAX + 0x1e) = 0x640;
  *(undefined2 *)(in_EAX + 0x20) = 0x640;
  *(undefined2 *)(in_EAX + 0x24) = 0xc80;
  *in_EAX = 0;
  in_EAX[0x10] = 0;
  in_EAX[0x16] = 0;
  in_EAX[0x1c] = 0;
  in_EAX[0x22] = 0;
  *(undefined2 *)(in_EAX + 0x2a) = 0;
  *(undefined2 *)(in_EAX + 0x2c) = 0;
  *(undefined2 *)(in_EAX + 0x34) = 0;
  *(undefined2 *)(in_EAX + 0x3c) = 0;
  *(undefined2 *)(in_EAX + 0x44) = 0;
  *(undefined4 *)(in_EAX + 2) = 0x1020080;
  *(undefined2 *)(in_EAX + 0xe) = 0x101;
  *(undefined4 *)(in_EAX + 6) = 0x4021103;
  *(undefined4 *)(in_EAX + 10) = 0;
  *(undefined4 *)(in_EAX + 0x26) = 0x1000c80;
  in_EAX[0x2e] = 8;
  *(undefined4 *)(in_EAX + 0x30) = 0x200;
  in_EAX[0x36] = 8;
  *(undefined4 *)(in_EAX + 0x38) = 0x400;
  in_EAX[0x3e] = 8;
  *(undefined4 *)(in_EAX + 0x40) = 0x800;
  in_EAX[0x46] = 8;
  *(undefined4 *)(in_EAX + 0x48) = 0x1000;
  *(undefined2 *)(in_EAX + 0x4c) = 0;
  in_EAX[0x4e] = 8;
  *(undefined4 *)(in_EAX + 0x50) = 0xf109;
  *(undefined2 *)(in_EAX + 0x54) = 0;
  in_EAX[0x56] = 8;
  *(undefined4 *)(in_EAX + 0x58) = 0x101;
  *(undefined2 *)(in_EAX + 0x5c) = 0;
  in_EAX[0x5e] = 8;
  *(undefined4 *)(in_EAX + 0x60) = 0xff01;
  *(undefined2 *)(in_EAX + 100) = 0;
  in_EAX[0x66] = 8;
  return;
}



/* no function at 4097 */


/* ---- disassembly context: where the blob and struct bases come from ----

==== 00403e70  (FUN_00403d90 @ 00403d90) ====
00403e71  eb 29                    JMP 0x00403e9c
00403e73  b9 00 01 00 00           MOV ECX,0x100
00403e78  8d b5 a4 fb ff ff        LEA ESI,[EBP + 0xfffffba4]
00403e7e  bf 48 34 58 00           MOV EDI,0x583448
00403e83  b8 50 38 58 00           MOV EAX,0x583850
00403e88  f3 a5                    MOVSD.REP ES:EDI,ESI
00403e8a  e8 a1 03 00 00           CALL 0x00404230
00403e8f  68 e0 cc 57 00           PUSH 0x57cce0
00403e94  ff 15 74 02 53 00        CALL dword ptr [0x00530274]
00403e9a  b0 01                    MOV AL,0x1
00403e9c  8b 4d f4                 MOV ECX,dword ptr [EBP + -0xc]
00403e9f  64 89 0d 00 00 00 00     MOV dword ptr FS:[0x0],ECX
00403ea6  59                       POP ECX
00403ea7  5f                       POP EDI

==== 00414c90  (FUN_004149a0 @ 004149a0) ====
00414c93  6a 00                    PUSH 0x0
00414c95  8d 8e f8 2b 00 00        LEA ECX,[ESI + 0x2bf8]
00414c9b  e8 c5 90 00 00           CALL 0x0041dd65
00414ca0  b8 20 43 58 00           MOV EAX,0x584320
00414ca5  e8 86 f5 fe ff           CALL 0x00404230
00414caa  8b fe                    MOV EDI,ESI
00414cac  e8 9f 07 00 00           CALL 0x00415450
00414cb1  8b 7d d0                 MOV EDI,dword ptr [EBP + -0x30]
00414cb4  8d 47 f0                 LEA EAX,[EDI + -0x10]
00414cb7  c7 45 fc ff ff ff ff     MOV dword ptr [EBP + -0x4],0xffffffff
00414cbe  8d 48 0c                 LEA ECX,[EAX + 0xc]
00414cc1  83 ca ff                 OR EDX,0xffffffff

==== 00414e90  (FUN_004149a0 @ 004149a0) ====
00414e94  e8 cc 8e 00 00           CALL 0x0041dd65
00414e99  b8 20 43 58 00           MOV EAX,0x584320
00414e9e  e8 8d f3 fe ff           CALL 0x00404230
00414ea3  8b fe                    MOV EDI,ESI
00414ea5  e8 a6 05 00 00           CALL 0x00415450
00414eaa  0f b7 05 86 44 58 00     MOVZX EAX,word ptr [0x00584486]
00414eb1  0f b7 0d 84 44 58 00     MOVZX ECX,word ptr [0x00584484]
00414eb8  50                       PUSH EAX
00414eb9  51                       PUSH ECX
00414eba  e8 e1 e6 fe ff           CALL 0x004035a0

==== 0041640a  (FUN_00416360 @ 00416360) ====
0041640e  b8 20 43 58 00           MOV EAX,0x584320
00416413  e8 08 fd ff ff           CALL 0x00416120
00416418  8d b7 a8 16 00 00        LEA ESI,[EDI + 0x16a8]
0041641e  e8 8d ae ff ff           CALL 0x004112b0
00416423  8d 87 c8 00 00 00        LEA EAX,[EDI + 0xc8]
00416429  e8 e2 95 ff ff           CALL 0x0040fa10
0041642e  8d b7 ac 1e 00 00        LEA ESI,[EDI + 0x1eac]
00416434  e8 a7 33 ff ff           CALL 0x004097e0

==== 004148b4  (FUN_004147b0 @ 004147b0) ====
004148b5  e8 66 18 00 00           CALL 0x00416120
004148ba  b8 20 43 58 00           MOV EAX,0x584320
004148bf  e8 5c 18 00 00           CALL 0x00416120
004148c4  e8 67 1c 00 00           CALL 0x00416530
004148c9  8a d8                    MOV BL,AL
004148cb  e8 e0 f9 ff ff           CALL 0x004142b0
004148d0  80 fb 11                 CMP BL,0x11
004148d3  75 1b                    JNZ 0x004148f0

==== 004165e4  (FUN_00416590 @ 00416590) ====
004165e9  b9 20 43 58 00           MOV ECX,0x584320
004165ee  e8 cd e7 fe ff           CALL 0x00404dc0
004165f3  33 f6                    XOR ESI,ESI
004165f5  eb 09                    JMP 0x00416600
00416600  8a 54 35 88              MOV DL,byte ptr [EBP + ESI*0x1 + -0x78]
00416604  3a 94 35 14 ff ff ff     CMP DL,byte ptr [EBP + ESI*0x1 + 0xffffff14]
0041660b  75 28                    JNZ 0x00416635
0041660d  83 fe 71                 CMP ESI,0x71

*/


