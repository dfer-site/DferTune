#!/usr/bin/env python3
"""把音乐转成英文文件名,中文歌名和歌手保存在标签里。

为什么需要它:
  有些 SD 卡上,Switch 的文件系统读不出中文文件名的文件(DBI、DferTune 都看不到,
  英文名就正常)。DferTune 的列表显示的是歌曲标签里的歌名和歌手,不是文件名,
  所以只要文件名是英文、标签里有中文,界面上看到的仍然是中文。

它做什么:
  1. 默认在**原文件夹里**生成英文名的副本,文件名像 0001_a1b2c3.mp3(序号 + 原名的
     短哈希)。你的中文名原文件不会被修改或删除。
  2. 同名直接覆盖:再次运行时,同一首歌(按原文件名识别)会覆盖它上次生成的副本,
     不会产生新的副本;新增的歌曲接在已有序号后面;已生成的 0001_xxxxxx.mp3
     不会被当成新歌再转一遍。
  3. 如果歌曲没有歌名标签,把原来的中文文件名写进歌名标签。
  4. 把旧式 GBK 编码的中文标签修成 Unicode,避免在 DferTune 里显示乱码。
  5. 每个文件夹最多放 300 首(DferTune 的"添加全部"只会添加前 300 首):
     前 300 首放在文件夹本身,之后依次放进 part02、part03……子文件夹。
  6. 生成 对照表.md(Markdown 表格,再次运行时合并更新),记录新旧文件名、歌名和歌手。
     以前生成过的 对照表.csv 会在第一次运行时自动并入。

用法:
  pip install mutagen
  python music_to_ascii.py F:\music                 (在 F:\music 里生成)
  python music_to_ascii.py F:\music F:\music_ascii  (指定另一个输出文件夹)
  python music_to_ascii.py F:\music --dry-run       (只显示会做什么,不写文件)
  python music_to_ascii.py F:\music -r              (连子文件夹里的歌一起处理)

处理完,原来的中文名文件 Switch 仍然读不到;确认英文名的副本正常后,可以把原文件删掉。
"""

import argparse
import csv
import hashlib
import re
import shutil
import sys
from pathlib import Path

try:
    from mutagen import File as MutagenFile
    from mutagen.flac import FLAC
    from mutagen.id3 import ID3, ID3NoHeaderError, TIT2
    from mutagen.wave import WAVE
except ImportError:
    sys.exit("缺少 mutagen,请先运行:pip install mutagen")

EXTENSIONS = {".mp3", ".flac", ".wav", ".wave"}
CHUNK = 300  # DferTune 「添加全部」一次最多 300 首
OUTPUT_NAME = re.compile(r"^(\d{4})_([0-9a-f]{6})(\.(?:mp3|flac|wav|wave))$", re.IGNORECASE)


def short_hash(text: str) -> str:
    return hashlib.sha1(text.lower().encode("utf-8")).hexdigest()[:6]


def fix_text(value: str) -> str:
    """旧标签常把 GBK 字节标成 Latin-1,读出来是乱码;能修就修回中文。"""
    if all(ord(ch) < 0x80 for ch in value):
        return value
    try:
        return value.encode("latin-1").decode("gbk")
    except (UnicodeEncodeError, UnicodeDecodeError):
        return value


def tidy_id3(tags: ID3, stem: str) -> None:
    for frame in tags.values():
        if getattr(frame, "encoding", None) == 0 and hasattr(frame, "text"):
            frame.text = [fix_text(str(t)) for t in frame.text]
        if getattr(frame, "encoding", None) in (0, 3):
            frame.encoding = 1  # UTF-16,DferTune 能正确解码
    if "TIT2" not in tags or not str(tags["TIT2"]).strip():
        tags.add(TIT2(encoding=1, text=[stem]))


def read_title_artist_id3(tags: ID3):
    title = str(tags["TIT2"]) if "TIT2" in tags else ""
    artist = str(tags["TPE1"]) if "TPE1" in tags else ""
    return title, artist


def process_tags(path: Path, stem: str):
    """修好副本里的标签,返回 (歌名, 歌手)。"""
    ext = path.suffix.lower()
    if ext == ".mp3":
        try:
            tags = ID3(path)
        except ID3NoHeaderError:
            tags = ID3()
        tidy_id3(tags, stem)
        tags.save(path, v2_version=3)
        return read_title_artist_id3(tags)
    if ext == ".flac":
        audio = FLAC(path)
        if not audio.get("title"):
            audio["title"] = stem
        audio.save()
        return (audio.get("title") or [""])[0], (audio.get("artist") or [""])[0]
    # wav:ID3 写在 RIFF 的 id3 块里
    audio = WAVE(path)
    if audio.tags is None:
        audio.add_tags()
    tidy_id3(audio.tags, stem)
    audio.save()
    return read_title_artist_id3(audio.tags)


MAPPING_NAME = "对照表.md"
LEGACY_MAPPING_NAME = "对照表.csv"
MAPPING_HEADER = ["新文件名", "原文件名", "歌名", "歌手"]


def md_escape(text: str) -> str:
    return text.replace("\\", "\\\\").replace("|", "\\|").replace("\r", " ").replace("\n", " ")


def md_cells(line: str):
    """把一行 Markdown 表格拆成单元格(支持 \\| 转义)。"""
    body = line.strip()
    if body.startswith("|"):
        body = body[1:]
    if body.endswith("|") and not body.endswith("\\|"):
        body = body[:-1]
    cells = re.split(r"(?<!\\)\|", body)
    return [c.strip().replace("\\|", "|").replace("\\\\", "\\") for c in cells]


def read_mapping(folder: Path) -> dict:
    """读取已有的对照表:优先 对照表.md,没有时导入旧的 对照表.csv。"""
    merged = {}
    md_path, csv_path = folder / MAPPING_NAME, folder / LEGACY_MAPPING_NAME
    if md_path.is_file():
        for line in md_path.read_text(encoding="utf-8-sig").splitlines():
            if not line.lstrip().startswith("|"):
                continue
            cells = md_cells(line)
            if len(cells) < 3 or cells[0] == MAPPING_HEADER[0] or set(cells[0]) <= set("-: "):
                continue
            merged[cells[0]] = (cells[1], cells[2], cells[3] if len(cells) > 3 else "")
    elif csv_path.is_file():
        with open(csv_path, newline="", encoding="utf-8-sig") as fh:
            for row in csv.DictReader(fh):
                merged[row.get("新文件名", "")] = (row.get("原文件名", ""), row.get("歌名", ""), row.get("歌手", ""))
    merged.pop("", None)
    return {k.replace("\\", "/"): (o.replace("\\", "/"), t, a) for k, (o, t, a) in merged.items()}


def write_mapping(folder: Path, merged: dict) -> None:
    lines = [
        "# DferTune 音乐对照表",
        "",
        "英文名副本和原文件的对应关系(由 music_to_ascii 自动生成,再次运行时合并更新)。",
        "",
        "| " + " | ".join(MAPPING_HEADER) + " |",
        "|" + "---|" * len(MAPPING_HEADER),
    ]
    for name in sorted(merged):
        original, title, artist = merged[name]
        lines.append("| " + " | ".join(md_escape(x) for x in (name, original, title, artist)) + " |")
    (folder / MAPPING_NAME).write_text("\n".join(lines) + "\n", encoding="utf-8")


def folder_for(target: Path, index: int) -> Path:
    """前 300 首放在目标文件夹本身,之后每 300 首一个 part02、part03……(与总数无关,序号稳定)。"""
    group = (index - 1) // CHUNK
    return target if group == 0 else target / f"part{group + 1:02d}"


def main() -> int:
    parser = argparse.ArgumentParser(description="把音乐转成英文文件名,中文信息保留在标签里")
    parser.add_argument("source", type=Path, help="存放音乐的文件夹")
    parser.add_argument("target", type=Path, nargs="?", help="输出文件夹(默认:就在源文件夹里)")
    parser.add_argument("-r", "--recursive", action="store_true", help="包含子文件夹里的歌")
    parser.add_argument("--dry-run", action="store_true", help="只显示会做什么,不写文件")
    args = parser.parse_args()

    source = args.source.resolve()
    if not source.is_dir():
        sys.exit(f"找不到文件夹:{source}")
    target = (args.target or source).resolve()

    # 已经生成过的英文名文件:(哈希, 扩展名) -> 路径,并找出最大序号。再次运行时覆盖它们。
    existing, max_index = {}, 0
    if target.is_dir():
        for p in target.rglob("*"):
            m = OUTPUT_NAME.match(p.name)
            if p.is_file() and m:
                existing[(m.group(2).lower(), m.group(3).lower())] = p
                max_index = max(max_index, int(m.group(1)))

    pattern = source.rglob("*") if args.recursive else source.glob("*")
    files = sorted(
        (
            p for p in pattern
            if p.is_file()
            and p.suffix.lower() in EXTENSIONS
            and not p.name.startswith(".")
            and not OUTPUT_NAME.match(p.name)          # 已转换的结果不再当作新歌
        ),
        key=lambda p: str(p.relative_to(source)).lower(),
    )
    if not files:
        sys.exit("没有找到需要转换的 mp3 / flac / wav 文件(请确认扩展名,Windows 默认隐藏扩展名)")

    print(f"源:{source}\n目标:{target}\n共 {len(files)} 个文件" + ("(演习,不会写文件)" if args.dry_run else ""))
    rows, added, replaced = {}, 0, 0
    next_index = max_index
    for src in files:
        ext = src.suffix.lower()
        key = (short_hash(src.stem), ext)
        if key in existing:
            dst, status = existing[key], "覆盖"
            replaced += 1
        else:
            next_index += 1
            dst = folder_for(target, next_index) / f"{next_index:04d}_{key[0]}{ext}"
            existing[key] = dst
            status = "新增"
            added += 1
        title = artist = ""
        if not args.dry_run:
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)                    # 同名直接覆盖
            try:
                title, artist = process_tags(dst, src.stem)
            except Exception as exc:  # 标签损坏的文件仍保留副本,只是不改标签
                print(f"  警告:{src.name} 的标签处理失败({exc}),已保留副本")
        rows[dst.relative_to(target).as_posix()] = (src.relative_to(source).as_posix(), title, artist)
        print(f"  [{status}] {dst.name}  <-  {src.relative_to(source)}")

    if not args.dry_run:
        merged = read_mapping(target)                 # 合并上次的记录
        merged.update(rows)
        write_mapping(target, merged)
    print(f"\n完成:新增 {added} 首,覆盖 {replaced} 首。英文名副本在 {target}"
          + ("" if args.dry_run else f";{MAPPING_NAME} 里有新旧文件名。"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
