$TargetLanguage = $Args[0]
$actualdate = Get-Date -Format "yyyy/MM/dd"
$newyeardate = Get-Date -Month 1 -Day 1 -Year (Get-Date).Year -Format "yyyy/MM/dd"

if ($actualdate -eq $newyeardate) {
    $newyeartext = "Happy New Year!"
    $newyearUri = "https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$newyeartext"
    $Response = Invoke-RestMethod -Uri $newyearUri -Method Get
    $newyearTranslation = $Response[0][0][0]
    Write-Host $newyearTranslation
}

Write-Host "https://discord.gg/medicat"
