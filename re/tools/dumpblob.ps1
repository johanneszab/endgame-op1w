<#
.SYNOPSIS
  Extract and hexdump the 1024-byte config blob (cmd 0x12 / report 0xA0) from a capture.
#>
param(
  [Parameter(Mandatory = $true)][string]$Pcap,
  [int]$Bytes = 128
)

$b = [System.IO.File]::ReadAllBytes($Pcap)
$best = $null
$off = 24
while ($off + 16 -le $b.Length) {
  $inclLen = [BitConverter]::ToUInt32($b, $off + 8)
  $p = $off + 16
  $off = $p + $inclLen
  if ($p + 28 -gt $b.Length) { break }
  $hdrLen   = [BitConverter]::ToUInt16($b, $p)
  $endpoint = $b[$p + 0x15]
  $transfer = $b[$p + 0x16]
  $dataLen  = [BitConverter]::ToUInt32($b, $p + 0x17)
  if ($transfer -ne 2 -or -not ($endpoint -band 0x80)) { continue }
  $dataOff = $p + $hdrLen
  if ($dataOff + $dataLen -gt $b.Length) { continue }
  if ($null -eq $best -or $dataLen -gt $best.Len) {
    $best = [pscustomobject]@{ Off = $dataOff; Len = $dataLen }
  }
}

if (-not $best) { Write-Warning 'no IN control payload found'; return }
"largest IN control payload: $($best.Len) bytes"

# response framing: [0]=report id, [1]=status, payload from offset 16
$blobOff = $best.Off + 16
$n = [Math]::Min($Bytes, $best.Len - 16)
"blob (first $n bytes, offsets are blob-relative):"
for ($i = 0; $i -lt $n; $i += 16) {
  $row = @()
  for ($j = 0; $j -lt 16 -and ($i + $j) -lt $n; $j++) {
    $row += '{0:X2}' -f $b[$blobOff + $i + $j]
  }
  '  {0:X4}  {1}' -f $i, ($row -join ' ')
}
