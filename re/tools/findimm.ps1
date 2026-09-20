<#
.SYNOPSIS
  Recover USB VID/PID pairs and protocol command headers from a 32-bit x86
  binary by scanning the raw bytes. No decompiler required.

.DESCRIPTION
  The vendor tools call a cdecl device-open helper as open(vid, pid), so the
  compiler emits the arguments right-to-left:

      68 <pid16> 00 00      push PID
      68 <vid16> 00 00      push VID
      E8 <rel32>            call

  Finding the VID push and reading the five bytes before it therefore yields
  the PID. Command headers appear as MOV imm32 and are matched as raw dwords.

.EXAMPLE
  .\findimm.ps1 -Path ..\..\OP1w4kv2\Endgame_Gear_OP1w_4k_v2_Configuration_Tool_v1_02-61846009.exe
#>
param(
  [Parameter(Mandatory = $true)][string]$Path,
  [int]$Vid = 0x3367
)

$b = [System.IO.File]::ReadAllBytes($Path)
$vl = $Vid -band 0xFF
$vh = ($Vid -shr 8) -band 0xFF

Write-Output ("### {0}" -f (Split-Path $Path -Leaf))
Write-Output ("    {0:N0} bytes" -f $b.Length)

# ---- VID pushes, and whatever was pushed immediately before ----------------
$pairs = @{}
$loneVid = 0
for ($i = 5; $i -lt $b.Length - 5; $i++) {
  if ($b[$i] -ne 0x68 -or $b[$i+1] -ne $vl -or $b[$i+2] -ne $vh -or
      $b[$i+3] -ne 0 -or $b[$i+4] -ne 0) { continue }

  $j = $i - 5
  if ($b[$j] -eq 0x68 -and $b[$j+3] -eq 0 -and $b[$j+4] -eq 0) {
    $p = [int]$b[$j+1] -bor ([int]$b[$j+2] -shl 8)
    $key = '0x{0:X4}' -f $p
    if ($pairs.ContainsKey($key)) { $pairs[$key]++ } else { $pairs[$key] = 1 }
  } else {
    $loneVid++
  }
}

Write-Output ""
Write-Output ("--- VID 0x{0:X4} / PID pairs ---" -f $Vid)
if ($pairs.Count -eq 0) {
  Write-Output "    (none found)"
} else {
  foreach ($k in ($pairs.Keys | Sort-Object)) {
    Write-Output ("    VID 0x{0:X4}  PID {1}   ({2} call site(s))" -f $Vid, $k, $pairs[$k])
  }
}
if ($loneVid -gt 0) {
  Write-Output ("    ({0} VID push(es) with no adjacent immediate - probably a variable PID)" -f $loneVid)
}

# ---- any 16-bit immediate that looks like an EGG product ID ----------------
$cand = @{}
for ($i = 0; $i -lt $b.Length - 5; $i++) {
  if ($b[$i] -ne 0x68 -or $b[$i+3] -ne 0 -or $b[$i+4] -ne 0) { continue }
  $v = [int]$b[$i+1] -bor ([int]$b[$i+2] -shl 8)
  if ($v -ge 0x1900 -and $v -le 0x19FF) {
    $key = '0x{0:X4}' -f $v
    if ($cand.ContainsKey($key)) { $cand[$key]++ } else { $cand[$key] = 1 }
  }
}
Write-Output ""
Write-Output "--- all pushed immediates in 0x1900-0x19FF ---"
if ($cand.Count -eq 0) {
  Write-Output "    (none)"
} else {
  foreach ($k in ($cand.Keys | Sort-Object)) {
    Write-Output ("    {0}   ({1}x)" -f $k, $cand[$k])
  }
}

# ---- protocol command headers, as raw little-endian dwords -----------------
# Each entry is the four header bytes at offsets 0..3 of a request.
$cmds = @(
  @{n = 'cmd 0x0D  dongle info';    b = @(0xA1, 0x0D, 0x00, 0x00)},
  @{n = 'cmd 0x0E  mouse info';     b = @(0xA1, 0x0E, 0x00, 0x00)},
  @{n = 'cmd 0x0F  probe (dongle)'; b = @(0xA1, 0x0F, 0x01, 0x00)},
  @{n = 'cmd 0x0F  probe (mouse)';  b = @(0xA1, 0x0F, 0x0F, 0x00)},
  @{n = 'cmd 0x12  read config';    b = @(0xA1, 0x12, 0x00, 0x00)},
  @{n = 'cmd 0x13  factory reset';  b = @(0xA1, 0x13, 0x00, 0x00)},
  @{n = 'cmd 0x14  sensor block';   b = @(0xA1, 0x14, 0x0F, 0x1C)},
  @{n = 'cmd 0x15  power block';    b = @(0xA1, 0x15, 0x0F, 0x0A)},
  @{n = 'cmd 0x16  button table';   b = @(0xA1, 0x16, 0x0F, 0x1C)},
  @{n = 'cmd 0x70  pair';           b = @(0xA1, 0x70, 0x00, 0x00)},
  @{n = 'cmd 0x71  pair default';   b = @(0xA1, 0x71, 0x00, 0x00)},
  @{n = 'cmd 0x72  pair data';      b = @(0xA1, 0x72, 0x00, 0x00)},
  @{n = 'cmd 0xB4  battery';        b = @(0xA1, 0xB4, 0x00, 0x00)},
  @{n = 'cmd 0x11  store config';   b = @(0xA0, 0x11, 0x00, 0x00)}
)

# Single pass: only positions starting with a report ID can match, which prunes
# essentially the whole file.
$hits = @{}
foreach ($c in $cmds) { $hits[$c.n] = 0 }
$last = $b.Length - 4
for ($i = 0; $i -lt $last; $i++) {
  $v = $b[$i]
  if ($v -ne 0xA1 -and $v -ne 0xA0) { continue }
  $b1 = $b[$i+1]; $b2 = $b[$i+2]; $b3 = $b[$i+3]
  foreach ($c in $cmds) {
    $p = $c.b
    if ($v -eq $p[0] -and $b1 -eq $p[1] -and $b2 -eq $p[2] -and $b3 -eq $p[3]) {
      $hits[$c.n]++
    }
  }
}

Write-Output ""
Write-Output "--- protocol command headers present ---"
foreach ($c in $cmds) {
  $n = $hits[$c.n]
  $mark = if ($n -gt 0) { 'yes' } else { ' - ' }
  Write-Output ("    {0,-28} {1,3}   ({2} occurrence(s))" -f $c.n, $mark, $n)
}
Write-Output ""
