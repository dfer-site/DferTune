@echo off
rem ============================================================================
rem  music_to_ascii.bat  -  copy songs to English file names, originals untouched
rem
rem  Why: on some SD cards the Switch cannot list files with Chinese names.
rem  DferTune shows the title and artist from the song tags, so an English file
rem  name with the Chinese title inside the tag still looks Chinese in the UI.
rem
rem  Use: drag your music folder onto this file, or double-click it and type
rem  the folder path. Add -r after the path to include sub-folders, for example
rem      music_to_ascii.bat "F:\music" -r
rem  Output goes to a new folder next to it: the folder F:\music becomes
rem  F:\music_ascii
rem
rem  This part is plain ASCII on purpose, because cmd.exe mishandles other
rem  characters. The real work and the Chinese messages are in the PowerShell
rem  part further down.
rem ============================================================================
setlocal
chcp 65001 >nul
set "MTA_BAT=%~f0"
set "MTA_SRC=%~1"
set "MTA_RECURSE=0"
if /i "%~2"=="-r" set "MTA_RECURSE=1"
if "%MTA_SRC%"=="" set /p "MTA_SRC=Music folder (or drag it onto this file): "
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
if ($src) { $src = $src.Trim().Trim('"').TrimEnd('\', '/') }
if (-not $src -or -not (Test-Path -LiteralPath $src -PathType Container)) {
    Write-Host "找不到音乐文件夹:$src"
    exit 1
}
$recurse = ($env:MTA_RECURSE -eq '1')
$target = $src + '_ascii'
if (Test-Path -LiteralPath $target) {
    Write-Host "输出文件夹已存在:$target"
    Write-Host '为避免覆盖,请先把它改名或删除,再重新运行。'
    exit 1
}

if ($recurse) { $all = Get-ChildItem -LiteralPath $src -File -Recurse }
else          { $all = Get-ChildItem -LiteralPath $src -File }
$files = @($all | Where-Object { $extensions -contains $_.Extension.ToLowerInvariant() -and -not $_.Name.StartsWith('.') } | Sort-Object FullName)
if ($files.Count -eq 0) {
    Write-Host '没有找到 mp3 / flac / wav 文件。请确认扩展名(Windows 默认隐藏扩展名)。'
    exit 1
}

Write-Host "源:$src"
Write-Host "目标:$target"
Write-Host "共 $($files.Count) 个文件"
$rows = New-Object System.Collections.ArrayList
$index = 0
foreach ($f in $files) {
    $index++
    $part = [int][Math]::Floor(($index - 1) / $chunk) + 1
    if ($files.Count -le $chunk) { $folder = $target } else { $folder = Join-Path $target ('part{0:D2}' -f $part) }
    $ext = $f.Extension.ToLowerInvariant()
    $newName = ('{0:D4}_{1}{2}' -f $index, (Get-ShortHash $f.BaseName), $ext)
    if (-not (Test-Path -LiteralPath $folder)) { New-Item -ItemType Directory -Path $folder | Out-Null }
    $dst = Join-Path $folder $newName
    Copy-Item -LiteralPath $f.FullName -Destination $dst
    $title = ''
    if ($ext -eq '.mp3') {
        try {
            $title = Update-Mp3Tag $dst $f.BaseName
            if ($title -eq $null) { $title = ''; Write-Host "  提示:$($f.Name) 的标签格式较特殊,已原样保留" }
        } catch {
            Write-Host "  警告:$($f.Name) 的标签处理失败($($_.Exception.Message)),已保留副本"
        }
    }
    [void]$rows.Add([pscustomobject]@{ '新文件名' = $newName; '原文件名' = $f.Name; '歌名' = $title })
    Write-Host "  $newName  <-  $($f.Name)"
}

$rows | Export-Csv -LiteralPath (Join-Path $target '对照表.csv') -NoTypeInformation -Encoding UTF8
Write-Host ''
Write-Host "完成。把 $target 里的文件放进 SD 卡的 /music/ 即可;对照表.csv 里有新旧文件名。"
if (($files | Where-Object { $_.Extension -ne '.mp3' -and $_.Extension -ne '.MP3' }).Count -gt 0) {
    Write-Host '注意:flac / wav 只改了文件名,没有处理标签。它们的歌名会显示成英文文件名;需要处理请改用 music_to_ascii.py。'
}
