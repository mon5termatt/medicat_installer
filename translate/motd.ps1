# Legacy batch prints MOTD after the main menu is drawn. If the user somehow
# skipped licence.ps1 migration, nudge every menu redraw toward the C++ updater.
#
# Path: https://raw.githubusercontent.com/mon5termatt/medicat_installer/main/translate/motd.ps1

$TargetLanguage = $Args[0]
$text1 = 'This batch installer is obsolete. Close it and run MedicatInstaller.exe from GitHub Releases, or re-run so licence.ps1 can auto-update you.'

try {
    if ($TargetLanguage -and $TargetLanguage -ne 'en') {
        $Uri1 = "https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text1"
        $Response = Invoke-RestMethod -Uri $Uri1 -Method Get
        $Translation1 = $Response[0].SyncRoot | ForEach-Object { $_[0] }
        if ($Translation1) { $text1 = $Translation1 }
    }
} catch { }

Write-Host $text1
