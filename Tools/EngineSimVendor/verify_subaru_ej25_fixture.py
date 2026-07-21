#!/usr/bin/env python3
"""Verify the sharded NC-003B Subaru EJ25 fixture transcription contract."""
from __future__ import annotations
import argparse, hashlib, json, math, re, sys
from collections import Counter
from pathlib import Path, PurePosixPath
from verify_fixture_source_snapshot import SnapshotFailure, verify_snapshot

SCRIPT="assets/engines/atg-video-2/01_subaru_ej25_eh.mr"
SCRIPT_SHA="04eba9d5e0f8fa2d1b2d6bfc89ee09cd1a414ecbd81d2b39f6cfd6c04fcdcb5a"
SCRIPT_BLOB="586ad92f1f27d097d5c422e2484915acc7c6a5d2"
ENGINE_COMMIT="85f7c3b959a908ed5232ede4f1a4ac7eafe6b630"
ENGINE_TREE="0756dea0444ada160540685dd1d28fcd3ef4aac5"
FIXTURE_SNAPSHOT_SHA="3e6db8e6a18ff13fe9e7806a5125ca317c6f7653e37265ff508c629d677bf954"
SEMANTIC_SHA="3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00"
MONOLITH_SHA="096c4e2ed9b310bc1d03cffd44090ba0515162ba56ca0594ffaefe7a72bc0eaf"
CATEGORIES=("explicit-script","resolved-library-default","derived-from-script","headless-construction","determinism-patch","generated-asset","excluded-non-headless")
COUNTS=dict(zip(CATEGORIES,(67,17,10,8,1,7,8)))
KEYS={"id","category","object","field","source_path","source_anchor","source_line_context","resolved_expression","resolved_value","resolved_unit","cpp_target","comparison","tolerance","reason"}
EXCLUDED={"Vehicle","Transmission","gear ratios","vehicle drag","tire radius","rolling resistance","clutch torque","vehicle mass"}
SHARDS=[f"subaru_ej25_atg_video_2_fixture/{name}.json" for name in (
"01-engine-and-fuel","02-crankshaft-and-inertia","03-pistons-rods-and-intake","04-exhaust-banks-and-cylinders","05-heads-and-flow-tables","06-camshafts-and-valvetrain","07-ignition-and-firing-order","08-fuel-library-defaults","09-engine-and-system-defaults","10-derived-values","11-headless-construction","12-determinism-and-generated-ir","13-excluded-non-headless")]
class Failure(RuntimeError): pass
def fail(message): raise Failure(message)
def sha(data): return hashlib.sha256(data).hexdigest()
def blob(data): return hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()
def canon(value): return json.dumps(value,ensure_ascii=False,sort_keys=True,separators=(",",":")).encode()
def text(path):
    try:return path.read_text(encoding="utf-8")
    except (OSError,UnicodeError) as exc:fail(f"cannot read {path}: {exc}")
def repository_root(): return Path(__file__).resolve().parents[2]
def source_path(source,source_root,root,fixture_cpp,fixture_header,fixture_snapshot,ir_header):
    source=source.replace("\\","/")
    if source.startswith(("assets/","es/","scripting/","include/","src/")):return source_root/source
    for path in (fixture_cpp,fixture_header,fixture_snapshot,ir_header):
        if source.endswith(path.name):return path
    return root/source
def array(cpp,name):
    match=re.search(r"\b"+re.escape(name)+r"\s*\{\{(.*?)\}\}\s*;",cpp,re.S)
    if not match:fail(f"snapshot array missing: {name}")
    out=[]
    for token in re.findall(r"0x[\da-fA-F]+u?|true|false|[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?",match.group(1)):
        lower=token.lower();out.append(True if lower=="true" else False if lower=="false" else int(lower.rstrip("u"),16) if lower.startswith("0x") else float(token) if any(c in token for c in ".eE") else int(token))
    return out
def scalar(cpp,name):
    match=re.search(r"\b"+re.escape(name)+r"\s*=\s*([-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?)\s*;",cpp)
    if not match:fail(f"snapshot scalar missing: {name}")
    return float(match.group(1))
def load_contract(spec):
    index=json.loads(text(spec));provenance=index.get("source_provenance",{})
    expected={"schema_version":2,"fixture_id":"SubaruEJ25AtgVideo2","fixture_slug":"subaru_ej25_atg_video_2_01","source_monolithic_candidate_sha256":MONOLITH_SHA,"total_records":118,"semantic_contract_sha256":SEMANTIC_SHA,"generated_at_utc":None,"reproducible":True}
    for key,value in expected.items():
        if index.get(key)!=value:fail(f"index metadata mismatch: {key}")
    pexp={"repository":"Dziuras98/engine-sim","engine_sim_commit":ENGINE_COMMIT,"engine_sim_tree":ENGINE_TREE,"script_path":SCRIPT,"script_git_blob":SCRIPT_BLOB,"script_sha256":SCRIPT_SHA}
    if provenance!=pexp:fail("source provenance mismatch")
    if index.get("verification_tool")!="Tools/EngineSimVendor/verify_subaru_ej25_fixture.py" or index.get("verification_tool_sha256")!=sha(Path(__file__).read_bytes()):fail("verification tool mismatch")
    if index.get("category_counts")!=COUNTS or set(index.get("excluded_components",[]))!=EXCLUDED:fail("index counts/exclusions mismatch")
    policy=index.get("canonicalization_policy")
    if policy!={"scope":"ordered combined record array","sort_keys":True,"separators":[",",":"],"ensure_ascii":False,"encoding":"UTF-8"}:fail("canonicalization policy mismatch")
    if not isinstance(index.get("units_policy"),dict) or not isinstance(index.get("floating_point_comparison_policy"),dict):fail("comparison policy missing")
    entries=index.get("ordered_shards")
    if not isinstance(entries,list) or [entry.get("path") for entry in entries if isinstance(entry,dict)]!=SHARDS:fail("shard ordering mismatch")
    fields=[]
    for entry in entries:
        rel=entry["path"]
        if "\\" in rel:fail("invalid shard path")
        pos=PurePosixPath(rel)
        if pos.is_absolute() or ".." in pos.parts or pos.as_posix()!=rel:fail("invalid shard path")
        path=spec.parent.joinpath(*pos.parts)
        try:path.resolve().relative_to(spec.parent.resolve())
        except ValueError:fail("shard path escapes fixture directory")
        data=path.read_bytes()
        if sha(data)!=entry.get("sha256"):fail(f"shard SHA-256 mismatch: {rel}")
        doc=json.loads(data.decode("utf-8"));name=PurePosixPath(rel).stem
        if doc.get("schema_version")!=1 or doc.get("fixture_id")!="SubaruEJ25AtgVideo2" or doc.get("fixture_slug")!="subaru_ej25_atg_video_2_01" or doc.get("shard_id")!=name:fail(f"shard metadata mismatch: {rel}")
        rows=doc.get("records")
        if not isinstance(rows,list) or len(rows)!=entry.get("record_count"):fail(f"shard record count mismatch: {rel}")
        fields.extend(rows)
    if len(fields)!=118:fail("total field count mismatch")
    ids=[record.get("id") for record in fields if isinstance(record,dict)]
    if len(ids)!=118 or len(set(ids))!=118:fail("field ids incomplete or duplicated")
    counts=Counter()
    for record in fields:
        if set(record)!=KEYS:fail(f"record shape mismatch: {record.get('id')}")
        if record["category"] not in CATEGORIES:fail(f"unknown category: {record['category']}")
        counts[record["category"]]+=1
    if dict(counts)!=COUNTS:fail(f"category counts mismatch: {dict(counts)}")
    if sha(canon(fields))!=index["semantic_contract_sha256"]:fail("semantic contract digest mismatch")
    return fields,counts
def check_fixture_snapshot(cpp):
    arrays={"JournalAnglesDeg":[0,180,0,180],"BankAnglesDeg":[90,-90],"JournalMapping":[0,3,1,2],"Blowby28InH2O":[.001,.002,.001,.002],"PrimaryLengthsInch":[2,3,3,5],"SoundAttenuations":[.9,1,1.1,.9],"IntakeFlowCfm":[0,58,103,156,214,249,268,280,280,281],"ExhaustFlowCfm":[0,37,72,113,160,196,222,235,245,246],"IntakeCenterlinesDeg":[477,657,837,1017],"ExhaustCenterlinesDeg":[248,428,608,788],"TimingRpm":[0,1000,2000,3000,4000],"TimingAdvanceDeg":[25,25,30,40,40],"FiringOrderDeg":[0,180,360,540],"IgnitionCylinderMapping":[0,1,2,3],"DeterministicSeeds":[0x4E433033,0x4E433034,0x4E433035,0x4E433036]}
    for name,value in arrays.items():
        if array(cpp,name)!=value:fail(f"snapshot {name} mismatch")
    if array(cpp,"FuelTurbulenceTable")!=[0,3,5,7.5,10,15,15,24.75,20,37.5,25,46.875,30,56.25,35,65.625,40,75,45,84.375]:fail("fuel turbulence table mismatch")
    if array(cpp,"MeanPistonSpeedTurbulenceTable")!=[value for i in range(30) for value in (i,i*.5)]:fail("mean-piston-speed turbulence table mismatch")
    values={"BankDisplayDepth":.5,"FuelMaxDilutionEffect":10,"ExhaustCollectorDiameterInch":2,"ExhaustCollectorAreaSquareInch":math.pi,"CrankThrowMm":39.5,"DeckHeightMm":195.5068,"CrankDiskMomentKgM2":.00732537375,"FlywheelMomentKgM2":.157935168,"OtherMomentKgM2":.018,"TotalCrankMomentKgM2":.18326054175,"RodMomentKgM2":.0007605085725282,"HeadIntakeRunnerAreaSquareInch":3.0625,"HeadExhaustRunnerAreaSquareInch":1.5625,"ImpulseGain":.01,"IrSampleCount":17555,"IrSampleRate":44100,"SimulationFrequencyHz":20000}
    for name,value in values.items():
        if not math.isclose(scalar(cpp,name),value,rel_tol=1e-12,abs_tol=1e-12):fail(f"snapshot {name} mismatch")
def check_ir(ir,fixture):
    for name,value in {"MinimalMuffling02SampleCount":17555,"MinimalMuffling02SampleRate":44100,"MinimalMuffling02Channels":1,"MinimalMuffling02BitsPerSample":16}.items():
        match=re.search(r"\b"+name+r"\s*=\s*(\d+)",ir)
        if not match or int(match.group(1))!=value:fail(f"generated IR {name} mismatch")
    if not re.search(r"MinimalMuffling02Pcm\[\]\s*=\s*\{\s*(-?\d+)",ir):fail("generated PCM missing")
    if not all(value in ir for value in ("ce0702aa501d94f35b5f804efcd1b21688b9f9cacaa0fa2fc7f459c03a98e540","df0b8be829d28ae64e5b2818a1942a3b3e5733186bdf262cad4c2af038995d48","6d3f8688e170cb6e5f4bfec42f580f3900514d72")):fail("generated IR provenance mismatch")
    if "MinimalMuffling02Pcm" not in fixture or re.search(r"[\"'][^\"']*\.wav[\"']",fixture,re.I) or any(value in fixture for value in ("ifstream","fopen(","filesystem","loadWav","load_wav")):fail("runtime WAV dependency detected")
def main(argv=None):
    root=repository_root()
    parser=argparse.ArgumentParser()
    parser.add_argument("--spec",type=Path,default=root/"Tests/EngineSimCore/Fixtures/subaru_ej25_atg_video_2_fixture.json")
    parser.add_argument("--source-root",type=Path,default=None,help="Explicit update/mutation source root; repository verification defaults to FixtureSourceSnapshot.")
    parser.add_argument("--snapshot-index",type=Path,default=root/"Tools/EngineSimVendor/FixtureSourceSnapshot/SNAPSHOT.json")
    parser.add_argument("--fixture-cpp",type=Path,default=root/"Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Fixtures/SubaruEJ25AtgVideo2Fixture.cpp")
    parser.add_argument("--fixture-header",type=Path,default=root/"Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Fixtures/SubaruEJ25AtgVideo2Fixture.h")
    parser.add_argument("--ir-header",type=Path,default=root/"Plugins/NextcarEngineSim/Source/NextcarEngineSimCore/Private/Generated/MinimalMuffling02ImpulseResponse.generated.h")
    args=parser.parse_args(argv)
    source_root=args.source_root or root/"Tools/EngineSimVendor/FixtureSourceSnapshot/engine-sim"
    try:
        verify_snapshot(args.snapshot_index,source_root)
        fields,counts=load_contract(args.spec);fixture_snapshot_path=args.fixture_header.with_name("SubaruEJ25AtgVideo2FixtureSpec.h")
        script_bytes=(source_root/SCRIPT).read_bytes();script=script_bytes.decode();fixture=text(args.fixture_cpp);header=text(args.fixture_header);fixture_snapshot=text(fixture_snapshot_path);ir=text(args.ir_header)
        if sha(script_bytes)!=SCRIPT_SHA or blob(script_bytes)!=SCRIPT_BLOB:fail("pinned script hash/blob mismatch")
        if sha(fixture_snapshot.encode())!=FIXTURE_SNAPSHOT_SHA:fail("fixture snapshot SHA-256 mismatch")
        for record in fields:
            source=text(source_path(record["source_path"],source_root,root,args.fixture_cpp,args.fixture_header,fixture_snapshot_path,args.ir_header))
            if source.count(record["source_anchor"])!=1:fail(f"source anchor is not unique: {record['id']}")
            if record["source_line_context"] not in source:fail(f"source context missing: {record['id']}")
            if record["category"]!="excluded-non-headless" and str(record["cpp_target"]) not in fixture_snapshot+fixture+header+ir:fail(f"C++ target missing: {record['id']}")
        objects=text(source_root/"es/objects/objects.mr");anchor="input collector_cross_section_area: circle_area(2.0 * units.inch);"
        if objects.count(anchor)!=1 or re.search(r"(?<!runner_)collector_cross_section_area\s*:",script) or "circle_area(2.0 * units.inch)" not in fixture or "Snapshot.ExhaustCollectorAreaSquareInch" not in fixture:fail("collector circle-area semantics mismatch")
        check_fixture_snapshot(fixture_snapshot);check_ir(ir,fixture)
        corpus=fixture+"\n"+header+"\n"+fixture_snapshot
        for value in ('#include "vehicle.h"','#include <vehicle.h>','#include "transmission.h"','#include <transmission.h>',"Vehicle ","Vehicle*","Transmission ","Transmission*"):
            if value in corpus:fail(f"non-headless dependency detected: {value}")
        if any(value in corpus for value in ("random_device","std::rand","rand(","srand(","time(nullptr)")):fail("unclassified nondeterminism detected")
        if "&MeanPistonSpeedTurbulence" not in fixture or re.search(r"meanPistonSpeedToTurbulence\s*=\s*&FuelTurbulence",fixture):fail("independent chamber turbulence missing")
        if "bank.displayDepth = Snapshot.BankDisplayDepth" not in fixture or "fuel.maxDilutionEffect = Snapshot.FuelMaxDilutionEffect" not in fixture:fail("fixture defaults missing")
        if "crank.pos_x = Snapshot.CrankPositionX" not in fixture or "crank.pos_y = Snapshot.CrankPositionY" not in fixture:fail("crank position fields mismatch")
        print("fixture source snapshot PASS");print("all shard hashes PASS");print("semantic contract digest PASS")
        for record in fields:print(f"PASS {record['id']} {record['category']}")
        for category in CATEGORIES:print(f"{category}: {counts[category]}")
        print("total: 118");return 0
    except (Failure,SnapshotFailure,OSError,UnicodeError,json.JSONDecodeError,KeyError,TypeError) as exc:
        print(f"FAIL {exc}",file=sys.stderr);return 1
if __name__=="__main__":raise SystemExit(main())
