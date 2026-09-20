<#
.SYNOPSIS
  Decode USBPcap captures of the OP1w config channel, without depending on
  tshark field names.

.DESCRIPTION
  Parses the pcap and the USBPcap pseudo-header directly and prints one line per
  HID feature-report control transfer:

      SET cmd=0F sub=01 len=00 idx=00 | <payload bytes 16..47>
      GET status=01                   | <payload bytes 16..47>

  Accepts a single .pcap or a directory (all *.pcap, in name order).

.EXAMPLE
  .\parse.ps1 ..\captures
  .\parse.ps1 ..\captures\18_LeftHandedModeOffToOn.pcap -Full
#>
param(
  [Parameter(Mandatory = $true)][string]$Path,
  [switch]$Full,       # print all 64 bytes instead of the payload window
  [switch]$Stages      # print USBPcap stage/endpoint detail (debugging)
)

$ErrorActionPreference = 'Stop'

function Convert-OneCapture {
  param([string]$File)

  $b = [System.IO.File]::ReadAllBytes($File)
  if ($b.Length -lt 24) { Write-Warning "$File too short"; return }

  $magic = [uint64][BitConverter]::ToUInt32($b, 0)
  if ($magic -ne 0xA1B2C3D4L -and $magic -ne 0xD4C3B2A1L -and $magic -ne 0xA1B23C4DL) {
    Write-Warning "$File is not a little-endian pcap (magic 0x$($magic.ToString('X8')))"
    return
  }

  '### {0}' -f (Split-Path $File -Leaf)

  $off = 24
  while ($off + 16 -le $b.Length) {
    $inclLen = [BitConverter]::ToUInt32($b, $off + 8)
    $p       = $off + 16
    $off     = $p + $inclLen
    if ($p + 28 -gt $b.Length) { break }

    $hdrLen   = [BitConverter]::ToUInt16($b, $p)
    $device   = [BitConverter]::ToUInt16($b, $p + 0x13)
    $endpoint = $b[$p + 0x15]
    $transfer = $b[$p + 0x16]
    $dataLen  = [BitConverter]::ToUInt32($b, $p + 0x17)
    if ($transfer -ne 2) { continue }                       # control transfers only

    $stage   = if ($hdrLen -ge 28) { $b[$p + 0x1B] } else { 255 }
    $dataOff = $p + $hdrLen
    if ($dataOff + $dataLen -gt $b.Length) { continue }

    if ($Stages) {
      '    [dev {0} ep {1:X2} stage {2} dlen {3}]' -f $device, $endpoint, $stage, $dataLen
    }

    $payload = $null
    $verb    = $null

    if ($stage -eq 0 -and $dataLen -ge 8) {
      # SETUP stage: 8-byte setup packet, OUT data (if any) follows in the same record
      $bReq   = $b[$dataOff + 1]
      $wValue = [BitConverter]::ToUInt16($b, $dataOff + 2)
      # remember what this control transfer asked for, so the IN data that comes
      # back on a later record can be attributed (string descriptors read during
      # enumeration are control IN too, and must not be mistaken for responses)
      $script:pendingGet = ($bReq -eq 1 -and (($wValue -shr 8) -band 0xFF) -eq 3)
      if ((($wValue -shr 8) -band 0xFF) -ne 3) { continue }  # feature reports only
      if ($bReq -eq 9 -and $dataLen -gt 8) {
        $verb    = 'SET'
        $payload = $b[($dataOff + 8)..($dataOff + $dataLen - 1)]
      }
      # a GET_REPORT setup stage carries no data; its response arrives on a later record
    }
    elseif ($dataLen -ge 8 -and ($endpoint -band 0x80) -and $script:pendingGet) {
      # IN data coming back from a GET_REPORT we saw the setup for
      $verb    = 'GET'
      $payload = $b[$dataOff..($dataOff + $dataLen - 1)]
      $script:pendingGet = $false
    }

    if (-not $payload -or $payload.Count -lt 4) { continue }

    $hex = ($payload | ForEach-Object { '{0:X2}' -f $_ })

    if ($verb -eq 'SET') {
      $ann = 'SET cmd={0} sub={1} len={2} idx={3}' -f $hex[1], $hex[2], $hex[3], $hex[6]
    } else {
      $ann = 'GET status={0}            ' -f $hex[1]
    }

    $window = if ($Full) { $hex } else { $hex[16..([Math]::Min(47, $hex.Count - 1))] }
    '  {0,-34} | {1}' -f $ann, ($window -join ' ')
  }
  ''
}

if (Test-Path $Path -PathType Container) {
  Get-ChildItem $Path -Filter *.pcap | Sort-Object Name | ForEach-Object {
    Convert-OneCapture -File $_.FullName
  }
} else {
  Convert-OneCapture -File (Resolve-Path $Path)
}
