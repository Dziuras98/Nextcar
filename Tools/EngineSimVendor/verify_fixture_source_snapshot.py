#!/usr/bin/env python3
"""Verify the minimal exact-byte engine-sim fixture-source evidence snapshot."""
from __future__ import annotations
import argparse, hashlib, json, sys
from pathlib import Path, PurePosixPath

REPOSITORY = "Dziuras98/engine-sim"
COMMIT = "85f7c3b959a908ed5232ede4f1a4ac7eafe6b630"
TREE = "0756dea0444ada160540685dd1d28fcd3ef4aac5"
EXPECTED_PATHS = (
    "assets/engines/atg-video-2/01_subaru_ej25_eh.mr",
    "es/objects/objects.mr",
    "es/sound-library/impulse_responses.mr",
    "scripting/include/engine_node.h",
)
class SnapshotFailure(RuntimeError): pass

def sha256(data: bytes) -> str: return hashlib.sha256(data).hexdigest()
def git_blob(data: bytes) -> str: return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()
def fail(message: str) -> None: raise SnapshotFailure(message)

def repository_root() -> Path: return Path(__file__).resolve().parents[2]

def verify_snapshot(index_path: Path, snapshot_root: Path) -> dict:
    try:
        index = json.loads(index_path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        fail(f"cannot read snapshot index: {exc}")
    expected_top = {"schema_version":1,"repository":REPOSITORY,"commit":COMMIT,"tree":TREE,"file_count":4}
    for key,value in expected_top.items():
        if index.get(key) != value: fail(f"snapshot metadata mismatch: {key}")
    provenance=index.get("source_archive_provenance")
    if not isinstance(provenance,dict) or provenance.get("archive_sha256") != "c98f9354358da2ce7c83de24f023f865b5243f6ac02f4ea1d1acc0b549913f7c":
        fail("snapshot archive provenance mismatch")
    entries=index.get("files")
    if not isinstance(entries,list) or len(entries)!=4: fail("snapshot file list mismatch")
    paths=[]
    for entry in entries:
        if not isinstance(entry,dict) or set(entry)!={"path","sha256","git_blob","reason","parity_record_ids"}: fail("snapshot record shape mismatch")
        rel=entry["path"]
        if not isinstance(rel,str) or "\\" in rel: fail("invalid snapshot path")
        pos=PurePosixPath(rel)
        if pos.is_absolute() or ".." in pos.parts or pos.as_posix()!=rel: fail("snapshot path traversal")
        if not isinstance(entry["reason"],str) or not entry["reason"]: fail("snapshot reason missing")
        ids=entry["parity_record_ids"]
        if not isinstance(ids,list) or ids!=sorted(set(ids)) or not all(isinstance(x,str) and x for x in ids): fail("snapshot parity IDs invalid")
        path=snapshot_root.joinpath(*pos.parts)
        try: path.resolve().relative_to(snapshot_root.resolve())
        except ValueError: fail("snapshot path escapes root")
        if not path.is_file(): fail(f"missing snapshot file: {rel}")
        data=path.read_bytes()
        if sha256(data)!=entry["sha256"]: fail(f"snapshot SHA-256 mismatch: {rel}")
        if git_blob(data)!=entry["git_blob"]: fail(f"snapshot Git blob mismatch: {rel}")
        paths.append(rel)
    if tuple(paths)!=EXPECTED_PATHS: fail("snapshot paths or ordering mismatch")
    actual=sorted(p.relative_to(snapshot_root).as_posix() for p in snapshot_root.rglob("*") if p.is_file())
    if actual!=list(EXPECTED_PATHS): fail("unexpected snapshot file")
    return index

def main(argv=None) -> int:
    root=repository_root()
    ap=argparse.ArgumentParser()
    ap.add_argument("--index",type=Path,default=root/"Tools/EngineSimVendor/FixtureSourceSnapshot/SNAPSHOT.json")
    ap.add_argument("--snapshot-root",type=Path,default=root/"Tools/EngineSimVendor/FixtureSourceSnapshot/engine-sim")
    args=ap.parse_args(argv)
    try:
        index=verify_snapshot(args.index,args.snapshot_root)
    except (SnapshotFailure,OSError,UnicodeError,json.JSONDecodeError,KeyError,TypeError) as exc:
        print(f"FAIL {exc}",file=sys.stderr);return 1
    for entry in index["files"]: print(f"PASS {entry['path']} {entry['sha256']} {entry['git_blob']}")
    print("snapshot file count: 4")
    print("fixture source snapshot PASS")
    return 0
if __name__=="__main__": raise SystemExit(main())
