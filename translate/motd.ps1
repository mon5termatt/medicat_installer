$TargetLanguage = $Args[0]

$text1 = "Giveaway in the discord"

$Uri1 = “https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=$($TargetLanguage)&dt=t&q=$text1”
$Response = Invoke-RestMethod -Uri $Uri1 -Method Get
