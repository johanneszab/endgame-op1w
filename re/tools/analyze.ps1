<#
.SYNOPSIS
  Differential analysis across a directory of single-change USBPcap captures.

.DESCRIPTION
  Parses every capture, groups the outgoing feature reports by (cmd, chunk index),
  and reports, for each capture, which payload bytes differ from the baseline
  capture. That maps a UI control directly onto a byte offset.

.EXAMPLE
  .\analyze.ps1 -Dir ..\captures -Baseline 00_baseline.pcap
#>
param(
  [string]$Dir = 'C:\Users\jmeyer\Downloads\endgame\re\captures',
  [string]$Baseline = '00_baseline.pcap',
  [switch]$ShowAll        # also print the full packet list per capture
)

$ErrorActionPreference = 'Stop'

function Get-FeaturePackets {
  param([string]$File)

  $b = [System.IO.File]::ReadAllBytes($File)
  $out = @()
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
    if ($transfer -ne 2) { continue }
    $stage   = if ($hdrLen -ge 28) { $b[$p + 0x1B] } else { 255 }
    $dataOff = $p + $hdrLen
    if ($dataOff + $dataLen -gt $b.Length) { continue }

    if ($stage -eq 0 -and $dataLen -gt 8) {
      $bReq   = $b[$dataOff + 1]
      $wValue = [BitConverter]::ToUInt16($b, $dataOff + 2)
      if ((($wValue -shr 8) -band 0xFF) -ne 3 -or $bReq -ne 9) { continue }
      $d = $b[($dataOff + 8)..($dataOff + $dataLen - 1)]
      $out += [pscustomobject]@{ Dir = 'SET'; Cmd = $d[1]; Sub = $d[2]; Len = $d[3]; Idx = $d[6]; Data = $d }
    }
    elseif ($stage -eq 3 -and $dataLen -ge 16 -and ($endpoint -band 0x80)) {
      $d = $b[$dataOff..($dataOff + $dataLen - 1)]
      $out += [pscustomobject]@{ Dir = 'GET'; Cmd = $null; Sub = $null; Len = $null; Idx = $d[1]; Data = $d }
    }
  }
  return $out
}

# Captures are cumulative: each APPLY resends the whole block, carrying forward
# every earlier change. So diff each capture against the PREVIOUS state of that
# same (cmd, idx), in chronological order - that isolates one setting per capture.
$files = Get-ChildItem $Dir -Filter *.pcap | Sort-Object LastWriteTime

$seen = @{}     # "cmd/idx" -> last payload

foreach ($f in $files) {
  $sets = Get-FeaturePackets $f.FullName | Where-Object { $_.Dir -eq 'SET' }

  "=== $($f.Name)"
  if (-not $sets) { '    (no outgoing feature reports)'; ''; continue }

  foreach ($s in $sets) {
    $key = '{0:X2}/{1:X2}' -f $s.Cmd, $s.Idx
    $hex = ($s.Data[16..([Math]::Min(47, $s.Data.Count - 1))] | ForEach-Object { '{0:X2}' -f $_ }) -join ' '

    if (-not $seen.ContainsKey($key)) {
      '    cmd={0} FIRST SEEN  len={1:X2}' -f $key, $s.Len
      "        $hex"
    }
    else {
      $prev = $seen[$key]
      $diffs = @()
      $n = [Math]::Min($prev.Count, $s.Data.Count)
      for ($i = 0; $i -lt $n; $i++) {
        if ($prev[$i] -ne $s.Data[$i]) {
          $diffs += ('payload +{0,2} : {1:X2} -> {2:X2}' -f ($i - 16), $prev[$i], $s.Data[$i])
        }
      }
      if ($diffs) {
        '    cmd={0}' -f $key
        foreach ($d in $diffs) { "        $d" }
      } else {
        '    cmd={0}  (unchanged)' -f $key
      }
    }
    if ($ShowAll) { "        raw: $hex" }
    $seen[$key] = $s.Data
  }
  ''
}
