/* v1/serializers.c -- the payload gather functions ("serializers").
 *
 * Program: Endgame_Gear_OP1w_4k_Configuration_Tool_v1.03.exe
 *   (OP1w 4k **v1** vendor tool; Ghidra project OP1w, image base 0x00400000)
 * Extracted with Ghidra 12.1.4 headless, scripts in git/re/ghidra_scripts.
 * Evidence class for everything in this file: [BIN].
 *
 * Addresses covered:
 *   FUN_00404d00   cmd 0x14 serializer  -- v2 counterpart FUN_00404ce0
 *   FUN_00404a20   cmd 0x16 serializer  -- button table, 0x38 bytes
 *   (cmd 0x15 has no separate serializer; it is gathered inline inside
 *    FUN_00404bb0, see v1/protocol_callers.c)
 *
 * Calling convention: Ghidra recovers these as __fastcall with the SOURCE in
 * ECX/param_1 and the DESTINATION in EAX ("in_EAX"). The raw disassembly at the
 * bottom of this file confirms EAX = payload, ECX = struct.
 *
 * ================= cmd 0x14, the field that matters =================
 *
 * FUN_00404750 calls FUN_00404d00 with ECX = 0x00583858, which is
 * settings-struct base 0x00583850 PLUS 8.  So "struct offset" below means
 * offset from 0x00583858; add 8 to get the offset within the shared settings
 * struct that the blob parser fills (v1/blob_parser.c).
 *
 *   payload  struct   settings   blob    meaning (from PROTOCOL.md S4)
 *   -------  ------   --------   ----    ----------------------------
 *   +0       +0x02    +0x0A      0x07    always 0x00 on the wire
 *   +1       +0x06    +0x0E      0x08    LED on lift-off
 *   +2       +0x07    +0x0F      0x09    lift-off distance  (v1: 1 or 2 mm)
 *   +3       +0x03    +0x0B      0x0A    angle snapping
 *   +4       +0x04    +0x0C      0x0B    ripple control
 *   +5       -- NEVER WRITTEN --         sensor angle tuning (v2 only)
 *   +6       +0x01    +0x09      0x0E    CPI levels
 *   +7       +0x00    +0x08      0x0D    ACTIVE CPI STAGE
 *   +8..+0x1B  four 5-byte CPI records, struct stride 6, from struct +0x08
 *              (settings +0x10): flag u8, X u16le, Y u16le
 *
 * The absence of any store to payload +5 is the [BIN] proof that the v1 tool
 * never writes sensor angle tuning (PROTOCOL.md S12).
 *
 * The same composition applied to the v2 tool (struct base = settings + 0x0A
 * there) reproduces PROTOCOL.md S4's blob offsets exactly, which is the
 * cross-check that this settings/blob mapping is right.
 */

/* ============ FUN_00404d00 @ 00404d00  (req 00404d00) ============ */
/* calls: */
/* called by: FUN_00404750@00404750 */

void __fastcall FUN_00404d00(undefined1 *param_1)

{
  undefined1 *in_EAX;
  
  *in_EAX = param_1[2];
  in_EAX[1] = param_1[6];
  in_EAX[2] = param_1[7];
  in_EAX[3] = param_1[3];
  in_EAX[4] = param_1[4];
  in_EAX[6] = param_1[1];
  in_EAX[7] = *param_1;
  in_EAX[8] = param_1[8];
  in_EAX[9] = param_1[10];
  in_EAX[10] = param_1[0xb];
  in_EAX[0xb] = param_1[0xc];
  in_EAX[0xc] = param_1[0xd];
  in_EAX[0xd] = param_1[0xe];
  in_EAX[0xe] = param_1[0x10];
  in_EAX[0xf] = param_1[0x11];
  in_EAX[0x10] = param_1[0x12];
  in_EAX[0x11] = param_1[0x13];
  in_EAX[0x12] = param_1[0x14];
  in_EAX[0x13] = param_1[0x16];
  in_EAX[0x14] = param_1[0x17];
  in_EAX[0x15] = param_1[0x18];
  in_EAX[0x16] = param_1[0x19];
  in_EAX[0x17] = param_1[0x1a];
  in_EAX[0x18] = param_1[0x1c];
  in_EAX[0x19] = param_1[0x1d];
  in_EAX[0x1a] = param_1[0x1e];
  in_EAX[0x1b] = param_1[0x1f];
  return;
}


/* ============ FUN_00404a20 @ 00404a20  (req 00404a20) ============ */
/* calls: */
/* called by: FUN_00404890@00404890 */

void __fastcall FUN_00404a20(int param_1)

{
  undefined1 *in_EAX;
  
  *in_EAX = *(undefined1 *)(param_1 + 0x28);
  in_EAX[1] = *(undefined1 *)(param_1 + 0x29);
  in_EAX[2] = *(undefined1 *)(param_1 + 0x2a);
  in_EAX[3] = *(undefined1 *)(param_1 + 0x2b);
  in_EAX[4] = *(undefined1 *)(param_1 + 0x2c);
  in_EAX[5] = *(undefined1 *)(param_1 + 0x2d);
  in_EAX[6] = *(undefined1 *)(param_1 + 0x2e);
  in_EAX[7] = *(undefined1 *)(param_1 + 0x30);
  in_EAX[8] = *(undefined1 *)(param_1 + 0x31);
  in_EAX[9] = *(undefined1 *)(param_1 + 0x32);
  in_EAX[10] = *(undefined1 *)(param_1 + 0x33);
  in_EAX[0xb] = *(undefined1 *)(param_1 + 0x34);
  in_EAX[0xc] = *(undefined1 *)(param_1 + 0x35);
  in_EAX[0xd] = *(undefined1 *)(param_1 + 0x36);
  in_EAX[0xe] = *(undefined1 *)(param_1 + 0x38);
  in_EAX[0xf] = *(undefined1 *)(param_1 + 0x39);
  in_EAX[0x10] = *(undefined1 *)(param_1 + 0x3a);
  in_EAX[0x11] = *(undefined1 *)(param_1 + 0x3b);
  in_EAX[0x12] = *(undefined1 *)(param_1 + 0x3c);
  in_EAX[0x13] = *(undefined1 *)(param_1 + 0x3d);
  in_EAX[0x14] = *(undefined1 *)(param_1 + 0x3e);
  in_EAX[0x15] = *(undefined1 *)(param_1 + 0x40);
  in_EAX[0x16] = *(undefined1 *)(param_1 + 0x41);
  in_EAX[0x17] = *(undefined1 *)(param_1 + 0x42);
  in_EAX[0x18] = *(undefined1 *)(param_1 + 0x43);
  in_EAX[0x19] = *(undefined1 *)(param_1 + 0x44);
  in_EAX[0x1a] = *(undefined1 *)(param_1 + 0x45);
  in_EAX[0x1b] = *(undefined1 *)(param_1 + 0x46);
  in_EAX[0x1c] = *(undefined1 *)(param_1 + 0x48);
  in_EAX[0x1d] = *(undefined1 *)(param_1 + 0x49);
  in_EAX[0x1e] = *(undefined1 *)(param_1 + 0x4a);
  in_EAX[0x1f] = *(undefined1 *)(param_1 + 0x4b);
  in_EAX[0x20] = *(undefined1 *)(param_1 + 0x4c);
  in_EAX[0x21] = *(undefined1 *)(param_1 + 0x4d);
  in_EAX[0x22] = *(undefined1 *)(param_1 + 0x4e);
  in_EAX[0x23] = *(undefined1 *)(param_1 + 0x50);
  in_EAX[0x24] = *(undefined1 *)(param_1 + 0x51);
  in_EAX[0x25] = *(undefined1 *)(param_1 + 0x52);
  in_EAX[0x26] = *(undefined1 *)(param_1 + 0x53);
  in_EAX[0x27] = *(undefined1 *)(param_1 + 0x54);
  in_EAX[0x28] = *(undefined1 *)(param_1 + 0x55);
  in_EAX[0x29] = *(undefined1 *)(param_1 + 0x56);
  in_EAX[0x2a] = *(undefined1 *)(param_1 + 0x58);
  in_EAX[0x2b] = *(undefined1 *)(param_1 + 0x59);
  in_EAX[0x2c] = *(undefined1 *)(param_1 + 0x5a);
  in_EAX[0x2d] = *(undefined1 *)(param_1 + 0x5b);
  in_EAX[0x2e] = *(undefined1 *)(param_1 + 0x5c);
  in_EAX[0x2f] = *(undefined1 *)(param_1 + 0x5d);
  in_EAX[0x30] = *(undefined1 *)(param_1 + 0x5e);
  in_EAX[0x31] = *(undefined1 *)(param_1 + 0x60);
  in_EAX[0x32] = *(undefined1 *)(param_1 + 0x61);
  in_EAX[0x33] = *(undefined1 *)(param_1 + 0x62);
  in_EAX[0x34] = *(undefined1 *)(param_1 + 99);
  in_EAX[0x35] = *(undefined1 *)(param_1 + 100);
  in_EAX[0x36] = *(undefined1 *)(param_1 + 0x65);
  in_EAX[0x37] = *(undefined1 *)(param_1 + 0x66);
  return;
}


/* ---- raw disassembly, proving EAX = payload and ECX = struct ----

==== 00404d00  (FUN_00404d00 @ 00404d00) ====
00404d00  0f b6 51 02              MOVZX EDX,byte ptr [ECX + 0x2]
00404d04  88 10                    MOV byte ptr [EAX],DL
00404d06  0f b6 51 06              MOVZX EDX,byte ptr [ECX + 0x6]
00404d0a  88 50 01                 MOV byte ptr [EAX + 0x1],DL
00404d0d  0f b6 51 07              MOVZX EDX,byte ptr [ECX + 0x7]
00404d11  88 50 02                 MOV byte ptr [EAX + 0x2],DL
00404d14  0f b6 51 03              MOVZX EDX,byte ptr [ECX + 0x3]
00404d18  88 50 03                 MOV byte ptr [EAX + 0x3],DL
00404d1b  0f b6 51 04              MOVZX EDX,byte ptr [ECX + 0x4]
00404d1f  88 50 04                 MOV byte ptr [EAX + 0x4],DL
00404d22  0f b6 51 01              MOVZX EDX,byte ptr [ECX + 0x1]
00404d26  88 50 06                 MOV byte ptr [EAX + 0x6],DL
00404d29  0f b6 11                 MOVZX EDX,byte ptr [ECX]
00404d2c  88 50 07                 MOV byte ptr [EAX + 0x7],DL
00404d2f  0f b6 51 08              MOVZX EDX,byte ptr [ECX + 0x8]
00404d33  88 50 08                 MOV byte ptr [EAX + 0x8],DL
00404d36  0f b6 51 0a              MOVZX EDX,byte ptr [ECX + 0xa]
00404d3a  88 50 09                 MOV byte ptr [EAX + 0x9],DL
00404d3d  0f b6 51 0b              MOVZX EDX,byte ptr [ECX + 0xb]
00404d41  88 50 0a                 MOV byte ptr [EAX + 0xa],DL
00404d44  0f b6 51 0c              MOVZX EDX,byte ptr [ECX + 0xc]
00404d48  88 50 0b                 MOV byte ptr [EAX + 0xb],DL
00404d4b  0f b6 51 0d              MOVZX EDX,byte ptr [ECX + 0xd]
00404d4f  88 50 0c                 MOV byte ptr [EAX + 0xc],DL
00404d52  0f b6 51 0e              MOVZX EDX,byte ptr [ECX + 0xe]
00404d56  88 50 0d                 MOV byte ptr [EAX + 0xd],DL
00404d59  0f b6 51 10              MOVZX EDX,byte ptr [ECX + 0x10]
00404d5d  88 50 0e                 MOV byte ptr [EAX + 0xe],DL
00404d60  0f b6 51 11              MOVZX EDX,byte ptr [ECX + 0x11]
00404d64  88 50 0f                 MOV byte ptr [EAX + 0xf],DL
00404d67  0f b6 51 12              MOVZX EDX,byte ptr [ECX + 0x12]
00404d6b  88 50 10                 MOV byte ptr [EAX + 0x10],DL
00404d6e  0f b6 51 13              MOVZX EDX,byte ptr [ECX + 0x13]
00404d72  88 50 11                 MOV byte ptr [EAX + 0x11],DL
00404d75  0f b6 51 14              MOVZX EDX,byte ptr [ECX + 0x14]
00404d79  88 50 12                 MOV byte ptr [EAX + 0x12],DL
00404d7c  0f b6 51 16              MOVZX EDX,byte ptr [ECX + 0x16]
00404d80  88 50 13                 MOV byte ptr [EAX + 0x13],DL
00404d83  0f b6 51 17              MOVZX EDX,byte ptr [ECX + 0x17]
00404d87  88 50 14                 MOV byte ptr [EAX + 0x14],DL
00404d8a  0f b6 51 18              MOVZX EDX,byte ptr [ECX + 0x18]
00404d8e  88 50 15                 MOV byte ptr [EAX + 0x15],DL
00404d91  0f b6 51 19              MOVZX EDX,byte ptr [ECX + 0x19]
00404d95  88 50 16                 MOV byte ptr [EAX + 0x16],DL
00404d98  0f b6 51 1a              MOVZX EDX,byte ptr [ECX + 0x1a]
00404d9c  88 50 17                 MOV byte ptr [EAX + 0x17],DL
00404d9f  0f b6 51 1c              MOVZX EDX,byte ptr [ECX + 0x1c]
00404da3  88 50 18                 MOV byte ptr [EAX + 0x18],DL
00404da6  0f b6 51 1d              MOVZX EDX,byte ptr [ECX + 0x1d]
00404daa  88 50 19                 MOV byte ptr [EAX + 0x19],DL
00404dad  0f b6 51 1e              MOVZX EDX,byte ptr [ECX + 0x1e]
00404db1  88 50 1a                 MOV byte ptr [EAX + 0x1a],DL
00404db4  8a 49 1f                 MOV CL,byte ptr [ECX + 0x1f]
00404db7  88 48 1b                 MOV byte ptr [EAX + 0x1b],CL
00404dba  c3                       RET

==== 00404230  (FUN_00404230 @ 00404230) ====
00404230  0f b6 0d 48 34 58 00     MOVZX ECX,byte ptr [0x00583448]
00404237  88 08                    MOV byte ptr [EAX],CL
00404239  0f b6 15 4b 34 58 00     MOVZX EDX,byte ptr [0x0058344b]
00404240  88 50 06                 MOV byte ptr [EAX + 0x6],DL
00404243  0f b6 0d 4c 34 58 00     MOVZX ECX,byte ptr [0x0058344c]
0040424a  88 48 05                 MOV byte ptr [EAX + 0x5],CL
0040424d  0f b6 15 4d 34 58 00     MOVZX EDX,byte ptr [0x0058344d]
00404254  88 50 04                 MOV byte ptr [EAX + 0x4],DL
00404257  0f b6 0d 4e 34 58 00     MOVZX ECX,byte ptr [0x0058344e]
0040425e  88 48 07                 MOV byte ptr [EAX + 0x7],CL
00404261  0f b6 15 4f 34 58 00     MOVZX EDX,byte ptr [0x0058344f]
00404268  88 50 0a                 MOV byte ptr [EAX + 0xa],DL
0040426b  0f b6 0d 50 34 58 00     MOVZX ECX,byte ptr [0x00583450]
00404272  88 48 0e                 MOV byte ptr [EAX + 0xe],CL
00404275  0f b6 15 51 34 58 00     MOVZX EDX,byte ptr [0x00583451]
0040427c  88 50 0f                 MOV byte ptr [EAX + 0xf],DL
0040427f  0f b6 0d 52 34 58 00     MOVZX ECX,byte ptr [0x00583452]
00404286  88 48 0b                 MOV byte ptr [EAX + 0xb],CL
00404289  0f b6 15 53 34 58 00     MOVZX EDX,byte ptr [0x00583453]
00404290  88 50 0c                 MOV byte ptr [EAX + 0xc],DL
00404293  0f b6 0d 54 34 58 00     MOVZX ECX,byte ptr [0x00583454]
0040429a  88 48 0d                 MOV byte ptr [EAX + 0xd],CL
0040429d  0f b6 15 55 34 58 00     MOVZX EDX,byte ptr [0x00583455]
004042a4  88 50 08                 MOV byte ptr [EAX + 0x8],DL

*/


