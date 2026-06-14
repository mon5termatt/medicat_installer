# Parser unit test for 7za progress output. DO NOT DELETE.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Extract-Archive.ps1')

$cases = @(
    @{ Line = '  0%'; ExpectPercent = 0; ExpectFile = $null }
    @{ Line = '45% 12 - Programs\foo\bar.dll'; ExpectPercent = 45; ExpectFile = 'Programs\foo\bar.dll' }
    @{ Line = '45% - Programs\foo\bar.dll'; ExpectPercent = 45; ExpectFile = 'Programs\foo\bar.dll' }
    @{ Line = '- MediCat\Programs\test.exe'; ExpectPercent = -1; ExpectFile = 'MediCat\Programs\test.exe' }
    @{ Line = 'MediCat\Programs\test.exe'; ExpectPercent = -1; ExpectFile = 'MediCat\Programs\test.exe' }
    @{ Line = 'Everything is Ok'; ExpectPercent = $null; ExpectFile = $null }
)

$failed = 0
foreach ($case in $cases) {
    $result = ConvertFrom-Medicat7ZipProgressLine -Line $case.Line
    $ok = $true
    if ($null -eq $case.ExpectPercent) {
        if ($result) { $ok = $false }
    } else {
        if (-not $result -or $result.Percent -ne $case.ExpectPercent -or $result.FileName -ne $case.ExpectFile) {
            $ok = $false
        }
    }
    if (-not $ok) {
        Write-Host "FAIL: '$($case.Line)' => $(if ($result) { "$($result.Percent) / $($result.FileName)" } else { 'null' })"
        $failed++
    } else {
        Write-Host "OK:   '$($case.Line)'"
    }
}

# Rolling buffer: simulate backspace overwrite "  0%" -> " 45%"
$buf = New-Object System.Text.StringBuilder
[void]$buf.Append('  0%')
foreach ($c in @([char]8, [char]8, [char]8, [char]8, '4', '5', '%')) {
    if ($c -eq [char]8) {
        if ($buf.Length -gt 0) { $buf.Length = $buf.Length - 1 }
    } else {
        [void]$buf.Append($c)
    }
    $parsed = Try-Parse-Medicat7ZipRollingBuffer -Buffer $buf
    if ($parsed -and $parsed.Percent -eq 45) {
        Write-Host 'OK:   backspace rolling buffer => 45%'
        break
    }
}

if ($failed -gt 0) {
    Write-Host "$failed case(s) failed"
    exit 1
}
Write-Host 'All parser tests passed'
