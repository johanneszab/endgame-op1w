<#
.SYNOPSIS
  Extract device notifications (interrupt IN, report ID 0x03) from a capture,
  with timestamps, collapsing mouse-motion traffic into counts.

.EXAMPLE
  .\events.ps1 -Pcap ..\captures\42_OnOffOnOffCycle.pcap
#>
param(
  [Parameter(Mandatory = $true)][string]$Pcap,
  [int]$NotifyReportId = 3
)

$b = [System.IO.File]::ReadAllBytes($Pcap)
if ($b.Length -lt 24) { throw "not a pcap" }

$off   = 24
$t0    = $null
$motion = 0
$lastMotionT = 0.0
$rows  = New-Object System.Collections.Generic.List[string]

function Flush-Motion {
  if ($script:motion -gt 0) {
    $rows.Add(('  t=+{0,8:F3}  ....   {1} input report(s) from the mouse' -f $script:lastMotionT, $script:motion))
    $script:motion = 0
  }
}

while ($off + 16 -le $b.Length) {
  $tsSec  = [BitConverter]::ToUInt32($b, $off)
  $tsUsec = [BitConverter]::ToUInt32($b, $off + 4)
  $incl   = [BitConverter]::ToUInt32($b, $off + 8)
  $p      = $off + 16
  $off    = $p + $incl
  if ($p + 27 -gt $b.Length) { break }

  $t = [double]$tsSec + ([double]$tsUsec / 1e6)
  if ($null -eq $t0) { $t0 = $t }
  $rel = $t - $t0

  $hdrLen   = [BitConverter]::ToUInt16($b, $p)
  $endpoint = $b[$p + 0x15]
  $transfer = $b[$p + 0x16]
  $dataLen  = [BitConverter]::ToUInt32($b, $p + 0x17)
  $dataOff  = $p + $hdrLen
  if ($dataOff + $dataLen -gt $b.Length) { continue }

  if ($transfer -eq 1 -and $dataLen -gt 0) {
    $id = $b[$dataOff]
    if ($id -eq $NotifyReportId) {
      Flush-Motion
      $hex = @()
      for ($i = 0; $i -lt $dataLen; $i++) { $hex += '{0:X2}' -f $b[$dataOff + $i] }
      $rows.Add(('  t=+{0,8:F3}  EVENT  {1}' -f $rel, ($hex -join ' ')))
    }
    else {
      $script:motion++
      $script:lastMotionT = $rel
    }
  }
  elseif ($transfer -eq 2 -and $dataLen -ge 8 -and $hdrLen -ge 28 -and $b[$p + 0x1B] -eq 0) {
    $bReq   = $b[$dataOff + 1]
    $wValue = [BitConverter]::ToUInt16($b, $dataOff + 2)
    if ((($wValue -shr 8) -band 0xFF) -eq 3 -and $bReq -eq 9 -and $dataLen -gt 8) {
      Flush-Motion
      $rows.Add(('  t=+{0,8:F3}  CMD    A1 {1:X2} {2:X2} {3:X2}' -f $rel,
                 $b[$dataOff + 9], $b[$dataOff + 10], $b[$dataOff + 11]))
    }
  }
}
Flush-Motion

Write-Output ("### {0}   ({1:N0} bytes, {2:F1} s)" -f (Split-Path $Pcap -Leaf), $b.Length, ($rel))
$rows
