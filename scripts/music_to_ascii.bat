@echo off
chcp 65001 >nul
rem ============================================================================
rem  music_to_ascii.bat  把音乐转成英文文件名,中文歌名保存在歌曲标签里
rem
rem  为什么需要:有些 SD 卡上,Switch 读不出中文文件名的文件,英文名就正常。
rem  DferTune 的列表显示的是标签里的歌名和歌手,所以文件名改成英文,
rem  标签里仍是中文,界面上看到的还是中文。
rem
rem  用法:把音乐文件夹拖到这个文件上,或者双击后输入文件夹路径。
rem  路径后面加 -r 可以连子文件夹里的歌一起处理,例如:
rem      music_to_ascii.bat "F:\music" -r
rem
rem  结果:直接在原文件夹里生成英文名的副本,例如 0001_a1b2c3.mp3。
rem  再次运行时同一首歌会覆盖上次生成的副本,新增的歌曲接在已有序号后面,
rem  已经转换好的文件不会被重复转换。中文名的原文件不会被改动或删除。
rem  每个文件夹最多放 300 首,更多的放进 part02、part03 子文件夹。
rem
rem  说明:下面可执行的命令行刻意只用英文字符,因为 cmd.exe 对其它字符的
rem  处理不可靠。真正的工作和所有中文提示都在文件后半部分的 PowerShell 里。
rem ============================================================================
setlocal
set "MTA_BAT=%~f0"
set "MTA_SRC=%~1"
set "MTA_RECURSE=0"
if /i "%~2"=="-r" set "MTA_RECURSE=1"
powershell -NoProfile -ExecutionPolicy Bypass -Command "$t=[IO.File]::ReadAllText($env:MTA_BAT,[Text.Encoding]::UTF8); Invoke-Expression $t.Substring($t.IndexOf('#PS'+'-BEGIN'))"
set "MTA_CODE=%ERRORLEVEL%"
echo.
pause
exit /b %MTA_CODE%
#PS-BEGIN
# ==== 以下是 PowerShell 部分(由上面的批处理调用,Windows 自带 PowerShell 5.1 即可) ====
$ErrorActionPreference = 'Stop'
try { [Console]::OutputEncoding = [Text.Encoding]::UTF8 } catch {}
try { [Text.Encoding]::RegisterProvider([Text.CodePagesEncodingProvider]::Instance) } catch {}

$utf8   = New-Object Text.UTF8Encoding($false)
$latin1 = [Text.Encoding]::GetEncoding(28591)
$gbkStrict = $null
try {
    $gbkStrict = [Text.Encoding]::GetEncoding(936, [Text.EncoderFallback]::ExceptionFallback, [Text.DecoderFallback]::ExceptionFallback)
} catch {}

$extensions = @('.mp3', '.flac', '.wav', '.wave')
$chunk = 300   # DferTune「添加全部」一次最多 300 首

function Get-Slice([byte[]]$b, [int]$start, [int]$len) {
    $r = New-Object byte[] $len
    if ($len -gt 0) { [Array]::Copy($b, $start, $r, 0, $len) }
    return ,$r
}

function Read-Syncsafe([byte[]]$b, [int]$o) {
    return ((([int]$b[$o] -band 0x7F) -shl 21) -bor (([int]$b[$o + 1] -band 0x7F) -shl 14) -bor (([int]$b[$o + 2] -band 0x7F) -shl 7) -bor ([int]$b[$o + 3] -band 0x7F))
}

function Get-SyncsafeBytes([int]$n) {
    return ,([byte[]]@((($n -shr 21) -band 0x7F), (($n -shr 14) -band 0x7F), (($n -shr 7) -band 0x7F), ($n -band 0x7F)))
}

function Get-ShortHash([string]$s) {
    $sha = [Security.Cryptography.SHA1]::Create()
    $hash = $sha.ComputeHash($utf8.GetBytes($s.ToLowerInvariant()))
    return (($hash | ForEach-Object { $_.ToString('x2') }) -join '').Substring(0, 6)
}

# 把 ID3 文字帧(第 1 个字节是编码)解成字符串。旧标签常把 GBK 标成 Latin-1,这里自动修回中文。
function Convert-TagText([byte[]]$d) {
    if ($d.Length -lt 2) { return '' }
    $enc = [int]$d[0]
    $body = Get-Slice $d 1 ($d.Length - 1)
    $text = $null
    if ($enc -eq 0) {
        $hasHigh = $false
        foreach ($x in $body) { if ($x -ge 0x80) { $hasHigh = $true; break } }
        if ($hasHigh -and $gbkStrict) {
            try { $text = $gbkStrict.GetString($body) } catch { $text = $null }
        }
        if ($text -eq $null) { $text = $latin1.GetString($body) }
    }
    elseif ($enc -eq 1) {
        if ($body.Length -ge 2 -and $body[0] -eq 0xFE -and $body[1] -eq 0xFF) {
            $text = [Text.Encoding]::BigEndianUnicode.GetString($body, 2, $body.Length - 2)
        }
        elseif ($body.Length -ge 2 -and $body[0] -eq 0xFF -and $body[1] -eq 0xFE) {
            $text = [Text.Encoding]::Unicode.GetString($body, 2, $body.Length - 2)
        }
        else { $text = [Text.Encoding]::Unicode.GetString($body) }
    }
    elseif ($enc -eq 2) { $text = [Text.Encoding]::BigEndianUnicode.GetString($body) }
    elseif ($enc -eq 3) { $text = $utf8.GetString($body) }
    else { $text = $latin1.GetString($body) }
    $parts = @($text.Split([char]0) | Where-Object { $_.Trim().Length -gt 0 })
    return ($parts -join '/')
}

# 文字 -> 帧数据:ID3v2.3 用 UTF-16(带 BOM),ID3v2.4 用 UTF-8。
function ConvertTo-TagData([string]$text, [int]$ver) {
    if ($ver -ge 4) {
        $head = [byte[]]@(3)
        $payload = $utf8.GetBytes($text)
    } else {
        $head = [byte[]]@(1, 0xFF, 0xFE)
        $payload = [Text.Encoding]::Unicode.GetBytes($text)
    }
    $r = New-Object byte[] ($head.Length + $payload.Length)
    [Array]::Copy($head, 0, $r, 0, $head.Length)
    [Array]::Copy($payload, 0, $r, $head.Length, $payload.Length)
    return ,$r
}

function New-Frame([string]$id, [byte[]]$data, [byte[]]$flags, [int]$ver) {
    $size = $data.Length
    if ($ver -ge 4) {
        $sz = Get-SyncsafeBytes $size
    } else {
        $sz = [byte[]]@((($size -shr 24) -band 0xFF), (($size -shr 16) -band 0xFF), (($size -shr 8) -band 0xFF), ($size -band 0xFF))
    }
    $ms = New-Object IO.MemoryStream
    $idb = [Text.Encoding]::ASCII.GetBytes($id)
    $ms.Write($idb, 0, 4)
    $ms.Write($sz, 0, 4)
    $ms.Write($flags, 0, 2)
    $ms.Write($data, 0, $data.Length)
    return ,($ms.ToArray())
}

# 修好 mp3 副本里的标签:文字转成 Unicode,没有歌名就写入原文件名。
# 返回歌名;遇到不好安全改写的标签(ID3v2.2、不同步、扩展头)就原样保留并返回 $null。
function Update-Mp3Tag([string]$path, [string]$stem) {
    $b = [IO.File]::ReadAllBytes($path)
    $frames = New-Object 'System.Collections.Generic.List[byte[]]'
    $ver = 3
    $audioStart = 0
    $title = ''

    if ($b.Length -ge 10 -and $b[0] -eq 0x49 -and $b[1] -eq 0x44 -and $b[2] -eq 0x33) {
        $ver = [int]$b[3]
        $flags = [int]$b[5]
        if (($ver -ne 3 -and $ver -ne 4) -or (($flags -band 0xC0) -ne 0)) { return $null }
        $tagSize = Read-Syncsafe $b 6
        $audioStart = 10 + $tagSize
        if ($ver -eq 4 -and (($flags -band 0x10) -ne 0)) { $audioStart += 10 }
        if ($audioStart -gt $b.Length) { return $null }

        $o = 10
        $end = 10 + $tagSize
        while (($o + 10) -le $end -and $b[$o] -ne 0) {
            $id = [Text.Encoding]::ASCII.GetString($b, $o, 4)
            if ($ver -ge 4) {
                $fs = Read-Syncsafe $b ($o + 4)
            } else {
                $fs = (([int]$b[$o + 4] -shl 24) -bor ([int]$b[$o + 5] -shl 16) -bor ([int]$b[$o + 6] -shl 8) -bor [int]$b[$o + 7])
            }
            if ($fs -lt 0 -or ($o + 10 + $fs) -gt $end) { break }
            $fl = Get-Slice $b ($o + 8) 2
            $data = Get-Slice $b ($o + 10) $fs
            $isText = $id.StartsWith('T') -and $id -ne 'TXXX' -and $fl[0] -eq 0 -and $fl[1] -eq 0 -and $data.Length -ge 2

            if ($isText) {
                $text = Convert-TagText $data
                if ($id -eq 'TIT2') {
                    if ($text.Trim().Length -eq 0) { $o += 10 + $fs; continue }   # 空歌名:丢掉,下面补上
                    $title = $text
                }
                $frames.Add((New-Frame $id (ConvertTo-TagData $text $ver) $fl $ver))
            } else {
                $frames.Add((Get-Slice $b $o (10 + $fs)))
            }
            $o += 10 + $fs
        }
    }

    if ($title.Length -eq 0) {
        $title = $stem
        $frames.Add((New-Frame 'TIT2' (ConvertTo-TagData $stem $ver) ([byte[]]@(0, 0)) $ver))
    }

    $total = 0
    foreach ($f in $frames) { $total += $f.Length }
    $out = New-Object IO.MemoryStream
    $out.Write([byte[]]@(0x49, 0x44, 0x33, $ver, 0, 0), 0, 6)
    $ss = Get-SyncsafeBytes $total
    $out.Write($ss, 0, 4)
    foreach ($f in $frames) { $out.Write($f, 0, $f.Length) }
    $out.Write($b, $audioStart, $b.Length - $audioStart)
    [IO.File]::WriteAllBytes($path, $out.ToArray())
    return $title
}

# ---------------------------------- 主流程 ----------------------------------
$src = $env:MTA_SRC
if (-not $src) { $src = Read-Host '请输入或拖入音乐文件夹的路径' }
if ($src) { $src = $src.Trim().Trim('"').TrimEnd('\', '/') }
if (-not $src -or -not (Test-Path -LiteralPath $src -PathType Container)) {
    Write-Host "找不到音乐文件夹:$src"
    exit 1
}
$recurse = ($env:MTA_RECURSE -eq '1')
$target = $src   # 就在原文件夹里生成;同名的副本直接覆盖

# 之前已经生成过的英文名副本:(哈希+扩展名) -> 路径,并找出最大序号。再次运行时覆盖它们。
$outRe = '^(\d{4})_([0-9a-f]{6})(\.(?:mp3|flac|wav|wave))$'
$existing = @{}
$maxIndex = 0
foreach ($p in (Get-ChildItem -LiteralPath $target -File -Recurse -ErrorAction SilentlyContinue)) {
    if ($p.Name -match $outRe) {
        $existing[($Matches[2].ToLowerInvariant() + $Matches[3].ToLowerInvariant())] = $p.FullName
        $n = [int]$Matches[1]
        if ($n -gt $maxIndex) { $maxIndex = $n }
    }
}

# 前 300 首放在文件夹本身,之后每 300 首一个 part02、part03 ……(与总数无关,序号稳定)
function Get-OutFolder([string]$base, [int]$index) {
    $group = [int][Math]::Floor(($index - 1) / $chunk)
    if ($group -eq 0) { return $base }
    return (Join-Path $base ('part{0:D2}' -f ($group + 1)))
}

function Get-Relative([string]$full, [string]$base) {
    return $full.Substring($base.Length).TrimStart('\', '/')
}

if ($recurse) { $all = Get-ChildItem -LiteralPath $src -File -Recurse }
else          { $all = Get-ChildItem -LiteralPath $src -File }
# 已转换的结果(0001_xxxxxx.mp3 这种名字)不再当作新歌
$files = @($all | Where-Object {
        $extensions -contains $_.Extension.ToLowerInvariant() -and
        -not $_.Name.StartsWith('.') -and
        -not ($_.Name -match $outRe)
    } | Sort-Object FullName)
if ($files.Count -eq 0) {
    Write-Host '没有找到需要转换的 mp3 / flac / wav 文件。请确认扩展名(Windows 默认隐藏扩展名)。'
    exit 1
}

Write-Host "文件夹:$src"
Write-Host "共 $($files.Count) 个文件"
$rows = @{}
$added = 0
$replaced = 0
$nextIndex = $maxIndex
foreach ($f in $files) {
    $ext = $f.Extension.ToLowerInvariant()
    $hash = Get-ShortHash $f.BaseName
    $key = $hash + $ext
    if ($existing.ContainsKey($key)) {
        $dst = $existing[$key]
        $status = '覆盖'
        $replaced++
    } else {
        $nextIndex++
        $dst = Join-Path (Get-OutFolder $target $nextIndex) ('{0:D4}_{1}{2}' -f $nextIndex, $hash, $ext)
        $existing[$key] = $dst
        $status = '新增'
        $added++
    }
    $dstDir = Split-Path -Parent $dst
    if (-not (Test-Path -LiteralPath $dstDir)) { New-Item -ItemType Directory -Path $dstDir | Out-Null }
    Copy-Item -LiteralPath $f.FullName -Destination $dst -Force   # 同名直接覆盖

    $title = ''
    if ($ext -eq '.mp3') {
        try {
            $title = Update-Mp3Tag $dst $f.BaseName
            if ($title -eq $null) { $title = ''; Write-Host "  提示:$($f.Name) 的标签格式较特殊,已原样保留" }
        } catch {
            Write-Host "  警告:$($f.Name) 的标签处理失败($($_.Exception.Message)),已保留副本"
        }
    }
    $rel = Get-Relative $dst $target
    $rows[$rel] = [pscustomobject]@{ '新文件名' = $rel; '原文件名' = (Get-Relative $f.FullName $src); '歌名' = $title }
    Write-Host "  [$status] $(Split-Path -Leaf $dst)  <-  $($f.Name)"
}

# 对照表.csv:合并上次的记录,本次处理过的条目更新
$csvPath = Join-Path $target '对照表.csv'
$merged = @{}
if (Test-Path -LiteralPath $csvPath) {
    foreach ($r in (Import-Csv -LiteralPath $csvPath -Encoding UTF8)) {
        if ($r.'新文件名') { $merged[$r.'新文件名'] = $r }
    }
}
foreach ($k in $rows.Keys) { $merged[$k] = $rows[$k] }
$merged.Values | Sort-Object { $_.'新文件名' } | Export-Csv -LiteralPath $csvPath -NoTypeInformation -Encoding UTF8

Write-Host ''
Write-Host "完成:新增 $added 首,覆盖 $replaced 首。英文名副本就在 $target 里;对照表.csv 记录了新旧文件名。"
Write-Host '你的中文名原文件没有被改动;确认英文名副本正常后,可以把原文件删掉。'
if (@($files | Where-Object { $_.Extension.ToLowerInvariant() -ne '.mp3' }).Count -gt 0) {
    Write-Host '注意:flac / wav 只改了文件名,没有处理标签。它们的歌名会显示成英文文件名;需要处理请改用 music_to_ascii.py。'
}
