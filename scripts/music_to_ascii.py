#!/usr/bin/env python3
"""把音乐复制成英文文件名,中文歌名和歌手保存在标签里。

为什么需要它:
  有些 SD 卡上,Switch 的文件系统读不出中文文件名的文件(DBI、DferTune 都看不到,
  英文名就正常)。DferTune 的列表显示的是歌曲标签里的歌名和歌手,不是文件名,
  所以只要文件名是英文、标签里有中文,界面上看到的仍然是中文。

它做什么(只复制,不会修改或删除你的原文件):
  1. 把 源文件夹 里的 mp3 / flac / wav 复制到 目标文件夹,文件名改成
     0001_a1b2c3.mp3 这样的英文名(序号保持原来的排序,后面是名字的短哈希,
     不同批次之间不会重名)。
  2. 如果歌曲没有歌名标签,把原来的中文文件名写进歌名标签。
  3. 把旧式 GBK 编码的中文标签修成 Unicode,避免在 DferTune 里显示乱码。
  4. 每个文件夹最多放 300 首(DferTune 的"添加全部"一次最多 300 首),超过就拆成
     part01、part02……
  5. 生成 对照表.csv,记录新旧文件名、歌名和歌手。

用法:
  pip install mutagen
  python music_to_ascii.py F:\\music F:\\music_ascii
  python music_to_ascii.py F:\\music              (目标默认是 F:\\music_ascii)
  python music_to_ascii.py F:\\music --dry-run    (只显示会做什么,不复制)
  python music_to_ascii.py F:\\music -r           (连子文件夹里的歌一起处理)

处理完把目标文件夹里的内容放进 SD 卡的 /music/ 即可。
"""

import argparse
import csv
import hashlib
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


def main() -> int:
    parser = argparse.ArgumentParser(description="把音乐复制成英文文件名,中文信息保留在标签里")
    parser.add_argument("source", type=Path, help="存放音乐的文件夹")
    parser.add_argument("target", type=Path, nargs="?", help="输出文件夹(默认:源文件夹名加 _ascii)")
    parser.add_argument("-r", "--recursive", action="store_true", help="包含子文件夹里的歌")
    parser.add_argument("--dry-run", action="store_true", help="只显示会做什么,不复制")
    args = parser.parse_args()

    source = args.source.resolve()
    if not source.is_dir():
        sys.exit(f"找不到文件夹:{source}")
    target = (args.target or source.with_name(source.name + "_ascii")).resolve()
    if target == source or source in target.parents:
        sys.exit("输出文件夹不能和源文件夹相同,也不能放在源文件夹里面")

    pattern = source.rglob("*") if args.recursive else source.glob("*")
    files = sorted(
        (p for p in pattern if p.is_file() and p.suffix.lower() in EXTENSIONS and not p.name.startswith(".")),
        key=lambda p: str(p.relative_to(source)).lower(),
    )
    if not files:
        sys.exit("没有找到 mp3 / flac / wav 文件(请确认扩展名,Windows 默认隐藏扩展名)")

    print(f"源:{source}\n目标:{target}\n共 {len(files)} 个文件" + ("(演习,不会复制)" if args.dry_run else ""))
    rows = []
    for index, src in enumerate(files, start=1):
        part = (index - 1) // CHUNK + 1
        folder = target if len(files) <= CHUNK else target / f"part{part:02d}"
        new_name = f"{index:04d}_{short_hash(src.stem)}{src.suffix.lower()}"
        dst = folder / new_name
        title = artist = ""
        if not args.dry_run:
            folder.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
            try:
                title, artist = process_tags(dst, src.stem)
            except Exception as exc:  # 标签损坏的文件仍保留副本,只是不改标签
                print(f"  警告:{src.name} 的标签处理失败({exc}),已保留副本")
        rows.append((str(dst.relative_to(target)), str(src.relative_to(source)), title, artist))
        print(f"  {new_name}  <-  {src.relative_to(source)}")

    if not args.dry_run:
        with open(target / "对照表.csv", "w", newline="", encoding="utf-8-sig") as fh:
            writer = csv.writer(fh)
            writer.writerow(["新文件名", "原文件名", "歌名", "歌手"])
            writer.writerows(rows)
        print(f"\n完成。把 {target} 里的文件放进 SD 卡的 /music/ 即可;对照表.csv 里有新旧文件名。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
