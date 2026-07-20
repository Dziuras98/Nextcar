#!/usr/bin/env python3
"""Verify the sharded NC-003B Subaru EJ25 fixture transcription contract."""
from __future__ import annotations
import argparse, hashlib, json, math, re, sys
from collections import Counter
from pathlib import Path, PurePosixPath

SCRIPT="assets/engines/atg-video-2/01_subaru_ej25_eh.mr"
SCRIPT_SHA="04eba9d5e0f8fa2d1b2d6bfc89ee09cd1a414ecbd81d2b39f6cfd6c04fcdcb5a"
SCRIPT_BLOB="586ad92f1f27d097d5c422e2484915acc7c6a5d2"
ENGINE_COMMIT="85f7c3b959a908ed5232ede4f1a4ac7eafe6b630"
ENGINE_TREE="0756dea0444ada160540685dd1d28fcd3ef4aac5"
SNAPSHOT_SHA="3e6db8e6a18ff13fe9e7806a5125ca317c6f7653e37265ff508c629d677bf954"
SEMANTIC_SHA="3ba447789024d613cdceb2382d917e9d6ce340cbeecb621d4f71133e55578f00"
MONOLITH_SHA="096c4e2ed9b310bc1d03cffd44090ba0515162ba56ca0594ffaefe7a72bc0eaf"
CATEGORIES=("explicit-script","resolved-library-default","derived-from-script","headless-construction","determinism-patch","generated-asset","excluded-non-headless")
COUNTS=dict(zip(CATEGORIES,(67,17,10,8,1,7,8)))
KEYS={"id","category","object","field","source_path","source_anchor","source_line_context","resolved_expression","resolved_value","resolved_unit","cpp_target","comparison","tolerance","reason"}
EXCLUDED={"Vehicle","Transmission","gear ratios","vehicle drag","tire radius","rolling resistance","clutch torque","vehicle mass"}
SHARDS=[f"subaru_ej25_atg_video_2_fixture/{name}.json" for name in (
"01-engine-and-fuel","02-crankshaft-and-inertia","03-pistons-rods-and-intake","04-exhaust-banks-and-cylinders","05-heads-and-flow-tables","06-camshafts-and-valvetrain","07-ignition-and-firing-order","08-fuel-library-defaults","09-engine-and-system-defaults","10-derived-values","11-headless-construction","12-determinism-and-generated-ir","13-excluded-non-headless")]
class Failure(RuntimeError): pass
def fail(s): raise Failure(s)
def sha(b): return hashlib.sha256(b).hexdigest()
def blob(b): return hashlib.sha1(b"blob "+str(len(b)).encode()+b"\0"+b).hexdigest()
def canon(v): return json.dumps(v,ensure_ascii=False,sort_keys=True,separators=(",",":")).encode()
def text(p):
 try:return p.read_text(encoding="utf-8")
 except (OSError,UnicodeError) as e:fail(f"cannot read {p}: {e}")
def root_of(*ps):
 for p in ps:
  for q in (p.resolve(),*p.resolve().parents):
   if (q/"Plugins").is_dir() and (q/"Tools").is_dir():return q
 fail("cannot infer repository root")
def source_path(s,src,root,cpp,hdr,snapshot,ir):
 s=s.replace("\\","/")
 if s.startswith(("assets/","es/","scripting/","include/","src/")):return src/s
 for p in (cpp,hdr,snapshot,ir):
  if s.endswith(p.name):return p
 return root/s
def array(cpp,name):
 m=re.search(r"\b"+re.escape(name)+r"\s*\{\{(.*?)\}\}\s*;",cpp,re.S)
 if not m:fail(f"snapshot array missing: {name}")
 out=[]
 for t in re.findall(r"0x[\da-fA-F]+u?|true|false|[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?",m.group(1)):
  x=t.lower();out.append(True if x=="true" else False if x=="false" else int(x.rstrip("u"),16) if x.startswith("0x") else float(t) if any(c in t for c in ".eE") else int(t))
 return out
def scalar(cpp,name):
 m=re.search(r"\b"+re.escape(name)+r"\s*=\s*([-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?)\s*;",cpp)
 if not m:fail(f"snapshot scalar missing: {name}")
 return float(m.group(1))
def load_contract(spec):
 idx=json.loads(text(spec)); prov=idx.get("source_provenance",{})
 expected={"schema_version":2,"fixture_id":"SubaruEJ25AtgVideo2","fixture_slug":"subaru_ej25_atg_video_2_01","source_monolithic_candidate_sha256":MONOLITH_SHA,"total_records":118,"semantic_contract_sha256":SEMANTIC_SHA,"generated_at_utc":None,"reproducible":True}
 for k,v in expected.items():
  if idx.get(k)!=v:fail(f"index metadata mismatch: {k}")
 pexp={"repository":"Dziuras98/engine-sim","engine_sim_commit":ENGINE_COMMIT,"engine_sim_tree":ENGINE_TREE,"script_path":SCRIPT,"script_git_blob":SCRIPT_BLOB,"script_sha256":SCRIPT_SHA}
 if prov!=pexp:fail("source provenance mismatch")
 if idx.get("verification_tool")!="Tools/EngineSimVendor/verify_subaru_ej25_fixture.py" or idx.get("verification_tool_sha256")!=sha(Path(__file__).read_bytes()):fail("verification tool mismatch")
 if idx.get("category_counts")!=COUNTS or set(idx.get("excluded_components",[]))!=EXCLUDED:fail("index counts/exclusions mismatch")
 policy=idx.get("canonicalization_policy")
 if policy!={"scope":"ordered combined record array","sort_keys":True,"separators":[",",":"],"ensure_ascii":False,"encoding":"UTF-8"}:fail("canonicalization policy mismatch")
 if not isinstance(idx.get("units_policy"),dict) or not isinstance(idx.get("floating_point_comparison_policy"),dict):fail("comparison policy missing")
 entries=idx.get("ordered_shards")
 if not isinstance(entries,list) or [e.get("path") for e in entries if isinstance(e,dict)]!=SHARDS:fail("shard ordering mismatch")
 fields=[]
 for n,e in enumerate(entries):
  rel=e["path"]
  if "\\" in rel:fail("invalid shard path")
  pos=PurePosixPath(rel)
  if pos.is_absolute() or ".." in pos.parts or pos.as_posix()!=rel:fail("invalid shard path")
  path=spec.parent.joinpath(*pos.parts)
  try:path.resolve().relative_to(spec.parent.resolve())
  except ValueError:fail("shard path escapes fixture directory")
  data=path.read_bytes()
  if sha(data)!=e.get("sha256"):fail(f"shard SHA-256 mismatch: {rel}")
  doc=json.loads(data.decode("utf-8")); name=PurePosixPath(rel).stem
  if doc.get("schema_version")!=1 or doc.get("fixture_id")!="SubaruEJ25AtgVideo2" or doc.get("fixture_slug")!="subaru_ej25_atg_video_2_01" or doc.get("shard_id")!=name:fail(f"shard metadata mismatch: {rel}")
  rows=doc.get("records")
  if not isinstance(rows,list) or len(rows)!=e.get("record_count"):fail(f"shard record count mismatch: {rel}")
  fields.extend(rows)
 if len(fields)!=118:fail("total field count mismatch")
 ids=[r.get("id") for r in fields if isinstance(r,dict)]
 if len(ids)!=118 or len(set(ids))!=118:fail("field ids incomplete or duplicated")
 counts=Counter()
 for r in fields:
  if set(r)!=KEYS:fail(f"record shape mismatch: {r.get('id')}")
  if r["category"] not in CATEGORIES:fail(f"unknown category: {r['category']}")
  counts[r["category"]]+=1
 if dict(counts)!=COUNTS:fail(f"category counts mismatch: {dict(counts)}")
 if sha(canon(fields))!=idx["semantic_contract_sha256"]:fail("semantic contract digest mismatch")
 return fields,counts
def check_snapshot(cpp):
 arrays={"JournalAnglesDeg":[0,180,0,180],"BankAnglesDeg":[90,-90],"JournalMapping":[0,3,1,2],"Blowby28InH2O":[.001,.002,.001,.002],"PrimaryLengthsInch":[2,3,3,5],"SoundAttenuations":[.9,1,1.1,.9],"IntakeFlowCfm":[0,58,103,156,214,249,268,280,280,281],"ExhaustFlowCfm":[0,37,72,113,160,196,222,235,245,246],"IntakeCenterlinesDeg":[477,657,837,1017],"ExhaustCenterlinesDeg":[248,428,608,788],"TimingRpm":[0,1000,2000,3000,4000],"TimingAdvanceDeg":[25,25,30,40,40],"FiringOrderDeg":[0,180,360,540],"IgnitionCylinderMapping":[0,1,2,3],"DeterministicSeeds":[0x4E433033,0x4E433034,0x4E433035,0x4E433036]}
 for n,v in arrays.items():
  if array(cpp,n)!=v:fail(f"snapshot {n} mismatch")
 if array(cpp,"FuelTurbulenceTable")!=[0,3,5,7.5,10,15,15,24.75,20,37.5,25,46.875,30,56.25,35,65.625,40,75,45,84.375]:fail("fuel turbulence table mismatch")
 if array(cpp,"MeanPistonSpeedTurbulenceTable")!=[v for i in range(30) for v in (i,i*.5)]:fail("mean-piston-speed turbulence table mismatch")
 vals={"BankDisplayDepth":.5,"FuelMaxDilutionEffect":10,"ExhaustCollectorDiameterInch":2,"ExhaustCollectorAreaSquareInch":math.pi,"CrankThrowMm":39.5,"DeckHeightMm":195.5068,"CrankDiskMomentKgM2":.00732537375,"FlywheelMomentKgM2":.157935168,"OtherMomentKgM2":.018,"TotalCrankMomentKgM2":.18326054175,"RodMomentKgM2":.0007605085725282,"HeadIntakeRunnerAreaSquareInch":3.0625,"HeadExhaustRunnerAreaSquareInch":1.5625,"ImpulseGain":.01,"IrSampleCount":17555,"IrSampleRate":44100,"SimulationFrequencyHz":20000}
 for n,v in vals.items():
  if not math.isclose(scalar(cpp,n),v,rel_tol=1e-12,abs_tol=1e-12):fail(f"snapshot {n} mismatch")
def check_ir(ir,fixture):
 for n,v in {"MinimalMuffling02SampleCount":17555,"MinimalMuffling02SampleRate":44100,"MinimalMuffling02Channels":1,"MinimalMuffling02BitsPerSample":16}.items():
  m=re.search(r"\b"+n+r"\s*=\s*(\d+)",ir)
  if not m or int(m.group(1))!=v:fail(f"generated IR {n} mismatch")
 if not re.search(r"MinimalMuffling02Pcm\[\]\s*=\s*\{\s*(-?\d+)",ir):fail("generated PCM missing")
 if not all(x in ir for x in ("ce0702aa501d94f35b5f804efcd1b21688b9f9cacaa0fa2fc7f459c03a98e540","df0b8be829d28ae64e5b2818a1942a3b3e5733186bdf262cad4c2af038995d48","6d3f8688e170cb6e5f4bfec42f580f3900514d72")):fail("generated IR provenance mismatch")
 if "MinimalMuffling02Pcm" not in fixture or re.search(r"[\"'][^\"']*\.wav[\"']",fixture,re.I) or any(x in fixture for x in ("ifstream","fopen(","filesystem","loadWav","load_wav")):fail("runtime WAV dependency detected")
def main(argv=None):
 ap=argparse.ArgumentParser()
 for n in ("spec","source-root","fixture-cpp","fixture-header","ir-header"):ap.add_argument("--"+n,required=True,type=Path)
 a=ap.parse_args(argv)
 try:
  fields,counts=load_contract(a.spec);root=root_of(a.spec,a.fixture_cpp,a.source_root);snap_path=a.fixture_header.with_name("SubaruEJ25AtgVideo2FixtureSpec.h")
  sb=(a.source_root/SCRIPT).read_bytes();script=sb.decode();fixture=text(a.fixture_cpp);header=text(a.fixture_header);snap=text(snap_path);ir=text(a.ir_header)
  if sha(sb)!=SCRIPT_SHA or blob(sb)!=SCRIPT_BLOB:fail("pinned script hash/blob mismatch")
  if sha(snap.encode())!=SNAPSHOT_SHA:fail("fixture snapshot SHA-256 mismatch")
  for r in fields:
   src=text(source_path(r["source_path"],a.source_root,root,a.fixture_cpp,a.fixture_header,snap_path,a.ir_header))
   if src.count(r["source_anchor"])!=1:fail(f"source anchor is not unique: {r['id']}")
   if r["source_line_context"] not in src:fail(f"source context missing: {r['id']}")
   if r["category"]!="excluded-non-headless" and str(r["cpp_target"]) not in snap+fixture+header+ir:fail(f"C++ target missing: {r['id']}")
  objects=text(a.source_root/"es/objects/objects.mr");anchor="input collector_cross_section_area: circle_area(2.0 * units.inch);"
  if objects.count(anchor)!=1 or re.search(r"(?<!runner_)collector_cross_section_area\s*:",script) or "circle_area(2.0 * units.inch)" not in fixture or "Snapshot.ExhaustCollectorAreaSquareInch" not in fixture:fail("collector circle-area semantics mismatch")
  check_snapshot(snap);check_ir(ir,fixture)
  corpus=fixture+"\n"+header+"\n"+snap
  for x in ('#include "vehicle.h"','#include <vehicle.h>','#include "transmission.h"','#include <transmission.h>',"Vehicle ","Vehicle*","Transmission ","Transmission*"):
   if x in corpus:fail(f"non-headless dependency detected: {x}")
  if any(x in corpus for x in ("random_device","std::rand","rand(","srand(","time(nullptr)")):fail("unclassified nondeterminism detected")
  if "&MeanPistonSpeedTurbulence" not in fixture or re.search(r"meanPistonSpeedToTurbulence\s*=\s*&FuelTurbulence",fixture):fail("independent chamber turbulence missing")
  if "bank.displayDepth = Snapshot.BankDisplayDepth" not in fixture or "fuel.maxDilutionEffect = Snapshot.FuelMaxDilutionEffect" not in fixture:fail("fixture defaults missing")
  if "crank.pos_x = Snapshot.CrankPositionX" not in fixture or "crank.pos_y = Snapshot.CrankPositionY" not in fixture:fail("crank position fields mismatch")
  print("all shard hashes PASS");print("semantic contract digest PASS")
  for r in fields:print(f"PASS {r['id']} {r['category']}")
  for c in CATEGORIES:print(f"{c}: {counts[c]}")
  print("total: 118");return 0
 except (Failure,OSError,UnicodeError,json.JSONDecodeError,KeyError,TypeError) as e:print(f"FAIL {e}",file=sys.stderr);return 1
if __name__=="__main__":raise SystemExit(main())
