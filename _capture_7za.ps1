$root = 'C:\Users\Matt\Github\medicat_installer'
$sevenZip = Join-Path $root '7za.exe'
$archive = Join-Path $root 'MediCat.USB.v21.12.7z'
$dest = 'H:\'
$rawOut = Join-Path $env:TEMP '7za_raw_capture.txt'

if (-not (Test-Path $archive)) { Write-Host 'No archive'; exit 1 }

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $sevenZip
$psi.Arguments = "x -bsp1 -bso1 -bse1 -o`"$dest`" `"$archive`" -aoa -y"
$psi.UseShellExecute = $false
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.CreateNoWindow = $true

$p = [System.Diagnostics.Process]::Start($psi)
$sb = New-Object System.Text.StringBuilder
$deadline = (Get-Date).AddSeconds(20)
$stdoutStream = $p.StandardOutput.BaseStream
$stderrStream = $p.StandardError.BaseStream
$readBuf = New-Object byte[] 4096

while ((Get-Date) -lt $deadline -and -not $p.HasExited) {
    foreach ($stream in @($stdoutStream, $stderrStream)) {
        while ($stream.DataAvailable) {
            $n = $stream.Read($readBuf, 0, $readBuf.Length)
            if ($n -le 0) { break }
            [void]$sb.Append([System.Text.Encoding]::UTF8.GetString($readBuf, 0, $n))
        }
    }
    Start-Sleep -Milliseconds 50
}

# Drain any remaining bytes after deadline or exit
foreach ($stream in @($stdoutStream, $stderrStream)) {
    while ($stream.DataAvailable) {
        $n = $stream.Read($readBuf, 0, $readBuf.Length)
        if ($n -le 0) { break }
        [void]$sb.Append([System.Text.Encoding]::UTF8.GetString($readBuf, 0, $n))
    }
}

try { $p.Kill() } catch {}
$text = $sb.ToString()
$escaped = ($text.ToCharArray() | ForEach-Object {
    switch ([int]$_) {
        9 { '<TAB>' }
        10 { '<LF>' + "`n" }
        13 { '<CR>' + "`n" }
        8 { '<BS>' }
        default { $_ }
    }
}) -join ''
$escaped | Set-Content $rawOut -Encoding UTF8
Write-Host "Captured $($text.Length) chars to $rawOut"
Write-Host '--- last 2000 chars ---'
if ($text.Length -gt 2000) { $text.Substring($text.Length - 2000) } else { $text }
