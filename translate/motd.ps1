$text1 = "Giveaway in the discord"
#
#
#
$TargetLanguage = $Args[0]
$actualdate = Get-Date -Format "yyyy/MM/dd"
$newyeardate = Get-Date -Month 1 -Day 1 -Year (Get-Date).Year -Format "yyyy/MM/dd"
$Uri1 = “https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text1”
$Response = Invoke-RestMethod -Uri $Uri1 -Method Get
$Translation1 = $Response[0].SyncRoot | foreach { $_[0] }

write-host $Translation1
write-host "https://discord.gg/medicat"


#if ($actualdate -eq $newyeardate) {
#    $newyeartext = "Happy New Year!"
#    $newyearUri = "https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$newyeartext"
#    $Response = Invoke-RestMethod -Uri $newyearUri -Method Get
#    $newyearTranslation = $Response[0][0][0]
#    Write-Host $newyearTranslation
#}
#$text1 = "Giveaway in the discord"
