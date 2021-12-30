@echo off &chcp 850 >nul &pushd "%~dp0"
@set "0=%~f0" &powershell -nop -c $f=[IO.File]::ReadAllText($env:0)-split':bat2file\:.*';iex($f[1]); X(1) &cls &exit/b

:bat2file: Compressed2TXT v6.5
$k='.,;{-}[+](/)_|^=?O123456789ABCDeFGHyIdJKLMoN0PQRSTYUWXVZabcfghijklmnpqrstuvwxz!@#$&~E<*`%\>'; Add-Type -Ty @'
using System.IO;public class BAT91{public static void Dec(ref string[] f,int x,string fo,string key){unchecked{int n=0,c=255,q=0
,v=91,z=f[x].Length; byte[]b91=new byte[256]; while(c>0) b91[c--]=91; while(c<91) b91[key[c]]=(byte)c++; using (FileStream o=new
FileStream(fo,FileMode.Create)){for(int i=0;i!=z;i++){c=b91[f[x][i]]; if(c==91)continue; if(v==91){v=c;}else{v+=c*91;q|=v<<n;if(
(v&8191)>88){n+=13;}else{n+=14;}v=91;do{o.WriteByte((byte)q);q>>=8;n-=8;}while(n>7);}}if(v!=91)o.WriteByte((byte)(q|v<<n));} }}}
'@; cd -Lit($env:__CD__); function X([int]$x=1){[BAT91]::Dec([ref]$f,$x+1,$x,$k); expand -R $x -F:* .; del $x -force}

:bat2file:[ file1
::AVEYO...Rng.......N=........z;].o;..Q5B-..a]..d,qGy+n;........Rntc+Of}MXcTgyT.*/..?#{.....^ZDpV5qGS1cW2;U`..Q5i.....|$Y[r.;P)vQD{(
::?#{.,P(-....>1NW^[?.#W[kju6)e...$4....`wFWc7k.Ne5xFD-PG,..!J;...Rntc(Of}MXcTgyV.3...0g)...;>WA2HA)$hEXB.j[25A1~h{QP.%w].2y};,P;.W5
::;.Nj;1MCrFFq*zp^c|$7&Yc(<.dapE%P=`W,9;....fy;.u}iDcHmd{)OqsOz9mmop=\5<H`*tn!onV)y,......`))P_xbxrTgG?$ek!`cj~%&$|Ke%nLG/+#9\[m5$?<
::&4}%t*$t9}=KR!z!P6uC@HK(wnSTv7}6sj}(wFryGUf0hEvK#|0HnaRS`TtcRZgs3YYbLUQE]#sy>zPWJYtJ4vu*}{%k)9(fw}bTjr#]i1yGkWJH_^@JdD.\TAcgv{_[*$
::G*&3lYeJ]^uS-8B?07>UN7wZ#V$$,|vYg?G5J|Gj0hp7GJiUB3;TwJ?whTR7[s.(q6y)q,
:bat2file:]
