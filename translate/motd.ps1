$TargetLanguage = $Args[0]
$actualdate = Get-Date -Format "yyyy/MM/dd"
$newyeardate = Get-Date -Month 1 -Day 1 -Year (Get-Date).Year -Format "yyyy/MM/dd"

if ($actualdate -eq $newyeardate) {
    $newyeartext = "Happy New Year!"
    $Uri1 = "https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$newyeartext"
    $Response = Invoke-RestMethod -Uri $Uri1 -Method Get
    $Translation1 = $Response[0][0][0]
    Write-Host $Translation1
}

Write-Host "https://discord.gg/medicat"
