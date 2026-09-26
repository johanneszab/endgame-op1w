<#
.SYNOPSIS
  Read-only probe: send cmd 0x0E ("mouse info") on the OP1w config channel and
  dump the response payload.

.DESCRIPTION
  Used to test whether cmd 0x0E reports the paired mouse's own USB VID/PID,
  which would be the only known way to tell a v1 from a v2 over the shared
  dongle (both dongles enumerate as 3367:1970).

  Sends ONLY read commands (0x0F probe, 0x0E mouse info, 0x0D dongle info).
  Writes nothing to the device configuration.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File probe-mouseinfo.ps1
#>
param([int]$Vid = 0x3367)

$src = @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public static class HidIo
{
    [StructLayout(LayoutKind.Sequential)]
    public struct HIDD_ATTRIBUTES { public int Size; public ushort VendorID; public ushort ProductID; public ushort VersionNumber; }

    [StructLayout(LayoutKind.Sequential)]
    public struct HIDP_CAPS {
        public ushort Usage, UsagePage;
        public ushort InputReportByteLength, OutputReportByteLength, FeatureReportByteLength;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 17)] public ushort[] Reserved;
        public ushort NumberLinkCollectionNodes;
        public ushort NumberInputButtonCaps, NumberInputValueCaps, NumberInputDataIndices;
        public ushort NumberOutputButtonCaps, NumberOutputValueCaps, NumberOutputDataIndices;
        public ushort NumberFeatureButtonCaps, NumberFeatureValueCaps, NumberFeatureDataIndices;
    }

    [StructLayout(LayoutKind.Sequential)]
    struct SP_DEVICE_INTERFACE_DATA { public int cbSize; public Guid InterfaceClassGuid; public int Flags; public IntPtr Reserved; }

    [DllImport("hid.dll")] static extern void HidD_GetHidGuid(out Guid g);
    [DllImport("hid.dll")] static extern bool HidD_GetAttributes(IntPtr h, ref HIDD_ATTRIBUTES a);
    [DllImport("hid.dll")] static extern bool HidD_GetPreparsedData(IntPtr h, out IntPtr pp);
    [DllImport("hid.dll")] static extern bool HidD_FreePreparsedData(IntPtr pp);
    [DllImport("hid.dll")] static extern int  HidP_GetCaps(IntPtr pp, ref HIDP_CAPS c);
    [DllImport("hid.dll")] static extern bool HidD_SetFeature(IntPtr h, byte[] buf, int len);
    [DllImport("hid.dll")] static extern bool HidD_GetFeature(IntPtr h, byte[] buf, int len);

    [DllImport("setupapi.dll", CharSet = CharSet.Unicode)] static extern IntPtr SetupDiGetClassDevs(ref Guid g, IntPtr e, IntPtr h, int f);
    [DllImport("setupapi.dll")] static extern bool SetupDiEnumDeviceInterfaces(IntPtr s, IntPtr d, ref Guid g, int i, ref SP_DEVICE_INTERFACE_DATA da);
    [DllImport("setupapi.dll", CharSet = CharSet.Unicode)] static extern bool SetupDiGetDeviceInterfaceDetail(IntPtr s, ref SP_DEVICE_INTERFACE_DATA da, IntPtr detail, int sz, ref int req, IntPtr dd);
    [DllImport("setupapi.dll")] static extern bool SetupDiDestroyDeviceInfoList(IntPtr s);

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    static extern IntPtr CreateFile(string n, uint access, uint share, IntPtr sec, uint disp, uint flags, IntPtr tmpl);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")] static extern int GetLastError();

    const int DIGCF_PRESENT = 0x02, DIGCF_DEVICEINTERFACE = 0x10;
    const uint OPEN_EXISTING = 3, FILE_SHARE_RW = 3, GENERIC_RW = 0xC0000000;

    static List<string> Paths()
    {
        var list = new List<string>();
        Guid g; HidD_GetHidGuid(out g);
        IntPtr set = SetupDiGetClassDevs(ref g, IntPtr.Zero, IntPtr.Zero, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        var did = new SP_DEVICE_INTERFACE_DATA();
        did.cbSize = Marshal.SizeOf(did);
        for (int i = 0; SetupDiEnumDeviceInterfaces(set, IntPtr.Zero, ref g, i, ref did); i++)
        {
            int req = 0;
            SetupDiGetDeviceInterfaceDetail(set, ref did, IntPtr.Zero, 0, ref req, IntPtr.Zero);
            if (req <= 0) continue;
            IntPtr buf = Marshal.AllocHGlobal(req);
            Marshal.WriteInt32(buf, IntPtr.Size == 8 ? 8 : 6);
            if (SetupDiGetDeviceInterfaceDetail(set, ref did, buf, req, ref req, IntPtr.Zero))
                list.Add(Marshal.PtrToStringUni(new IntPtr(buf.ToInt64() + 4)));
            Marshal.FreeHGlobal(buf);
        }
        SetupDiDestroyDeviceInfoList(set);
        return list;
    }

    // Finds the 0xFF01 / 0x02 vendor collection and returns an open handle.
    public static IntPtr OpenConfigChannel(int wantVid, out string path)
    {
        path = "";
        foreach (var p in Paths())
        {
            IntPtr h = CreateFile(p, GENERIC_RW, FILE_SHARE_RW, IntPtr.Zero, OPEN_EXISTING, 0, IntPtr.Zero);
            if (h.ToInt64() == -1) continue;
            bool keep = false;
            try
            {
                var a = new HIDD_ATTRIBUTES(); a.Size = Marshal.SizeOf(a);
                if (!HidD_GetAttributes(h, ref a) || a.VendorID != wantVid) continue;
                IntPtr pp;
                if (!HidD_GetPreparsedData(h, out pp)) continue;
                try
                {
                    var c = new HIDP_CAPS();
                    if (HidP_GetCaps(pp, ref c) == 0x00110000 &&
                        (ushort)c.UsagePage == 0xFF01 && c.Usage == 0x0002)
                    {
                        keep = true; path = p;
                        return h;
                    }
                }
                finally { HidD_FreePreparsedData(pp); }
            }
            finally { if (!keep) CloseHandle(h); }
        }
        return IntPtr.Zero;
    }

    // One command/response exchange on the 64-byte feature report 0xA1.
    public static byte[] Command(IntPtr h, byte cmd, byte target)
    {
        var tx = new byte[64];
        tx[0] = 0xA1; tx[1] = cmd; tx[2] = target; tx[3] = 0x00;
        if (!HidD_SetFeature(h, tx, tx.Length))
            throw new Exception("SetFeature failed, GetLastError=" + GetLastError());

        System.Threading.Thread.Sleep(200);

        var rx = new byte[64];
        rx[0] = 0xA1;
        if (!HidD_GetFeature(h, rx, rx.Length))
            throw new Exception("GetFeature failed, GetLastError=" + GetLastError());
        return rx;
    }

    public static void Close(IntPtr h) { CloseHandle(h); }
}
'@

Add-Type -TypeDefinition $src -Language CSharp | Out-Null

$path = ''
$h = [HidIo]::OpenConfigChannel($Vid, [ref]$path)
if ($h -eq [IntPtr]::Zero) { throw "config channel (usage page 0xFF01 / usage 0x02) not found for VID 0x$('{0:X4}' -f $Vid)" }
Write-Output "channel: $path"
Write-Output ""

function Show([string]$name, [byte]$cmd, [byte]$target) {
    try {
        $r = [HidIo]::Command($h, $cmd, $target)
    } catch {
        Write-Output ("{0,-22} ERROR {1}" -f $name, $_.Exception.Message)
        return
    }
    $status = $r[1]
    $payload = $r[16..31]
    $hex = ($payload | ForEach-Object { '{0:X2}' -f $_ }) -join ' '
    Write-Output ("{0,-22} status={1:X2}  payload[0..15]= {2}" -f $name, $status, $hex)

    if ($cmd -eq 0x0E -or $cmd -eq 0x0D) {
        $a = [int]$payload[0] -bor ([int]$payload[1] -shl 8)
        $b = [int]$payload[2] -bor ([int]$payload[3] -shl 8)
        $c = [int]$payload[4] -bor ([int]$payload[5] -shl 8)
        Write-Output ("{0,-22}   as u16le: +0={1}  +2={2}  +4={3}   fw(+6,+7)={4}.{5:d2}" -f `
            '', ('0x{0:X4}' -f $a), ('0x{0:X4}' -f $b), ('0x{0:X4}' -f $c), $payload[6], $payload[7])
    }
}

try {
    Show 'probe 0x0F (dongle)' 0x0F 0x01
    Show 'cmd 0x0D dongle info' 0x0D 0x00
    Show 'probe 0x0F (dongle)' 0x0F 0x01
    Show 'cmd 0x0E mouse info'  0x0E 0x00
    Show 'probe 0x0F (dongle)' 0x0F 0x01
    Show 'cmd 0xB4 battery'     0xB4 0x00
} finally {
    [HidIo]::Close($h)
}
