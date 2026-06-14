# Shared MediCat archive extraction with real PercentDone progress (SevenZipSharp)
# and 7za.exe CLI fallback (-bsp1 stream parsing).
# Tier 2: vendored lib/SevenZipSharp.dll + 7z*.dll — no PSGallery dependency.

$script:MedicatSevenZipSharpLoaded = $false
$script:MedicatSevenZipBridgeLoaded = $false
$script:MedicatManagedDependenciesInitialized = $false
$script:MedicatSevenZipSharpUsable = $null

$script:MedicatSharpManagedDependencies = @(
    'System.Security.Permissions.dll',
    'System.Configuration.ConfigurationManager.dll',
    'System.Security.Cryptography.ProtectedData.dll'
)

function Write-MedicatExtractLog {
    param(
        [string]$Message,
        [scriptblock]$OnLog
    )

    if ($OnLog) {
        & $OnLog $Message
    }
}

function Get-MedicatLibDirectory {
    param([string]$ScriptRoot = $PSScriptRoot)
    return Join-Path $ScriptRoot 'lib'
}

function Get-MedicatNative7ZipDll {
    param([string]$ScriptRoot = $PSScriptRoot)

    $libDir = Get-MedicatLibDirectory -ScriptRoot $ScriptRoot
    $arch = $env:PROCESSOR_ARCHITECTURE

    if ($arch -eq 'ARM64') {
        $candidate = Join-Path $libDir '7zARM64.dll'
    } elseif ([Environment]::Is64BitProcess) {
        $candidate = Join-Path $libDir '7z64.dll'
    } else {
        $candidate = Join-Path $libDir '7z.dll'
    }

    if (Test-Path -LiteralPath $candidate) {
        return (Resolve-Path -LiteralPath $candidate).Path
    }

    return $null
}

function Initialize-MedicatManagedDependencies {
    param([string]$ScriptRoot = $PSScriptRoot)

    if ($script:MedicatManagedDependenciesInitialized) {
        return
    }

    $libDir = Get-MedicatLibDirectory -ScriptRoot $ScriptRoot
    $script:MedicatResolveLibDir = $libDir
    if (-not $script:MedicatAssemblyResolveCache) {
        $script:MedicatAssemblyResolveCache = @{}
    }
    if (-not $script:MedicatAssemblyResolveLoading) {
        $script:MedicatAssemblyResolveLoading = New-Object 'System.Collections.Generic.HashSet[string]'
    }

    $resolveHandler = [System.ResolveEventHandler]{
        param($sender, $eventArgs)

        $requestedName = $eventArgs.Name
        if ([string]::IsNullOrWhiteSpace($requestedName)) { return $null }
        if ($script:MedicatAssemblyResolveCache.ContainsKey($requestedName)) {
            return $script:MedicatAssemblyResolveCache[$requestedName]
        }
        if ($script:MedicatAssemblyResolveLoading.Contains($requestedName)) { return $null }

        [void]$script:MedicatAssemblyResolveLoading.Add($requestedName)
        try {
            $simpleName = ($requestedName -split ',')[0]
            foreach ($dependency in $script:MedicatSharpManagedDependencies) {
                if ($dependency -ieq "$simpleName.dll") {
                    $candidate = Join-Path $script:MedicatResolveLibDir $dependency
                    if (Test-Path -LiteralPath $candidate) {
                        $assembly = [System.Reflection.Assembly]::LoadFrom((Resolve-Path -LiteralPath $candidate).Path)
                        $script:MedicatAssemblyResolveCache[$requestedName] = $assembly
                        return $assembly
                    }
                }
            }
        } finally {
            [void]$script:MedicatAssemblyResolveLoading.Remove($requestedName)
        }

        return $null
    }

    [System.AppDomain]::CurrentDomain.add_AssemblyResolve($resolveHandler) | Out-Null
    $script:MedicatManagedDependenciesInitialized = $true
}

function Test-MedicatSharpDependenciesPresent {
    param([string]$ScriptRoot = $PSScriptRoot)

    $libDir = Get-MedicatLibDirectory -ScriptRoot $ScriptRoot
    foreach ($dependency in $script:MedicatSharpManagedDependencies) {
        if (-not (Test-Path -LiteralPath (Join-Path $libDir $dependency))) {
            return $false
        }
    }
    return $true
}

function Test-MedicatSevenZipSharpUsable {
    param(
        [string]$ScriptRoot = $PSScriptRoot,
        [scriptblock]$OnLog
    )

    if ($null -ne $script:MedicatSevenZipSharpUsable) {
        return $script:MedicatSevenZipSharpUsable
    }

    if (-not (Test-MedicatLibReady -ScriptRoot $ScriptRoot)) {
        $script:MedicatSevenZipSharpUsable = $false
        return $false
    }

    try {
        if (-not (Initialize-MedicatSevenZipSharp -ScriptRoot $ScriptRoot -OnLog $OnLog)) {
            $script:MedicatSevenZipSharpUsable = $false
            return $false
        }

        $probeArchive = Get-ChildItem -Path $ScriptRoot -Filter '*.7z' -File -ErrorAction SilentlyContinue |
            Select-Object -First 1 -ExpandProperty FullName
        if (-not $probeArchive) {
            $probeArchive = Get-ChildItem -Path $ScriptRoot -Filter '*.zip' -File -ErrorAction SilentlyContinue |
                Select-Object -First 1 -ExpandProperty FullName
        }

        if ($probeArchive) {
            $probe = [SevenZip.SevenZipExtractor]::new($probeArchive)
            $probe.Dispose()
        }

        $script:MedicatSevenZipSharpUsable = $true
        return $true
    } catch {
        Write-MedicatExtractLog "SevenZipSharp unavailable ($($PSVersionTable.PSEdition)): $($_.Exception.Message); using CLI fallback" $OnLog
        $script:MedicatSevenZipSharpUsable = $false
        $script:MedicatSevenZipSharpLoaded = $false
        return $false
    }
}

function Initialize-MedicatSevenZipSharp {
    param(
        [string]$ScriptRoot = $PSScriptRoot,
        [scriptblock]$OnLog
    )

    if ($script:MedicatSevenZipSharpLoaded) {
        return $true
    }

    Initialize-MedicatManagedDependencies -ScriptRoot $ScriptRoot

    $libDir = Get-MedicatLibDirectory -ScriptRoot $ScriptRoot
    $sharpDll = Join-Path $libDir 'SevenZipSharp.dll'

    if (-not (Test-Path -LiteralPath $sharpDll)) {
        Write-MedicatExtractLog "SevenZipSharp.dll not found in $libDir" $OnLog
        return $false
    }

    $nativeDll = Get-MedicatNative7ZipDll -ScriptRoot $ScriptRoot
    if (-not $nativeDll) {
        Write-MedicatExtractLog "7z native DLL not found in $libDir" $OnLog
        return $false
    }

    if (-not ([System.Management.Automation.PSTypeName]'SevenZip.SevenZipExtractor').Type) {
        Add-Type -Path $sharpDll -ErrorAction Stop
    }

    [SevenZip.SevenZipBase]::SetLibraryPath($nativeDll)
    $script:MedicatSevenZipSharpLoaded = $true
    Write-MedicatExtractLog "Loaded SevenZipSharp from lib (native: $(Split-Path -Leaf $nativeDll))" $OnLog
    return $true
}

function Initialize-MedicatSevenZipBridge {
    param(
        [string]$ScriptRoot = $PSScriptRoot,
        [scriptblock]$OnLog
    )

    if ($script:MedicatSevenZipBridgeLoaded) {
        return $true
    }

    if (-not (Initialize-MedicatSevenZipSharp -ScriptRoot $ScriptRoot -OnLog $OnLog)) {
        return $false
    }

    $sharpDll = (Resolve-Path -LiteralPath (Join-Path (Get-MedicatLibDirectory -ScriptRoot $ScriptRoot) 'SevenZipSharp.dll')).Path

    $referencedAssemblies = New-Object 'System.Collections.Generic.List[string]'
    [void]$referencedAssemblies.Add($sharpDll)
    [void]$referencedAssemblies.Add([string][System.Object].Assembly.Location)

    $runtimeDir = [Runtime.InteropServices.RuntimeEnvironment]::GetRuntimeDirectory()
    foreach ($relativePath in @('Facades\netstandard.dll', 'netstandard.dll')) {
        $candidate = Join-Path $runtimeDir $relativePath
        if (Test-Path -LiteralPath $candidate) {
            [void]$referencedAssemblies.Add((Resolve-Path -LiteralPath $candidate).Path)
            break
        }
    }

    $bridgeSource = @'
using System;
using System.Collections.Concurrent;
using System.IO;
using System.Threading;
using SevenZip;

namespace MedicatArchive
{
    public sealed class ProgressUpdate
    {
        public int Percent;
        public string Status;
        public long BytesExtracted;
    }

    public sealed class ExtractionResult
    {
        public bool Success;
        public string ErrorMessage;
        public string Method;
    }

    public sealed class Extractor
    {
        private readonly ConcurrentQueue<ProgressUpdate> _progress = new ConcurrentQueue<ProgressUpdate>();
        private Thread _worker;
        private ExtractionResult _result;
        private int _lastPercent = -1;
        private string _lastFile;
        private long _uncompressedTotal;
        private long _lastFileEnqueueTicks;

        public ConcurrentQueue<ProgressUpdate> Progress { get { return _progress; } }

        public int LastPercent { get { return _lastPercent; } }

        public string LastFile { get { return _lastFile; } }

        public int PendingCount { get { return _progress.Count; } }

        public bool IsRunning
        {
            get { return _worker != null && _worker.IsAlive; }
        }

        public void Start(string archivePath, string destinationPath, string[] files, long uncompressedTotal)
        {
            if (IsRunning)
            {
                throw new InvalidOperationException("Extraction already running");
            }

            _uncompressedTotal = uncompressedTotal;
            _result = null;
            _lastPercent = -1;
            _lastFile = null;
            _lastFileEnqueueTicks = 0;
            _worker = new Thread(() => Run(archivePath, destinationPath, files));
            _worker.IsBackground = true;
            _worker.SetApartmentState(ApartmentState.STA);
            _worker.Start();
        }

        public ExtractionResult GetResult()
        {
            if (IsRunning)
            {
                return null;
            }

            if (_result != null)
            {
                return _result;
            }

            return new ExtractionResult
            {
                Success = false,
                ErrorMessage = "Extraction returned no result",
                Method = "SevenZipSharp"
            };
        }

        private void Enqueue(int percent, string status, long bytes)
        {
            _progress.Enqueue(new ProgressUpdate
            {
                Percent = percent,
                Status = status,
                BytesExtracted = bytes
            });
        }

        private void Run(string archivePath, string destinationPath, string[] files)
        {
            SevenZipExtractor extractor = null;
            try
            {
                if (!Directory.Exists(destinationPath))
                {
                    Directory.CreateDirectory(destinationPath);
                }

                if (destinationPath.Length == 2 && destinationPath[1] == ':')
                {
                    destinationPath += "\\";
                }

                extractor = new SevenZipExtractor(archivePath);

                extractor.Extracting += delegate(object sender, ProgressEventArgs e)
                {
                    int percent = (int)e.PercentDone;
                    if (percent == _lastPercent)
                    {
                        return;
                    }

                    _lastPercent = percent;
                    long bytes = _uncompressedTotal > 0
                        ? (long)Math.Round(_uncompressedTotal * (percent / 100.0))
                        : 0L;
                    Enqueue(percent, null, bytes);
                };

                extractor.FileExtractionStarted += delegate(object sender, FileInfoEventArgs e)
                {
                    string name = e.FileInfo.FileName;
                    if (string.IsNullOrEmpty(name) || name == _lastFile)
                    {
                        return;
                    }

                    long now = DateTime.UtcNow.Ticks;
                    if (now - _lastFileEnqueueTicks < 2500000L)
                    {
                        return;
                    }

                    _lastFileEnqueueTicks = now;
                    _lastFile = name;
                    int pct = _lastPercent >= 0 ? _lastPercent : 0;
                    Enqueue(pct, name, 0L);
                };

                if (files != null && files.Length > 0)
                {
                    extractor.ExtractFiles(destinationPath, files);
                }
                else
                {
                    extractor.ExtractArchive(destinationPath);
                }

                Enqueue(100, "complete", _uncompressedTotal);
                _result = new ExtractionResult { Success = true, Method = "SevenZipSharp" };
            }
            catch (Exception ex)
            {
                _result = new ExtractionResult
                {
                    Success = false,
                    ErrorMessage = ex.Message,
                    Method = "SevenZipSharp"
                };
            }
            finally
            {
                if (extractor != null)
                {
                    extractor.Dispose();
                }
            }
        }
    }
}
'@

    Add-Type -TypeDefinition $bridgeSource -ReferencedAssemblies $referencedAssemblies.ToArray() -ErrorAction Stop
    $script:MedicatSevenZipBridgeLoaded = $true
    return $true
}

function Get-MedicatDestinationRoot {
    param([Parameter(Mandatory)][string]$DriveLetter)

    if ($DriveLetter -match '^[A-Za-z]:$') {
        return "$($DriveLetter.TrimEnd(':')):\"
    }

    return $DriveLetter.TrimEnd('\') + '\'
}

function Get-MedicatDriveDeviceId {
    param([Parameter(Mandatory)][string]$DestinationPath)

    if ($DestinationPath -match '^([A-Za-z]:)') {
        return $matches[1].ToUpperInvariant()
    }

    return $null
}

function Get-MedicatDriveFreeBytes {
    param([Parameter(Mandatory)][string]$DeviceId)

    $disk = Get-WmiObject -Class Win32_LogicalDisk -Filter "DeviceID='$DeviceId'" -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $disk) {
        return $null
    }

    return [int64]$disk.FreeSpace
}

function Get-MedicatDriveWrittenBytes {
    param(
        [Parameter(Mandatory)][string]$DestinationPath,
        [int64]$InitialFreeBytes
    )

    $deviceId = Get-MedicatDriveDeviceId -DestinationPath $DestinationPath
    if (-not $deviceId) {
        return $null
    }

    $currentFree = Get-MedicatDriveFreeBytes -DeviceId $deviceId
    if ($null -eq $currentFree) {
        return $null
    }

    $written = $InitialFreeBytes - $currentFree
    if ($written -lt 0) {
        return [int64]0
    }

    return [int64]$written
}

function Update-MedicatDriveExtractionProgress {
    param(
        [string]$DestinationPath,
        [int64]$UncompressedTotal,
        [int64]$InitialFreeBytes,
        [int]$SharpPercent,
        [string]$CurrentFile,
        [scriptblock]$OnProgress
    )

    if ($UncompressedTotal -le 0 -or $InitialFreeBytes -le 0 -or -not $OnProgress) {
        return $null
    }

    $writtenBytes = Get-MedicatDriveWrittenBytes -DestinationPath $DestinationPath -InitialFreeBytes $InitialFreeBytes
    if ($null -eq $writtenBytes -or $writtenBytes -le 0) {
        return $null
    }

    $drivePercent = [int][math]::Min(99, [math]::Round(($writtenBytes / $UncompressedTotal) * 100))
    if ($drivePercent -le $SharpPercent) {
        return $writtenBytes
    }

    $bytes = [int64][math]::Min($UncompressedTotal, $writtenBytes)
    & $OnProgress $drivePercent $CurrentFile $bytes
    return $writtenBytes
}

function Update-MedicatFolderExtractionProgress {
    param(
        [string]$DestinationPath,
        [int64]$UncompressedTotal,
        [int]$SharpPercent,
        [string]$CurrentFile,
        [scriptblock]$OnProgress
    )

    if ($UncompressedTotal -le 0 -or -not $OnProgress) {
        return
    }

    $destBytes = Get-MedicatDirectoryByteSize -Path $DestinationPath
    if ($destBytes -le 0) {
        return
    }

    $folderPercent = [int][math]::Min(99, [math]::Round(($destBytes / $UncompressedTotal) * 100))
    if ($folderPercent -le $SharpPercent) {
        return
    }

    $percent = $folderPercent
    $bytes = [int64][math]::Min($UncompressedTotal, $destBytes)
    & $OnProgress $percent $CurrentFile $bytes
}

function Drain-MedicatExtractorProgress {
    param(
        [MedicatArchive.Extractor]$Extractor,
        [scriptblock]$OnProgress
    )

    if (-not $OnProgress) {
        return
    }

    $update = $null
    while ($Extractor.Progress.TryDequeue([ref]$update)) {
        & $OnProgress $update.Percent $update.Status $update.BytesExtracted
    }
}

function Wait-MedicatExtractor {
    param(
        [MedicatArchive.Extractor]$Extractor,
        [scriptblock]$OnProgress
    )

    while ($Extractor.IsRunning) {
        Drain-MedicatExtractorProgress -Extractor $Extractor -OnProgress $OnProgress
        Start-Sleep -Milliseconds 50
    }

    Drain-MedicatExtractorProgress -Extractor $Extractor -OnProgress $OnProgress
    return $Extractor.GetResult()
}

function Test-MedicatCliReady {
    param([string]$ScriptRoot = $PSScriptRoot)
    return $null -ne (Get-Medicat7ZipExecutable -ScriptRoot $ScriptRoot)
}

function Test-MedicatExtractionReady {
    param([string]$ScriptRoot = $PSScriptRoot)
    return (Test-MedicatCliReady -ScriptRoot $ScriptRoot) -or (Test-MedicatLibReady -ScriptRoot $ScriptRoot)
}

function Test-MedicatLibReady {
    param([string]$ScriptRoot = $PSScriptRoot)

    $libDir = Get-MedicatLibDirectory -ScriptRoot $ScriptRoot
    $sharpDll = Join-Path $libDir 'SevenZipSharp.dll'
    $nativeDll = Get-MedicatNative7ZipDll -ScriptRoot $ScriptRoot

    if (-not (Test-Path -LiteralPath $sharpDll) -or $null -eq $nativeDll) {
        return $false
    }

    return (Test-MedicatSharpDependenciesPresent -ScriptRoot $ScriptRoot)
}

function Get-Medicat7ZipExecutable {
    param([string]$ScriptRoot = $PSScriptRoot)

    $archSub = if ([Environment]::Is64BitProcess) { 'x64' } else { 'x32' }

    # Prefer vendored 7za so -bsp1 behavior is consistent across machines
    $candidates = @(
        (Join-Path $ScriptRoot "7za.exe"),
        (Join-Path $ScriptRoot "7z\$archSub\7za.exe"),
        (Get-Command "7z.exe" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)
    )

    foreach ($path in $candidates) {
        if ($path -and (Test-Path -LiteralPath $path)) {
            return (Resolve-Path -LiteralPath $path).Path
        }
    }

    return $null
}

function Get-ArchiveUncompressedSize {
    param(
        [Parameter(Mandatory)][string]$ArchivePath,
        [string]$ScriptRoot = $PSScriptRoot,
        [string]$SevenZipExe,
        [switch]$PreferCli,
        [scriptblock]$OnLog
    )

    if (-not (Test-Path -LiteralPath $ArchivePath)) {
        throw "Archive not found: $ArchivePath"
    }

    if (-not $PreferCli) {
        if (Initialize-MedicatSevenZipSharp -ScriptRoot $ScriptRoot -OnLog $OnLog) {
            $extractor = $null
            try {
                $extractor = [SevenZip.SevenZipExtractor]::new($ArchivePath)
                $fileEntries = $extractor.ArchiveFileData | Where-Object { -not $_.IsDirectory }
                if ($fileEntries) {
                    return ($fileEntries | Measure-Object -Property Size -Sum).Sum
                }
            } finally {
                if ($extractor) {
                    $extractor.Dispose()
                }
            }
        }
    }

    if (-not $SevenZipExe) {
        $SevenZipExe = Get-Medicat7ZipExecutable -ScriptRoot $ScriptRoot
    }

    if (-not $SevenZipExe) {
        return 0
    }

    $total = [int64]0
    $output = & $SevenZipExe l -slt $ArchivePath 2>&1
    foreach ($line in $output) {
        if ($line -match '^\s*Size\s*=\s*(\d+)\s*$') {
            $total += [int64]$matches[1]
        }
    }

    return $total
}

function Get-MedicatDirectoryByteSize {
    param([Parameter(Mandatory)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        return [int64]0
    }

    $total = [int64]0
    Get-ChildItem -LiteralPath $Path -Recurse -File -Force -ErrorAction SilentlyContinue |
        ForEach-Object { $total += $_.Length }
    return $total
}

function Update-MedicatExtractionProgress {
    param(
        [hashtable]$State,
        [int]$Percent,
        [string]$Status,
        [int64]$BytesExtracted,
        [scriptblock]$OnProgress
    )

    if (-not $OnProgress) {
        return
    }

    if ($Status -and $Status -notin @('complete', 'starting') -and $Status -ne $State.LastFile) {
        $State.LastFile = $Status
        $reportPercent = if ($Percent -ge 0) { $Percent } elseif ($State.LastPercent -ge 0) { $State.LastPercent } else { 0 }
        & $OnProgress $reportPercent $Status $BytesExtracted
    }

    if ($Percent -lt 0) {
        return
    }
    if ($Percent -gt 100) { $Percent = 100 }
    if ($Percent -le $State.LastPercent) {
        return
    }

    $State.LastPercent = $Percent
    $file = if ($State.LastFile) { $State.LastFile } else { $Status }
    & $OnProgress $Percent $file $BytesExtracted
}

function Try-Parse-Medicat7ZipRollingBuffer {
    param([System.Text.StringBuilder]$Buffer)

    if ($Buffer.Length -eq 0) {
        return $null
    }

    $text = $Buffer.ToString()
    if ($text -match '[\r\n]([^\r\n]*)$') {
        $text = $matches[1]
    }

    return ConvertFrom-Medicat7ZipProgressLine -Line $text
}

function ConvertFrom-Medicat7ZipProgressLine {
    param([string]$Line)

    if ([string]::IsNullOrWhiteSpace($Line)) {
        return $null
    }

    $trimmed = $Line.Trim()
    if (-not $trimmed) {
        return $null
    }

    if ($trimmed -match '^(Everything is Ok|ERROR:|Open ERROR:|System ERROR:|Archives:|Folders:|Files:|Extracting archive:|Type =|Path =|Method =|Solid =|Blocks =|Physical Size =|Headers Size =|^\s*-+\s*$|^\s*Algo|^Scanning)') {
        return $null
    }

    # "45% 12 - Programs\foo\bar.dll"
    if ($trimmed -match '(\d+)%\s+\d+\s+-\s+(.+)$') {
        return @{
            Percent = [int]$matches[1]
            FileName = $matches[2].Trim()
        }
    }

    # "45% - Programs\foo\bar.dll"
    if ($trimmed -match '(\d+)%\s*-\s+(.+)$') {
        return @{
            Percent = [int]$matches[1]
            FileName = $matches[2].Trim()
        }
    }

    # "45%"
    if ($trimmed -match '(\d+)%$') {
        return @{
            Percent = [int]$matches[1]
            FileName = $null
        }
    }

    # 7za lists current file as "- path" or plain path
    if ($trimmed -match '^-\s+(.+)$') {
        return @{
            Percent = -1
            FileName = $matches[1].Trim()
        }
    }

    if ($trimmed -notmatch '%' -and $trimmed.Length -gt 1 -and $trimmed -notmatch '^\d+\s*$') {
        return @{
            Percent = -1
            FileName = $trimmed
        }
    }

    return $null
}

function Read-Medicat7ZipStreamUpdate {
    param(
        [System.IO.StreamReader]$Reader,
        [System.Text.StringBuilder]$Buffer
    )

    while ($Reader.Peek() -ge 0) {
        $charCode = $Reader.Read()
        if ($charCode -lt 0) { break }

        if ($charCode -eq 8) {
            if ($Buffer.Length -gt 0) {
                $Buffer.Length = $Buffer.Length - 1
            }
            $parsed = Try-Parse-Medicat7ZipRollingBuffer -Buffer $Buffer
            if ($parsed) {
                return $parsed
            }
            continue
        }

        $char = [char]$charCode
        if ($char -eq "`r") {
            $line = $Buffer.ToString()
            $null = $Buffer.Clear()
            $parsed = ConvertFrom-Medicat7ZipProgressLine -Line $line
            if ($parsed) {
                return $parsed
            }
            continue
        }

        if ($char -eq "`n") {
            $line = $Buffer.ToString()
            $null = $Buffer.Clear()
            $parsed = ConvertFrom-Medicat7ZipProgressLine -Line $line
            if ($parsed) {
                return $parsed
            }
            continue
        }

        [void]$Buffer.Append($char)

        if ($char -eq '%' -or $Buffer.Length -ge 4) {
            $parsed = Try-Parse-Medicat7ZipRollingBuffer -Buffer $Buffer
            if ($parsed) {
                return $parsed
            }
        }
    }

    return $null
}

function Read-Medicat7ZipStreamPercent {
    param(
        [System.IO.StreamReader]$Reader,
        [System.Text.StringBuilder]$Buffer,
        [regex]$PercentRegex
    )

    while ($Reader.Peek() -ge 0) {
        $charCode = $Reader.Read()
        if ($charCode -lt 0) { break }

        $char = [char]$charCode
        if ($char -eq "`r") {
            $null = $Buffer.Clear()
            continue
        }

        [void]$Buffer.Append($char)
        if ($char -ne '%' -and $char -ne "`n") {
            continue
        }

        $matches = $PercentRegex.Matches($Buffer.ToString())
        if ($matches.Count -gt 0) {
            return [int]$matches[$matches.Count - 1].Groups[1].Value
        }
    }

    return -1
}

function Invoke-MedicatSevenZipSharpExtraction {
    param(
        [Parameter(Mandatory)][string]$ArchivePath,
        [Parameter(Mandatory)][string]$DestinationPath,
        [string]$ScriptRoot = $PSScriptRoot,
        [string[]]$FileList,
        [scriptblock]$OnProgress,
        [scriptblock]$OnLog
    )

    if (-not (Initialize-MedicatSevenZipBridge -ScriptRoot $ScriptRoot -OnLog $OnLog)) {
        throw "SevenZipSharp libraries are missing from lib/"
    }

    $uncompressedTotal = Get-ArchiveUncompressedSize -ArchivePath $ArchivePath -ScriptRoot $ScriptRoot -OnLog $OnLog
    Write-MedicatExtractLog "Uncompressed size: $([math]::Round($uncompressedTotal / 1MB, 2)) MB" $OnLog

    if ($FileList -and $FileList.Count -gt 0) {
        Write-MedicatExtractLog "Extracting $($FileList.Count) file(s) via SevenZipSharp" $OnLog
    } else {
        Write-MedicatExtractLog "Extracting full archive via SevenZipSharp" $OnLog
    }

    if ($OnProgress) {
        & $OnProgress 0 "starting" 0
    }

    $extractor = [MedicatArchive.Extractor]::new()
    $extractor.Start($ArchivePath, $DestinationPath, $FileList, $uncompressedTotal)
    $bridgeResult = Wait-MedicatExtractor -Extractor $extractor -OnProgress $OnProgress

    if (-not $bridgeResult.Success) {
        throw $bridgeResult.ErrorMessage
    }

    return [pscustomobject]@{
        Success = $true
        Method = 'SevenZipSharp'
        ExitCode = 0
        PercentComplete = 100
    }
}

function Invoke-Medicat7ZipCliExtraction {
    param(
        [Parameter(Mandatory)][string]$ArchivePath,
        [Parameter(Mandatory)][string]$DestinationPath,
        [Parameter(Mandatory)][string]$SevenZipExe,
        [string[]]$FileList,
        [string]$FileListPath,
        [int64]$UncompressedTotal = 0,
        [int64]$InitialFreeBytes = 0,
        [scriptblock]$OnProgress,
        [scriptblock]$OnLog
    )

    $DestinationPath = Get-MedicatDestinationRoot -DriveLetter $DestinationPath
    if (-not (Test-Path -LiteralPath $DestinationPath)) {
        New-Item -ItemType Directory -Path $DestinationPath -Force | Out-Null
    }

    if ($UncompressedTotal -le 0) {
        $UncompressedTotal = Get-ArchiveUncompressedSize -ArchivePath $ArchivePath -SevenZipExe $SevenZipExe -PreferCli -OnLog $OnLog
        Write-MedicatExtractLog "CLI uncompressed size: $([math]::Round($UncompressedTotal / 1MB, 2)) MB" $OnLog
    }

    if ($InitialFreeBytes -le 0) {
        $deviceId = Get-MedicatDriveDeviceId -DestinationPath $DestinationPath
        if ($deviceId) {
            $free = Get-MedicatDriveFreeBytes -DeviceId $deviceId
            if ($null -ne $free) {
                $InitialFreeBytes = $free
            }
        }
    }

    $args = @(
        'x',
        '-bsp1', '-bso1', '-bse1',
        "-o`"$DestinationPath`"",
        "`"$ArchivePath`"",
        '-aoa', '-y'
    )

    if ($FileListPath -and (Test-Path -LiteralPath $FileListPath)) {
        $args += "@`"$FileListPath`""
    }

    $argString = $args -join ' '
    Write-MedicatExtractLog "7z CLI: $SevenZipExe $argString" $OnLog

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $SevenZipExe
    $psi.Arguments = $argString
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $psi

    $stdoutBuffer = New-Object System.Text.StringBuilder
    $stderrBuffer = New-Object System.Text.StringBuilder
    $progressState = @{
        LastPercent = -1
        LastFile = $null
        LastDrivePoll = [datetime]::MinValue
    }

    $process.Start() | Out-Null
    $stdout = $process.StandardOutput
    $stderr = $process.StandardError

    while (-not $process.HasExited -or ($stdout.Peek() -ge 0) -or ($stderr.Peek() -ge 0)) {
        foreach ($reader in @($stdout, $stderr)) {
            $buffer = if ($reader -eq $stdout) { $stdoutBuffer } else { $stderrBuffer }
            $update = Read-Medicat7ZipStreamUpdate -Reader $reader -Buffer $buffer
            if ($update) {
                $bytes = 0
                if ($InitialFreeBytes -gt 0) {
                    $written = Get-MedicatDriveWrittenBytes -DestinationPath $DestinationPath -InitialFreeBytes $InitialFreeBytes
                    if ($null -ne $written -and $written -gt 0) {
                        $bytes = [int64][math]::Min($UncompressedTotal, $written)
                    }
                }
                Update-MedicatExtractionProgress `
                    -State $progressState `
                    -Percent $update.Percent `
                    -Status $update.FileName `
                    -BytesExtracted $bytes `
                    -OnProgress $OnProgress
            }
        }

        if ($UncompressedTotal -gt 0 -and $InitialFreeBytes -gt 0 -and $OnProgress) {
            $now = Get-Date
            if (($now - $progressState.LastDrivePoll).TotalMilliseconds -ge 2000) {
                $progressState.LastDrivePoll = $now
                $written = Get-MedicatDriveWrittenBytes -DestinationPath $DestinationPath -InitialFreeBytes $InitialFreeBytes
                if ($null -ne $written -and $written -gt 0) {
                    $drivePercent = [int][math]::Min(99, [math]::Round(($written / $UncompressedTotal) * 100))
                    $bytes = [int64][math]::Min($UncompressedTotal, $written)
                    Update-MedicatExtractionProgress `
                        -State $progressState `
                        -Percent $drivePercent `
                        -Status $progressState.LastFile `
                        -BytesExtracted $bytes `
                        -OnProgress $OnProgress
                }
            }
        }

        if (-not $process.HasExited -and $stdout.Peek() -lt 0 -and $stderr.Peek() -lt 0) {
            Start-Sleep -Milliseconds 50
        }
    }

    $process.WaitForExit()
    $stderrTail = $stderr.ReadToEnd()
    if ($stderrTail) {
        Write-MedicatExtractLog "7z stderr: $stderrTail" $OnLog
    }

    if ($process.ExitCode -eq 0) {
        if ($OnProgress) { & $OnProgress 100 "complete" $UncompressedTotal }
        return [pscustomobject]@{
            Success = $true
            Method = '7zCli'
            ExitCode = 0
            PercentComplete = 100
        }
    }

    throw "7z.exe extraction failed with exit code $($process.ExitCode)"
}

function Invoke-MedicatZipExtraction {
    param(
        [Parameter(Mandatory)][string]$ArchivePath,
        [Parameter(Mandatory)][string]$DestinationPath,
        [string]$ScriptRoot = $PSScriptRoot,
        [scriptblock]$OnProgress,
        [scriptblock]$OnLog
    )

    if (-not (Test-Path -LiteralPath $DestinationPath)) {
        New-Item -ItemType Directory -Path $DestinationPath -Force | Out-Null
    }

    Write-MedicatExtractLog "Extracting zip via Expand-Archive" $OnLog
    if ($OnProgress) { & $OnProgress 0 "starting" 0 }

    try {
        Expand-Archive -LiteralPath $ArchivePath -DestinationPath $DestinationPath -Force
        if ($OnProgress) { & $OnProgress 100 "complete" 0 }
        return [pscustomobject]@{
            Success = $true
            Method = 'ExpandArchive'
            ExitCode = 0
            PercentComplete = 100
        }
    } catch {
        throw "Expand-Archive failed: $($_.Exception.Message)"
    }
}

function Invoke-MedicatCliArchiveExtraction {
    param(
        [Parameter(Mandatory)][string]$ArchivePath,
        [Parameter(Mandatory)][string]$DestinationPath,
        [string]$ScriptRoot = $PSScriptRoot,
        [string[]]$FileList,
        [int64]$UncompressedTotal = 0,
        [int64]$InitialFreeBytes = 0,
        [scriptblock]$OnProgress,
        [scriptblock]$OnLog,
        $ProgressQueue
    )

    if ($ProgressQueue) {
        $queue = $ProgressQueue
        $OnProgress = {
            param($Percent, $Status, $BytesExtracted)
            [void]$queue.Enqueue(@($Percent, $Status, $BytesExtracted))
        }
    }

    $sevenZipExe = Get-Medicat7ZipExecutable -ScriptRoot $ScriptRoot
    if (-not $sevenZipExe) {
        throw "7za.exe not found"
    }

    if ($UncompressedTotal -le 0) {
        $UncompressedTotal = Get-ArchiveUncompressedSize `
            -ArchivePath $ArchivePath `
            -SevenZipExe $sevenZipExe `
            -ScriptRoot $ScriptRoot `
            -PreferCli `
            -OnLog $OnLog
    }

    $tempListPath = $null
    try {
        if ($FileList -and $FileList.Count -gt 0) {
            $tempListPath = Join-Path $env:TEMP "medicat_extract_$([Guid]::NewGuid().ToString('N').Substring(0, 8)).txt"
            $FileList | Out-File -FilePath $tempListPath -Encoding UTF8
        }

        return Invoke-Medicat7ZipCliExtraction `
            -ArchivePath $ArchivePath `
            -DestinationPath $DestinationPath `
            -SevenZipExe $sevenZipExe `
            -FileList $FileList `
            -FileListPath $tempListPath `
            -UncompressedTotal $UncompressedTotal `
            -InitialFreeBytes $InitialFreeBytes `
            -OnProgress $OnProgress `
            -OnLog $OnLog
    } finally {
        if ($tempListPath -and (Test-Path -LiteralPath $tempListPath)) {
            Remove-Item -LiteralPath $tempListPath -Force -ErrorAction SilentlyContinue
        }
    }
}

function Invoke-MedicatArchiveExtraction {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$ArchivePath,
        [Parameter(Mandatory)][string]$DestinationPath,
        [string[]]$FileList,
        [int64]$UncompressedTotal = 0,
        [int64]$InitialFreeBytes = 0,
        [scriptblock]$OnProgress,
        [scriptblock]$OnLog,
        $ProgressQueue,
        [switch]$AllowCliFallback,
        [switch]$ForceCli,
        [string]$ScriptRoot = $PSScriptRoot
    )

    if ($ProgressQueue) {
        $queue = $ProgressQueue
        $OnProgress = {
            param($Percent, $Status, $BytesExtracted)
            [void]$queue.Enqueue(@($Percent, $Status, $BytesExtracted))
        }
    }

    if (-not (Test-Path -LiteralPath $ArchivePath)) {
        throw "Archive not found: $ArchivePath"
    }

    $startTime = Get-Date
    $archiveExtension = [System.IO.Path]::GetExtension($ArchivePath).ToLowerInvariant()
    $cliError = $null
    $sharpError = $null
    $result = $null

    try {
        if ($OnProgress) { & $OnProgress 0 "starting" 0 }

        # SevenZipSharp — small archives / selective extract (hangs on full solid MediCat .7z)
        if (-not $ForceCli -and (Test-MedicatSevenZipSharpUsable -ScriptRoot $ScriptRoot -OnLog $OnLog)) {
            try {
                if (Initialize-MedicatSevenZipSharp -ScriptRoot $ScriptRoot -OnLog $OnLog) {
                    $result = Invoke-MedicatSevenZipSharpExtraction `
                        -ArchivePath $ArchivePath `
                        -DestinationPath $DestinationPath `
                        -ScriptRoot $ScriptRoot `
                        -FileList $FileList `
                        -OnProgress $OnProgress `
                        -OnLog $OnLog
                }
            } catch {
                $sharpError = $_
                Write-MedicatExtractLog "SevenZipSharp extraction failed: $($_.Exception.Message)" $OnLog
                $script:MedicatSevenZipSharpUsable = $false
                if (-not $AllowCliFallback) { throw }
            }
        }

        if ((-not $result) -and ($AllowCliFallback -or $ForceCli)) {
            try {
                $result = Invoke-MedicatCliArchiveExtraction `
                    -ArchivePath $ArchivePath `
                    -DestinationPath $DestinationPath `
                    -ScriptRoot $ScriptRoot `
                    -FileList $FileList `
                    -UncompressedTotal $UncompressedTotal `
                    -InitialFreeBytes $InitialFreeBytes `
                    -OnProgress $OnProgress `
                    -OnLog $OnLog
            } catch {
                $cliError = $_
                Write-MedicatExtractLog "7za.exe fallback failed: $($_.Exception.Message)" $OnLog
            }
        } elseif (-not $result -and $archiveExtension -eq '.zip') {
            try {
                $result = Invoke-MedicatZipExtraction `
                    -ArchivePath $ArchivePath `
                    -DestinationPath $DestinationPath `
                    -ScriptRoot $ScriptRoot `
                    -OnProgress $OnProgress `
                    -OnLog $OnLog
            } catch {
                Write-MedicatExtractLog $_.Exception.Message $OnLog
            }
        }

        if (-not $result) {
            if ($sharpError) { throw $sharpError }
            if ($cliError) { throw $cliError }
            throw "Extraction libraries missing from lib/. See lib/NOTICE.txt"
        }

        $duration = ((Get-Date) - $startTime).TotalSeconds
        $result | Add-Member -NotePropertyName DurationSeconds -NotePropertyValue $duration -Force
        $result | Add-Member -NotePropertyName ArchivePath -NotePropertyValue $ArchivePath -Force
        $result | Add-Member -NotePropertyName DestinationPath -NotePropertyValue $DestinationPath -Force
        return $result
    } catch {
        $duration = ((Get-Date) - $startTime).TotalSeconds
        return [pscustomobject]@{
            Success = $false
            Method = 'none'
            ExitCode = -1
            PercentComplete = 0
            DurationSeconds = $duration
            ArchivePath = $ArchivePath
            DestinationPath = $DestinationPath
            ErrorMessage = $_.Exception.Message
        }
    }
}
