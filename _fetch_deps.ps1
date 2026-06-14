Set-Location $PSScriptRoot
$libDir = Join-Path $PSScriptRoot 'lib'
$pkgRoot = Join-Path $PSScriptRoot '_pkg'
New-Item -ItemType Directory -Path $pkgRoot -Force | Out-Null

$packages = @(
    @{ Name = 'System.Security.Permissions'; Version = '4.7.0' },
    @{ Name = 'System.Configuration.ConfigurationManager'; Version = '4.7.0' },
    @{ Name = 'System.Security.Cryptography.ProtectedData'; Version = '4.7.0' }
)

foreach ($pkg in $packages) {
    $nupkg = Join-Path $pkgRoot ($pkg.Name + '.' + $pkg.Version + '.nupkg')
    $url = 'https://www.nuget.org/api/v2/package/' + $pkg.Name + '/' + $pkg.Version
    Write-Host "Downloading $($pkg.Name)..."
    Invoke-WebRequest -Uri $url -OutFile $nupkg -UseBasicParsing
    $extractDir = Join-Path $pkgRoot $pkg.Name
    if (Test-Path $extractDir) { Remove-Item $extractDir -Recurse -Force }
    Expand-Archive -Path $nupkg -DestinationPath $extractDir -Force
}

$dllSources = @(
    (Join-Path $pkgRoot 'System.Security.Permissions\lib\net461\System.Security.Permissions.dll'),
    (Join-Path $pkgRoot 'System.Configuration.ConfigurationManager\lib\net461\System.Configuration.ConfigurationManager.dll'),
    (Join-Path $pkgRoot 'System.Security.Cryptography.ProtectedData\runtimes\win\lib\net461\System.Security.Cryptography.ProtectedData.dll')
)

foreach ($src in $dllSources) {
    if (-not (Test-Path -LiteralPath $src)) {
        throw "Missing expected DLL: $src"
    }
    $dest = Join-Path $libDir (Split-Path -Leaf $src)
    Copy-Item -LiteralPath $src -Destination $dest -Force
    Write-Host "Copied $(Split-Path -Leaf $src)"
}

Remove-Item $pkgRoot -Recurse -Force
Write-Host 'Done'
